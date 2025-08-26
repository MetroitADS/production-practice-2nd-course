#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "eventdialog.h"
#include "settingsdialog.h"
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>
#include <QFileDialog>
#include <QSystemTrayIcon>
#include <QPushButton>
#include <QSysInfo>
#include <QAction>
#include <QSettings>
#include <QPixmap>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_networkSync(new NetworkSync(this))
    , m_connectedToServer(false)
    , m_trayIcon(nullptr)
    , m_notificationTimer(nullptr)
{
    ui->setupUi(this);
    QIcon appIcon("icon.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
    }

    m_localEvents = QVector<Event>();
    m_serverEvents = QVector<Event>();

    m_connectedToServer = m_networkSync->isConnected();
    ui->statusBar->showMessage(m_connectedToServer ? "На сервере" : "Локально");

    connect(ui->calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onCalendarClicked);
    connect(ui->eventsList, &QListWidget::itemSelectionChanged, this, &MainWindow::onEventSelected);
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(ui->editButton, &QPushButton::clicked, this, &MainWindow::onEditButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);
    connect(ui->syncButton, &QPushButton::clicked, this, &MainWindow::onSyncButtonClicked);
    connect(m_networkSync, &NetworkSync::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectButtonClicked);

    // Сеть
    connect(m_networkSync, &NetworkSync::syncStarted, this, &MainWindow::onSyncStarted);
    connect(m_networkSync, &NetworkSync::syncFinished, this, &MainWindow::onSyncFinished);
    connect(m_networkSync, &NetworkSync::eventsDownloaded, this, &MainWindow::onEventsDownloaded);

    // Меню
    connect(ui->actionSettings_3, &QAction::triggered, this, &MainWindow::onSettingsActionTriggered);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onExportActionTriggered);
    connect(ui->actionImport, &QAction::triggered, this, &MainWindow::onImportActionTriggered);

    // Инициализация
    ui->editButton->setEnabled(false);
    ui->deleteButton->setEnabled(false);

    // Файл
    loadEventsFromFile();
    updateEventsList();
    updateCalendarColors();

    setupNotifications();//Уведомления

    autoSyncIfEnabled();
}

MainWindow::~MainWindow()
{
    // Сохранение только локальных событий при выходе
    saveEventsToFile();

    if (m_networkSync) {
        disconnect(m_networkSync, nullptr, this, nullptr);
    }

    delete ui;
}

void MainWindow::onCalendarClicked(const QDate& date)
{
    Q_UNUSED(date);
    updateEventsList();
}

//-==========================-
// Обработка выбора события в списке
//-==========================-
void MainWindow::onEventSelected()
{
    QList<QListWidgetItem*> selectedItems = ui->eventsList->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();
    ui->editButton->setEnabled(hasSelection);
    ui->deleteButton->setEnabled(hasSelection);

    if (hasSelection) {
        QListWidgetItem* item = selectedItems.first();
        if (item) {
            Event event = item->data(Qt::UserRole).value<Event>();
            showEventDetails(event);
        }
    }
    else {
        ui->eventDetails->clear();
    }
}

//-==========================-
// Добавление нового события
//-==========================-
void MainWindow::onAddButtonClicked()
{
    EventDialog dialog(this);
    dialog.setWindowTitle("Добавление события");
    QDate selectedDate = ui->calendarWidget->selectedDate();
    QDateTime startDateTime(selectedDate, QTime(9, 0));
    QDateTime endDateTime = startDateTime.addSecs(3600);
    dialog.setDateTime(startDateTime, endDateTime);

    if (dialog.exec() == QDialog::Accepted) {
        Event newEvent = dialog.getEvent();

        if (m_connectedToServer && m_networkSync->isConnected()) {
            try {
                newEvent.setSource(Event::Server);
                m_networkSync->uploadSingleEvent(newEvent);
                m_serverEvents.append(newEvent); // Добавляем локально сразу
                ui->statusBar->showMessage("Событие отправляется на сервер...", 3000);
            }
            catch (...) {
                // Если ошибка при отправке, сохраняем локально
                newEvent.setSource(Event::Local);
                m_localEvents.append(newEvent);
                saveEventsToFile();
                ui->statusBar->showMessage("Ошибка отправки, событие сохранено локально", 3000);
            }
        }
        else {
            newEvent.setSource(Event::Local);
            m_localEvents.append(newEvent);
            saveEventsToFile();
            ui->statusBar->showMessage("Событие сохранено локально", 3000);
        }

        updateEventsList();
        updateCalendarColors();
    }
}

//-==========================-
// Редактирование события
//-==========================-
void MainWindow::onEditButtonClicked()
{
    QListWidgetItem* item = ui->eventsList->currentItem();
    if (!item) return;

    Event oldEvent = item->data(Qt::UserRole).value<Event>();

    // Определяем, в каком массиве искать событие
    QVector<Event>* targetEvents = (oldEvent.source() == Event::Local) ?
        &m_localEvents : &m_serverEvents;

    EventDialog dialog(this);
    dialog.setWindowTitle("Edit Event");
    dialog.setEvent(oldEvent);

    if (dialog.exec() == QDialog::Accepted) {
        Event updatedEvent = dialog.getEvent();
        updatedEvent.setId(oldEvent.id());
        updatedEvent.setSource(oldEvent.source()); // Сохраняем источник

        // Находим событие в соответствующем массиве
        for (int i = 0; i < targetEvents->size(); ++i) {
            if (targetEvents->at(i).id() == oldEvent.id()) {
                (*targetEvents)[i] = updatedEvent;
                updateEventsList();

                if (oldEvent.source() == Event::Local) {
                    saveEventsToFile(); // Сохраняем только локальные
                }

                updateCalendarColors();

                // Автоматическое обновление на сервере, если это серверное событие
                if (oldEvent.source() == Event::Server && m_connectedToServer) {
                    m_networkSync->updateEvent(updatedEvent);
                    // Сообщение будет показано через сигнал syncFinished/errorOccurred
                }
                else if (oldEvent.source() == Event::Local && m_connectedToServer) {
                    // Если редактируем локальное событие при подключении к серверу,
                    // отправляем обновление на сервер
                    m_networkSync->uploadSingleEvent(updatedEvent);
                    ui->statusBar->showMessage("Событие сохранено с сервером", 3000);
                }
                else {
                    ui->statusBar->showMessage("Событие сохранено", 3000);
                }
                break;
            }
        }
    }
}

//-==========================-
// Удаление события
//-==========================-
void MainWindow::onDeleteButtonClicked()
{
    QListWidgetItem* item = ui->eventsList->currentItem();
    if (!item) return;

    Event eventToDelete = item->data(Qt::UserRole).value<Event>();
    QString eventId = eventToDelete.id();
    QString eventName = eventToDelete.title();

    if (QMessageBox::question(this, "Удаление события",
        QString("Точно хочешь удалить? (╯°益°)╯彡┻━ '%1'").arg(eventName)) == QMessageBox::Yes) {

        // Определяем, в каком массиве удалять
        QVector<Event>* targetEvents = (eventToDelete.source() == Event::Local) ?
            &m_localEvents : &m_serverEvents;

        for (int i = 0; i < targetEvents->size(); ++i) {
            if (targetEvents->at(i).id() == eventId) {
                targetEvents->removeAt(i);
                break;
            }
        }

        updateEventsList();

        if (eventToDelete.source() == Event::Local) {
            saveEventsToFile(); // Сохраняем только локальные
        }

        updateCalendarColors();

        // Автоматическое удаление на сервере, если подключены
        if (m_connectedToServer) {
            m_networkSync->deleteEvent(eventId);
            // Сообщение будет показано через сигнал syncFinished/errorOccurred
        }
        else {
            ui->statusBar->showMessage("Событие удалено", 3000);
        }
    }
}

//-==========================-
// Синхронизация с сервером
//-==========================-
void MainWindow::onSyncButtonClicked()
{
    if (!m_networkSync->isConnected()) {
        QMessageBox::warning(this, "Ошибка соединения",
            "Возможно ошибка в URL сервера");
        return;
    }

    qDebug() << "Синхронизация с сервером" << m_networkSync->isConnected();
    ui->statusBar->showMessage("Подключение к серверу...");

    m_networkSync->downloadEvents();
}

//-==========================-
// Обработка загруженных событий
//-==========================-
void MainWindow::onEventsDownloaded(const QVector<Event>& downloadedEvents)
{
    // Очищаем старые серверные события и добавляем новые
    m_serverEvents.clear();
    for (const Event& event : downloadedEvents) {
        Event serverEvent = event;
        serverEvent.setSource(Event::Server); // Помечаем как серверное
        m_serverEvents.append(serverEvent);
    }

    // Обновляем интерфейс
    updateEventsList();
    updateCalendarColors();
    ui->statusBar->showMessage("Событие с сервера было скачано", 3000);

    // Уведомляем о завершении синхронизации
    emit m_networkSync->syncFinished(true, "Скачалось");
}

//-==========================-
// Обновление цветов календаря
//-==========================-
void MainWindow::updateCalendarColors()
{
    // Создаем карту дат с событиями
    QMap<QDate, QColor> dateColors;

    if (m_connectedToServer) {
        // Если подключены к серверу, используем только серверные события
        for (const Event& event : m_serverEvents) {
            QDate eventDate = event.start().date();
            dateColors[eventDate] = event.color();
        }
    }
    else {
        // Если не подключены к серверу, используем только локальные события
        for (const Event& event : m_localEvents) {
            QDate eventDate = event.start().date();
            if (!dateColors.contains(eventDate) || dateColors[eventDate] == Qt::white) {
                dateColors[eventDate] = event.color();
            }
        }
    }

    // Устанавливаем форматер дат для подсветки
    ui->calendarWidget->setDateTextFormat(QDate(), QTextCharFormat()); // Сброс

    QTextCharFormat format;
    format.setBackground(QBrush(Qt::lightGray));
    format.setFontWeight(QFont::Bold);

    for (auto it = dateColors.begin(); it != dateColors.end(); ++it) {
        format.setBackground(it.value());
        ui->calendarWidget->setDateTextFormat(it.key(), format);
    }
}

//-==========================-
// Обработка ошибок
//-==========================-
void MainWindow::onErrorOccurred(const QString& error)
{
    // Если это ошибка прав доступа, показываем информативное сообщение
    if (error.contains("Нет прав", Qt::CaseInsensitive) ||
        error.contains("Запрещено", Qt::CaseInsensitive)) {
        QMessageBox::information(this, "Информация",
            "У вас нет прав для изменения событий на сервере. "
            "Вы можете работать с событиями локально.");
    }
    else {
        // Для других ошибок показываем предупреждение
        QMessageBox::warning(this, "Ошибка", error);
    }

    ui->statusBar->showMessage("Ошибка: " + error, 5000);
    m_connectedToServer = false;

    // Обновляем статус в UI
    ui->statusBar->showMessage(m_connectedToServer ? "На сервере" : "Локально");

    updateEventsList();
    updateCalendarColors();
}

//-==========================-
// Настройки приложения
//-==========================-
void MainWindow::onSettingsActionTriggered()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_networkSync->setServerUrl(dialog.serverUrl());
        m_networkSync->setAuthToken(dialog.authToken());

        QSettings settings;
        settings.setValue("sync/auto", dialog.autoSync());
    }
}

//-==========================-
// Экспорт событий
//-==========================-
void MainWindow::onExportActionTriggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Events", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray eventsArray;

        // Экспортируем и локальные и серверные события
        for (const Event& event : m_localEvents) {
            eventsArray.append(event.toJson());
        }
        for (const Event& event : m_serverEvents) {
            eventsArray.append(event.toJson());
        }

        QJsonDocument doc(eventsArray);
        file.write(doc.toJson());
        file.close();

        ui->statusBar->showMessage("Events exported successfully", 3000);
    }
}

//-==========================-
// Импорт событий
//-==========================-
void MainWindow::onImportActionTriggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Events", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray eventsArray = doc.array();

        for (const QJsonValue& value : eventsArray) {
            Event event = Event::fromJson(value.toObject());
            event.setSource(Event::Local); // Импортируем как локальные
            m_localEvents.append(event);
        }

        updateEventsList();
        saveEventsToFile();
        ui->statusBar->showMessage("Events imported successfully", 3000);
    }
}

//-==========================-
// Начало синхронизации
//-==========================-
void MainWindow::onSyncStarted()
{
    ui->statusBar->showMessage("Synchronizing events...");
    ui->syncButton->setEnabled(false);
}

//-==========================-
// Завершение синхронизации
//-==========================-
void MainWindow::onSyncFinished(bool success, const QString& message)
{
    ui->syncButton->setEnabled(true);

    // Пропускаем ошибку "No events received" если это было после добавления
    if (message.contains("No events received", Qt::CaseInsensitive)) {
        // Это нормально для операций добавления/обновления
        ui->statusBar->showMessage("Операция выполнена успешно", 3000);
        m_connectedToServer = true;
        updateEventsList();
        updateCalendarColors();
        return;
    }

    if (success) {
        ui->statusBar->showMessage("Sync completed: " + message, 3000);
        m_connectedToServer = true;
        mergeServerAndLocalEvents();
        updateEventsList();
        updateCalendarColors();
    }
    else {
        QMessageBox::warning(this, "Sync Error", message);
        ui->statusBar->showMessage("Sync failed: " + message, 5000);
        m_connectedToServer = false;
        updateEventsList();
        updateCalendarColors();
    }

    ui->statusBar->showMessage(m_connectedToServer ? "Connected to server" : "Working offline");
}

//-==========================-
// Отображение деталей события
//-==========================-
void MainWindow::showEventDetails(const Event& event)
{
    QString details = QString("<h3>%1</h3>"
        "<p><b>Time:</b> %2 - %3</p>"
        "<p><b>Description:</b><br>%4</p>")
        .arg(event.title(),
            event.start().toString("dd.MM.yyyy hh:mm"),
            event.end().toString("dd.MM.yyyy hh:mm"),
            event.description());

    ui->eventDetails->setHtml(details);
}

//-==========================-
// Сохранение событий в файл
//-==========================-
void MainWindow::saveEventsToFile()
{
    QFile file("events.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray eventsArray;
        for (const Event& event : m_localEvents) { // Сохраняем только локальные
            eventsArray.append(event.toJson());
        }

        QJsonDocument doc(eventsArray);
        file.write(doc.toJson());
        file.close();
    }
}

//-==========================-
// Загрузка событий из файла
//-==========================-
void MainWindow::loadEventsFromFile()
{
    QFile file("events.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isArray()) {
            QJsonArray eventsArray = doc.array();
            m_localEvents.clear(); // Очищаем перед загрузкой

            for (const QJsonValue& value : eventsArray) {
                if (value.isObject()) {
                    Event event = Event::fromJson(value.toObject());
                    event.setSource(Event::Local);
                    m_localEvents.append(event);
                }
            }
        }
        file.close();

        qDebug() << "Loaded" << m_localEvents.size() << "local events";
    }
    else {
        qDebug() << "No local events file found";
    }
}

//-==========================-
// Отключение от сервера
//-==========================-
void MainWindow::onDisconnectButtonClicked()
{
    m_connectedToServer = false;
    updateEventsList();
    updateCalendarColors();
    ui->statusBar->showMessage("Disconnected from server", 3000);
}

//-==========================-
// Автосинхронизация при запуске
//-==========================-
void MainWindow::autoSyncIfEnabled()
{
    if (autoSyncEnabled()) {
        QSettings settings;
        m_networkSync->setServerUrl(settings.value("server/url", "http://localhost:3000/api").toString());
        m_networkSync->setAuthToken(settings.value("server/token").toString());

        m_connectedToServer = true;
        m_networkSync->downloadEvents();
        ui->statusBar->showMessage("Auto-syncing with server...");
    }
    else {
        ui->statusBar->showMessage("Working offline");
    }
}

//-==========================-
// Проверка включения автосинхронизации
//-==========================-
bool MainWindow::autoSyncEnabled() const
{
    QSettings settings;
    return settings.value("sync/auto", false).toBool();
}

//-==========================-
// Объединение серверных и локальных событий
//-==========================-
void MainWindow::mergeServerAndLocalEvents()
{
    // Удаляем локальные события, которые есть на сервере (чтобы избежать дублирования)
    for (int i = m_localEvents.size() - 1; i >= 0; i--) {
        for (const Event& serverEvent : m_serverEvents) {
            if (m_localEvents[i].id() == serverEvent.id()) {
                m_localEvents.removeAt(i);
                break;
            }
        }
    }

    saveEventsToFile(); // Сохраняем обновленный список локальных событий
}

//-==========================-
// Загрузка локальных событий на сервер (Потом)
//-==========================-
/*/
void MainWindow::uploadLocalEventsToServer()
{
    if (!m_connectedToServer) {
        QMessageBox::information(this, "Информация", "Не подключено к серверу");
        return;
    }
    for (const Event& localEvent : m_localEvents) {
        bool existsOnServer = false;
        for (const Event& serverEvent : m_serverEvents) {
            if (serverEvent.id() == localEvent.id()) {
                existsOnServer = true;
                break;
            }
        }
        if (!existsOnServer) {
            Event eventToUpload = localEvent;
            eventToUpload.setSource(Event::Server);
            m_networkSync->uploadSingleEvent(eventToUpload);
            m_serverEvents.append(eventToUpload);
        }
    }
    m_localEvents.clear();
    saveEventsToFile();
    updateEventsList();
    updateCalendarColors();
    ui->statusBar->showMessage("Локальное событие было загружено на сервер", 3000);
}
//*/

//-==========================-
// Обновление списка событий
//-==========================-
void MainWindow::updateEventsList()
{
    ui->eventsList->clear();
    QDate selectedDate = ui->calendarWidget->selectedDate();

    // Всегда показываем локальные события
    for (const Event& event : m_localEvents) {
        if (event.start().date() == selectedDate) {
            QString itemText = QString("%1 - %2 (Локальное)")
                .arg(event.start().time().toString("hh:mm"))
                .arg(event.title());

            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setBackground(event.color());
            item->setData(Qt::UserRole, QVariant::fromValue(event));
            ui->eventsList->addItem(item);
        }
    }

    // Показываем серверные события только если подключены
    if (m_connectedToServer) {
        for (const Event& event : m_serverEvents) {
            if (event.start().date() == selectedDate) {
                QString itemText = QString("%1 - %2 (Серверное)")
                    .arg(event.start().time().toString("hh:mm"))
                    .arg(event.title());

                QListWidgetItem* item = new QListWidgetItem(itemText);
                item->setBackground(event.color());
                item->setData(Qt::UserRole, QVariant::fromValue(event));
                ui->eventsList->addItem(item);
            }
        }
    }

    // Сбрасываем выделение кнопок, если список пуст
    if (ui->eventsList->count() == 0) {
        ui->editButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
        ui->eventDetails->clear();
    }
}

//-==========================-
// Настройка уведомлений
//-==========================-
void MainWindow::setupNotifications()
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(QApplication::windowIcon());
        m_trayIcon->setToolTip("Modern Calendar");
        m_trayIcon->show();
        connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
            this->show();
            this->activateWindow();
            });


    }

    // Таймер проверки уведомлений
    m_notificationTimer = new QTimer(this);
    connect(m_notificationTimer, &QTimer::timeout,
        this, &MainWindow::checkForEventNotifications);
    m_notificationTimer->start(30000);
    QTimer::singleShot(5000, this, &MainWindow::checkForEventNotifications);
}

//-==========================-
// Проверка уведомлений о событиях
//-==========================-
void MainWindow::checkForEventNotifications()
{
    QDateTime now = QDateTime::currentDateTime();
    QSet<QString> notifiedEvents; // Чтобы избежать повторных уведомлений

    // Проверяем локальные события
    for (const Event& event : m_localEvents) {
        if (shouldNotifyEvent(event, now) && !notifiedEvents.contains(event.id())) {
            showEventNotification(event, getNotificationMessage(event, now));
            notifiedEvents.insert(event.id());
        }
    }

    // Проверяем серверные события
    for (const Event& event : m_serverEvents) {
        if (shouldNotifyEvent(event, now) && !notifiedEvents.contains(event.id())) {
            showEventNotification(event, getNotificationMessage(event, now));
            notifiedEvents.insert(event.id());
        }
    }
}

//-==========================-
// Проверка необходимости уведомления
//-==========================-
bool MainWindow::shouldNotifyEvent(const Event& event, const QDateTime& now)
{
    // Проверяем, не отключены ли уведомления для этого события
    if (m_dismissedNotifications.contains(event.id())) {
        QDateTime dismissTime = m_dismissedNotifications[event.id()];
        if (dismissTime.secsTo(now) < 86400) { // 24 часа
            return false;
        }
    }

    QDateTime eventStart = event.start();
    qint64 secondsToStart = now.secsTo(eventStart);

    // Уведомляем за 10 минут, за 1 минуту и в момент начала
    return (secondsToStart <= 600 && secondsToStart >= 570) || // 10 минут до
        (secondsToStart <= 60 && secondsToStart >= 30) ||   // 1 минута до
        (secondsToStart >= -60 && secondsToStart <= 0);     // В момент начала (±1 минута)
}

//-==========================-
// Получение ID уведомления
//-==========================-
QString MainWindow::getNotificationId(const Event& event) const
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsToStart = now.secsTo(event.start());

    // Создаем уникальный ID для каждого типа уведомления (10min, 1min, start)
    QString type;
    if (secondsToStart <= 600 && secondsToStart >= 570) {
        type = "10min";
    }
    else if (secondsToStart <= 60 && secondsToStart >= 30) {
        type = "1min";
    }
    else {
        type = "start";
    }

    return QString("%1_%2_%3").arg(event.id())
        .arg(event.start().date().toString("yyyyMMdd"))
        .arg(type);
}

//-==========================-
// Откладывание уведомления (Потом)
//-==========================-
/*/
void MainWindow::dismissEventNotification(const QString& eventId)
{
    m_dismissedNotifications[eventId] = QDateTime::currentDateTime();
    QSettings settings;
    settings.beginWriteArray("dismissed_notifications");
    int index = 0;
    for (auto it = m_dismissedNotifications.begin(); it != m_dismissedNotifications.end(); ++it) {
        settings.setArrayIndex(index++);
        settings.setValue("eventId", it.key());
        settings.setValue("dismissTime", it.value());
    }
    settings.endArray();
}
//*/

//-==========================-
// Формирование сообщения уведомления
//-==========================-
QString MainWindow::getNotificationMessage(const Event& event, const QDateTime& now)
{
    qint64 secondsToStart = now.secsTo(event.start());
    QString timeInfo;

    if (secondsToStart <= 600 && secondsToStart >= 570) {
        timeInfo = QString("Через 10 минут: %1").arg(event.title());
    }
    else if (secondsToStart <= 60 && secondsToStart >= 30) {
        timeInfo = QString("Через 1 минуту: %1").arg(event.title());
    }
    else if (secondsToStart >= -60 && secondsToStart <= 0) {
        timeInfo = QString("Сейчас начинается: %1").arg(event.title());
    }
    else {
        timeInfo = event.title();
    }
    QString message = QString("%1\n\nВремя: %2 - %3")
        .arg(timeInfo)
        .arg(event.start().toString("dd.MM.yyyy hh:mm"))
        .arg(event.end().toString("dd.MM.yyyy hh:mm"));
    if (!event.description().isEmpty()) {
        message += QString("\n\nОписание: %1").arg(event.description());
    }
    return message;
}

//-==========================-
// Показ уведомления о событии
//-==========================-
void MainWindow::showEventNotification(const Event& event, const QString& message)
{
    QString notificationId = getNotificationId(event);

    if (m_shownNotifications.contains(notificationId)) {
        return;
    }

    m_shownNotifications.insert(notificationId);

    // Используем только QSystemTrayIcon для всех платформ
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage("Календарь - Напоминание",
            message, QSystemTrayIcon::Information, 15000);
    }
    else {
        // Если иконке кердык
        QMessageBox::information(this, "Календарь - Напоминание", message);
    }

    QApplication::beep();
}