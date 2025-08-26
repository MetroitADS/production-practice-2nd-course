#include "eventdialog.h"
#include "ui_eventdialog.h"
#include <QColorDialog>

EventDialog::EventDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::EventDialog),
    m_color(Qt::blue) 
{
    ui->setupUi(this);
    ui->startDateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->endDateTimeEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
    connect(ui->colorButton, &QPushButton::clicked, this, &EventDialog::onColorButtonClicked);
    updateColorButton();
}

EventDialog::~EventDialog()
{
    delete ui;
}

Event EventDialog::getEvent() const
{
    Event event(
        ui->titleEdit->text(),                    // Заголовок из текстового поля
        ui->descriptionEdit->toPlainText(),       // Описание из текстовой области
        ui->startDateTimeEdit->dateTime(),        // Время начала из редактора даты/времени
        ui->endDateTimeEdit->dateTime(),          // Время окончания из редактора даты/времени
        m_color);                                 // Цвет события

    qDebug() << "Создано событие с датой начала:" << event.start();
    return event;
}

void EventDialog::setEvent(const Event& event)
{
    // Заполнение полей диалога
    ui->titleEdit->setText(event.title());
    ui->descriptionEdit->setText(event.description());
    ui->startDateTimeEdit->setDateTime(event.start());
    ui->endDateTimeEdit->setDateTime(event.end());
    m_color = event.color();
    updateColorButton();
}

void EventDialog::onColorButtonClicked()
{
    QColor newColor = QColorDialog::getColor(m_color, this, "Choose Event Color");
    if (newColor.isValid()) {
        m_color = newColor;
        updateColorButton();
    }
}

void EventDialog::updateColorButton()
{
    QString textColor = m_color.lightness() > 128 ? "black" : "white";
    ui->colorButton->setStyleSheet(
        QString("background-color: %1; color: %2; border: 1px solid %3;")
        .arg(m_color.name())            // Основной цвет фона
        .arg(textColor)                 // Цвет текста
        .arg(m_color.darker().name())); // Цвет границы (более темный оттенок)
}

void EventDialog::setDateTime(const QDateTime& start, const QDateTime& end)
{
    ui->startDateTimeEdit->setDateTime(start);
    ui->endDateTimeEdit->setDateTime(end);
}