#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include <QColor>
#include "event.h"

namespace Ui {
    class EventDialog;
}

class EventDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventDialog(QWidget* parent = nullptr);
    ~EventDialog();
    Event getEvent() const;
    void setEvent(const Event& event);
    void setDateTime(const QDateTime& start, const QDateTime& end);

private slots:
    void onColorButtonClicked();

private:
    Ui::EventDialog* ui;
    QColor m_color;

    void updateColorButton();
};

#endif // EVENTDIALOG_H