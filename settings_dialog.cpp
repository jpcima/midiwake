#include "settings_dialog.h"
#include "ui_settings_dialog.h"

struct SettingsDialog::Impl
{
    Ui::SettingsDialog ui;
    QSettings *m_settings = nullptr;
    QTimer *m_timer = nullptr;
};

SettingsDialog::SettingsDialog(QSettings *settings, bool hasAutostart, QWidget *parent)
    : QDialog(parent), m_impl(new Impl)
{
    m_impl->m_settings = settings;

    Ui::SettingsDialog &ui = m_impl->ui;
    ui.setupUi(this);

    ui.valWakeDuration->setValue(settings->value("wake-duration").toInt());
    QObject::connect(
        ui.valWakeDuration, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](int value) {
            m_impl->m_settings->setValue("wake-duration", value);
            emit wakeDurationChanged(value);
        });

    ui.chkAutostart->setChecked(hasAutostart);
    QObject::connect(
        ui.chkAutostart, &QCheckBox::toggled,
        this, &SettingsDialog::autostartChanged);
}

SettingsDialog::~SettingsDialog()
{
}
