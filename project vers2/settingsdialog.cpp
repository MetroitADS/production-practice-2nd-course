#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings settings;
    ui->serverUrlEdit->setText(settings.value("server/url", "http://localhost:3000/api").toString());
    ui->authTokenEdit->setText(settings.value("server/token").toString());
    ui->autoSyncCheckBox->setChecked(settings.value("sync/auto", false).toBool());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

QString SettingsDialog::serverUrl() const
{
    return ui->serverUrlEdit->text();
}

QString SettingsDialog::authToken() const
{
    return ui->authTokenEdit->text();
}

bool SettingsDialog::autoSync() const
{
    return ui->autoSyncCheckBox->isChecked();
}

void SettingsDialog::setServerUrl(const QString& url)
{
    ui->serverUrlEdit->setText(url);
}

void SettingsDialog::setAuthToken(const QString& token)
{
    ui->authTokenEdit->setText(token);
}

void SettingsDialog::setAutoSync(bool autoSync)
{
    ui->autoSyncCheckBox->setChecked(autoSync);
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue("server/url", ui->serverUrlEdit->text());
    settings.setValue("server/token", ui->authTokenEdit->text());
    settings.setValue("sync/auto", ui->autoSyncCheckBox->isChecked());

    QDialog::accept();
}