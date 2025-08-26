#include "calendarwidget.h"
#include "eventdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCalendarWidget>
#include <QListWidget>
#include <QListWidgetItem> 
#include <QPushButton>
#include <QMessageBox>

CalendarWidget::CalendarWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
}

CalendarWidget::~CalendarWidget()
{
}

void CalendarWidget::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_calendar = new QCalendarWidget(this);
    layout->addWidget(m_calendar);
    m_eventsList = new QListWidget(this);
    m_eventsList->setSelectionMode(QAbstractItemView::SingleSelection); 
    layout->addWidget(m_eventsList);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Добавить событие", this);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addStretch(); 
    layout->addLayout(buttonLayout);
    connect(m_calendar, &QCalendarWidget::clicked, this, &CalendarWidget::onDateSelected);
    connect(m_addButton, &QPushButton::clicked, this, &CalendarWidget::onAddEvent);
    connect(m_eventsList, &QListWidget::itemDoubleClicked, this, &CalendarWidget::onEventDoubleClicked);
    onDateSelected(QDate::currentDate());
}

void CalendarWidget::addEvent(const Event& event)
{
    m_events.append(event);
    updateEventsList();
}

QVector<Event> CalendarWidget::eventsForDate(const QDate& date) const
{
    QVector<Event> result;
    for (const Event& event : m_events) {
        qDebug() << "Проверка события:" << event.title()
            << "на" << event.start().date() << date;
        if (event.start().date() == date) {
            result.append(event);
        }
    }
    return result;
}

void CalendarWidget::onDateSelected(const QDate& date)
{
    Q_UNUSED(date);
    updateEventsList();
}


void CalendarWidget::onAddEvent()
{
    EventDialog dialog(this);
    dialog.setWindowTitle("Добавить новое событие");
    if (dialog.exec() == QDialog::Accepted) {
        Event newEvent = dialog.getEvent();
        qDebug() << "Добавление события:" << newEvent.title() << "на" << newEvent.start().toString();
        m_events.append(newEvent);
        updateEventsList();
        qDebug() << "Всего событий теперь:" << m_events.size();
    }
}

void CalendarWidget::onEventDoubleClicked(QListWidgetItem* item)// Костыль
{
    if (!item) return;

    QVariant data = item->data(Qt::UserRole);
    if (data.canConvert<Event>()) {
        Event event = data.value<Event>();
        emit eventDoubleClicked(event);
    }
}

void CalendarWidget::updateEventsList()
{
    qDebug() << "Обновление списка событий для даты:" << m_calendar->selectedDate();
    m_eventsList->clear();
    QDate selectedDate = m_calendar->selectedDate();
    QVector<Event> events = eventsForDate(selectedDate);
    qDebug() << "Найдено" << events.size() << "событий на эту дату";
    for (const Event& event : events) {
        QString itemText = QString("%1 - %2").arg(
            event.start().time().toString("hh:mm"),
            event.title());
        qDebug() << "Добавление события в список:" << itemText;
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setBackground(event.color());
        item->setData(Qt::UserRole, QVariant::fromValue(event));
        m_eventsList->addItem(item);
    }
}