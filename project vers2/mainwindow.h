// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMap>
#include <QSet>
#include "event.h"
#include "networksync.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onCalendarClicked(const QDate& date);
    void onEventSelected();
    void onAddButtonClicked();
    void onEditButtonClicked();
    void onDeleteButtonClicked();
    void onSyncButtonClicked();
    void onSettingsActionTriggered();
    void onExportActionTriggered();
    void onImportActionTriggered();
    void onErrorOccurred(const QString& error);
    void onDisconnectButtonClicked();

    void onSyncStarted();
    void onSyncFinished(bool success, const QString& message);
    void onEventsDownloaded(const QVector<Event>& events);

private:
    Ui::MainWindow* ui;
    QVector<Event> m_localEvents;
    QVector<Event> m_serverEvents;
    NetworkSync* m_networkSync;
    QSystemTrayIcon* m_trayIcon; 
    QTimer* m_notificationTimer;
    bool m_connectedToServer;

    // Уведомления
    QMap<QString, QDateTime> m_dismissedNotifications;
    QSet<QString> m_shownNotifications;

    void autoSyncIfEnabled();
    void mergeServerAndLocalEvents();
    void setupNotifications();
    void checkForEventNotifications();
    void showEventNotification(const Event& event, const QString& message);
    bool shouldNotifyEvent(const Event& event, const QDateTime& now);

    QString getNotificationId(const Event& event) const;
    QString getNotificationMessage(const Event& event, const QDateTime& now);

    void updateEventsList();
    bool autoSyncEnabled() const;
    void showEventDetails(const Event& event);
    void saveEventsToFile();
    void loadEventsFromFile();
    void updateCalendarColors();
};
#endif // MAINWINDOW_H