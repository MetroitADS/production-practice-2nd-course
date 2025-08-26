// event.h
#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QDateTime>
#include <QColor>
#include <QJsonObject>

class Event
{
public:
    enum Source {
        Local,      
        Server  
    };

    Event(const QString& title = "", const QString& description = "",
        const QDateTime& start = QDateTime(), const QDateTime& end = QDateTime(),
        const QColor& color = Qt::blue, const QString& id = "",
        Source source = Local);

    QString title() const;
    QString description() const;
    QDateTime start() const;
    QDateTime end() const;
    QColor color() const;
    QString id() const;
    Source source() const;
    bool isValid() const { return !m_id.isEmpty() && !m_title.isEmpty(); }
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setStart(const QDateTime& start);
    void setEnd(const QDateTime& end);
    void setColor(const QColor& color);
    void setId(const QString& id);
    void setSource(Source source);
    QJsonObject toJson() const;
    static Event fromJson(const QJsonObject& json);

private:
    QString m_title;
    QString m_description;
    QDateTime m_start;
    QDateTime m_end;
    QColor m_color;
    QString m_id;
    Source m_source;
};

#endif // EVENT_H