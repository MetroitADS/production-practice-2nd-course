#include "event.h"
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>

Event::Event(const QString& title, const QString& description,
    const QDateTime& start, const QDateTime& end, const QColor& color,
    const QString& id, Source source)
    : m_title(title), m_description(description), m_start(start),
    m_end(end), m_color(color), m_source(source)
{
    if (id.isEmpty()) {
        m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    else {
        m_id = id;
    }
}

QString Event::title() const { return m_title; }
QString Event::description() const { return m_description; }
QDateTime Event::start() const { return m_start; }
QDateTime Event::end() const { return m_end; }
QColor Event::color() const { return m_color; }
QString Event::id() const { return m_id; }

void Event::setTitle(const QString& title) { m_title = title; }
void Event::setDescription(const QString& description) { m_description = description; }
void Event::setStart(const QDateTime& start) { m_start = start; }
void Event::setEnd(const QDateTime& end) { m_end = end; }
void Event::setColor(const QColor& color) { m_color = color; }
void Event::setId(const QString& id) { m_id = id; }

Event::Source Event::source() const { return m_source; }
void Event::setSource(Source source) { m_source = source; }

QJsonObject Event::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["title"] = m_title;
    json["description"] = m_description;
    json["start"] = m_start.toString(Qt::ISODate);
    json["end"] = m_end.toString(Qt::ISODate);
    json["color"] = m_color.name();
    json["source"] = m_source;
    return json;
}

Event Event::fromJson(const QJsonObject& json)
{
    Event event;
    event.setId(json["id"].toString());
    event.setTitle(json["title"].toString());
    event.setDescription(json["description"].toString());
    event.setStart(QDateTime::fromString(json["start"].toString(), Qt::ISODate));
    event.setEnd(QDateTime::fromString(json["end"].toString(), Qt::ISODate));
    event.setColor(QColor(json["color"].toString()));
    event.setSource(static_cast<Event::Source>(json["source"].toInt(Event::Local)));
    return event;
}