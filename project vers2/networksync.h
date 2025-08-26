#ifndef NETWORKSYNC_H
#define NETWORKSYNC_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVector>
#include "event.h"

class NetworkSync : public QObject
{
    Q_OBJECT

public:
    explicit NetworkSync(QObject* parent = nullptr);
    ~NetworkSync();

    void setServerUrl(const QString& url);
    void setAuthToken(const QString& token);

    void syncEvents(const QVector<Event>& events);
    void downloadEvents();
    void uploadEventsAfterDownload(const QVector<Event>& localEvents);
    void uploadEvents(const QVector<Event>& events);
    void deleteEvent(const QString& eventId);
    void updateEvent(const Event& event);
    bool isConnected() const;
    void uploadSingleEvent(const Event& event);

signals:
    void syncStarted();
    void syncFinished(bool success, const QString& message);
    void eventsDownloaded(const QVector<Event>& events);
    void errorOccurred(const QString& error);
    void singleOperationFinished(bool success, const QString& message);

private slots:
    void onUploadFinished(QNetworkReply* reply);
    void onDownloadFinished(QNetworkReply* reply);
    void onErrorOccurred(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_serverUrl;
    QString m_authToken;

    QJsonArray eventsToJsonArray(const QVector<Event>& events);
    QVector<Event> jsonArrayToEvents(const QJsonArray& jsonArray);
};

#endif // NETWORKSYNC_H