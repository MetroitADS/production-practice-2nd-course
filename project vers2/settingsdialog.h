#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

    QString serverUrl() const;
    QString authToken() const;
    bool autoSync() const;
    void accept();

    void setServerUrl(const QString& url);
    void setAuthToken(const QString& token);
    void setAutoSync(bool autoSync);

private:
    Ui::SettingsDialog* ui;
};

#endif // SETTINGSDIALOG_H