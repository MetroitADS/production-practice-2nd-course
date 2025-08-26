#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <QWidget>
#include <QDate>
#include <QVector>
#include "event.h"

class QCalendarWidget;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class CalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarWidget(QWidget* parent = nullptr);
    ~CalendarWidget();
    void addEvent(const Event& event);
    QVector<Event> eventsForDate(const QDate& date) const;

signals:
    void eventDoubleClicked(const Event& event);

private slots:
    void onDateSelected(const QDate& date);
    void onAddEvent();
    void onEventDoubleClicked(QListWidgetItem* item);  

private:
    QCalendarWidget* m_calendar;
    QListWidget* m_eventsList;
    QPushButton* m_addButton;
    QVector<Event> m_events;
    void setupUi();
    void updateEventsList();
};

#endif // CALENDARWIDGET_H