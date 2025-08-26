#include "networksync.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

NetworkSync::NetworkSync(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    QSettings settings;
    m_serverUrl = settings.value("server/url", "http://localhost:3000/api").toString();
    m_authToken = settings.value("server/token").toString();
    connect(m_networkManager, &QNetworkAccessManager::finished,
        this, &NetworkSync::onDownloadFinished);
}

NetworkSync::~NetworkSync()
{
    delete m_networkManager;
}

void NetworkSync::setServerUrl(const QString& url)          //URL  -=========================
{
    m_serverUrl = url;
    QSettings settings;
    settings.setValue("server/url", url); // Сохранение в настройках
}

void NetworkSync::setAuthToken(const QString& token)        //Токен -=========================
{
    m_authToken = token;
    QSettings settings;
    settings.setValue("server/token", token); // Сохранение в настройках
}

void NetworkSync::syncEvents(const QVector<Event>& events)  //Сервер синхронизация
{
    emit syncStarted();
    downloadEvents();
}

void NetworkSync::downloadEvents()
{
    QUrl serverUrl(m_serverUrl + "/events");
    if (!serverUrl.isValid()) {
        emit errorOccurred("Неправильный URL " + m_serverUrl);
        return;
    }
    qDebug() << "Подключение к :" << serverUrl.toString();

    QNetworkRequest request(serverUrl);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
        qDebug() << "Использование токена";
    }

    QNetworkReply* reply = m_networkManager->get(request);
    if (!reply) {
        emit errorOccurred("Не удалось создать сетевой запрос");
        return;
    }
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
    qDebug() << "Сетевой запрос запущен";
}

//-==========================-
// Отправка событий на сервер
//-==========================-
void NetworkSync::uploadEvents(const QVector<Event>& events)
{
    QNetworkRequest request(QUrl(m_serverUrl + "/events/sync"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["events"] = eventsToJsonArray(events);

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onUploadFinished(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
}

//-==========================-
// Обработка завершения загрузки
//-==========================-
void NetworkSync::onUploadFinished(QNetworkReply* reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString response = QString::fromUtf8(reply->readAll());

    qDebug() << "Загрузка завершена. Статус:" << statusCode << "Ответ:" << response;

    if (statusCode == 403) {
        emit syncFinished(false, "У вас нет прав для изменения событий на сервере");
    }
    else if (statusCode == 201 || statusCode == 200) {
        emit syncFinished(true, "Операция выполнена успешно");
    }
    else if (reply->error() == QNetworkReply::NoError) {
        emit syncFinished(true, "Операция выполнена успешно");
    }
    else {
        emit syncFinished(false, reply->errorString() + ". Response: " + response);
    }
    reply->deleteLater();
}

void NetworkSync::onDownloadFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);

        // Проверяем HTTP статус код
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 403) {
            emit syncFinished(false, "Y BAC HET PRAV");
            reply->deleteLater();
            return;
        }

        QVector<Event> downloadedEvents;

        if (doc.isArray()) {
            // Если ответ - массив событий
            downloadedEvents = jsonArrayToEvents(doc.array());
        }
        else if (doc.isObject()) {
            // Если ответ - объект с полем events
            QJsonObject responseObj = doc.object();
            if (responseObj.contains("events") && responseObj["events"].isArray()) {
                QJsonArray eventsArray = responseObj["events"].toArray();
                downloadedEvents = jsonArrayToEvents(eventsArray);
            }
        }

        if (!downloadedEvents.isEmpty()) {
            emit eventsDownloaded(downloadedEvents);
            emit syncFinished(true, "События успешно загружены");
        }
        //else {
        //    emit syncFinished(false, "Никаких событий с сервера не получено");
        //}
    }
    else {
        // Обработка других ошибок
        emit syncFinished(false, reply->errorString());
    }
    reply->deleteLater();
}

//-==========================-
// Отправка событий после загрузки
//-==========================-
void NetworkSync::uploadEventsAfterDownload(const QVector<Event>& localEvents)
{
    // Отправляем только локальные события
    QVector<Event> eventsToSend;
    for (const Event& event : localEvents) {
        if (event.source() == Event::Local) {
            eventsToSend.append(event);
        }
    }

    QNetworkRequest request(QUrl(m_serverUrl + "/events/sync"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["events"] = eventsToJsonArray(eventsToSend);

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onUploadFinished(reply); });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
}

//-==========================-
// Удаление события с сервера
//-==========================-
void NetworkSync::deleteEvent(const QString& eventId)
{
    QNetworkRequest request(QUrl(m_serverUrl + "/events/" + eventId));
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QNetworkReply* reply = m_networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 403) {
            emit errorOccurred("У вас нет прав для удаления событий на сервере");
        }
        else if (reply->error() == QNetworkReply::NoError) {
            emit syncFinished(true, "Событие удалено");
        }
        else {
            emit syncFinished(false, reply->errorString());
        }
        reply->deleteLater();
        });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
}

//-==========================-
// Обновление события на сервере
//-==========================-
void NetworkSync::updateEvent(const Event& event)
{
    QNetworkRequest request(QUrl(m_serverUrl + "/events/" + event.id()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject eventJson = event.toJson();
    QNetworkReply* reply = m_networkManager->put(request, QJsonDocument(eventJson).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUploadFinished(reply);
        });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
}

//-==========================-
// Обработка сетевых ошибок
//-==========================-
void NetworkSync::onErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString errorMessage;
    if (statusCode == 403) {
        errorMessage = "У вас нет прав для выполнения этой операции на сервере";
    }
    else if (statusCode == 401) {
        errorMessage = "Неверный токен авторизации";
    }
    else if (statusCode == 404) {
        errorMessage = "Сервер не найден или недоступен";
    }
    else {
        errorMessage = reply->errorString();
        if (errorMessage.contains("Connection refused")) {
            errorMessage = "Не удалось подключиться к серверу. Проверьте URL и доступность сервера";
        }
        else if (errorMessage.contains("Host not found")) {
            errorMessage = "Сервер не найден. Проверьте правильность URL";
        }
        else if (errorMessage.contains("Timeout")) {
            errorMessage = "Превышено время ожидания ответа от сервера";
        }
    }

    emit errorOccurred(errorMessage);
}

QJsonArray NetworkSync::eventsToJsonArray(const QVector<Event>& events)
{
    QJsonArray array;
    for (const Event& event : events) {
        array.append(event.toJson());
    }
    return array;
}
QVector<Event> NetworkSync::jsonArrayToEvents(const QJsonArray& jsonArray)
{
    QVector<Event> events;
    for (const QJsonValue& value : jsonArray) {
        events.append(Event::fromJson(value.toObject()));
    }
    return events;
}

//-==========================-
// Отправка одиночного события
//-==========================-
void NetworkSync::uploadSingleEvent(const Event& event)
{
    QNetworkRequest request(QUrl(m_serverUrl + "/events"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject eventJson = event.toJson();
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(eventJson).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onUploadFinished(reply);
        });
    connect(reply, &QNetworkReply::errorOccurred, this, &NetworkSync::onErrorOccurred);
}

//-==========================-
// Проверка подключения к серверу
//-==========================-
bool NetworkSync::isConnected() const
{
    return !m_serverUrl.isEmpty() && QUrl(m_serverUrl).isValid();
}