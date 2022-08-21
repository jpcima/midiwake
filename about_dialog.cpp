#include "about_dialog.h"
#include "ui_about_dialog.h"

struct AboutDialog::Impl {
    Ui::AboutDialog ui;
};

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), m_impl(new Impl)
{
    Ui::AboutDialog &ui = m_impl->ui;
    ui.setupUi(this);

    ui.txtDisplayName->setText(APPLICATION_DISPLAY_NAME);
    ui.txtVersion->setText(APPLICATION_VERSION);
}

AboutDialog::~AboutDialog()
{
}
