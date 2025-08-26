#ifndef UI_SETTINGSDIALOG_H
#define UI_SETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SettingsDialog
{
public:
    QVBoxLayout* verticalLayout;
    QWidget* widget;
    QFormLayout* formLayout;
    QLabel* label;
    QLineEdit* serverUrlEdit;
    QLabel* label_2;
    QLineEdit* authTokenEdit;
    QCheckBox* autoSyncCheckBox;
    QDialogButtonBox* buttonBox;

    void setupUi(QDialog* SettingsDialog)
    {
        if (SettingsDialog->objectName().isEmpty())
            SettingsDialog->setObjectName(QString::fromUtf8("SettingsDialog"));
        SettingsDialog->resize(400, 200);
        SettingsDialog->setMinimumSize(QSize(400, 200));
        SettingsDialog->setMaximumSize(QSize(500, 250));
        verticalLayout = new QVBoxLayout(SettingsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        widget = new QWidget(SettingsDialog);
        widget->setObjectName(QString::fromUtf8("widget"));
        formLayout = new QFormLayout(widget);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(widget);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        serverUrlEdit = new QLineEdit(widget);
        serverUrlEdit->setObjectName(QString::fromUtf8("serverUrlEdit"));
        serverUrlEdit->setPlaceholderText(QString::fromUtf8("http://localhost:3000/api"));

        formLayout->setWidget(0, QFormLayout::FieldRole, serverUrlEdit);

        label_2 = new QLabel(widget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        authTokenEdit = new QLineEdit(widget);
        authTokenEdit->setObjectName(QString::fromUtf8("authTokenEdit"));
        authTokenEdit->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(1, QFormLayout::FieldRole, authTokenEdit);

        autoSyncCheckBox = new QCheckBox(widget);
        autoSyncCheckBox->setObjectName(QString::fromUtf8("autoSyncCheckBox"));

        formLayout->setWidget(2, QFormLayout::FieldRole, autoSyncCheckBox);


        verticalLayout->addWidget(widget);

        buttonBox = new QDialogButtonBox(SettingsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(SettingsDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, SettingsDialog, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, SettingsDialog, &QDialog::reject);

        QMetaObject::connectSlotsByName(SettingsDialog);
    } 

    void retranslateUi(QDialog* SettingsDialog)
    {
        SettingsDialog->setWindowTitle(QCoreApplication::translate("SettingsDialog", "Настройки", nullptr));
        label->setText(QCoreApplication::translate("SettingsDialog", "URL:", nullptr));
        label_2->setText(QCoreApplication::translate("SettingsDialog", "Token:", nullptr));
        autoSyncCheckBox->setText(QCoreApplication::translate("SettingsDialog", "Автоматический запуск", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SettingsDialog : public Ui_SettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDIALOG_H