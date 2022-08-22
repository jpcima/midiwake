#include "about_dialog.h"
#include "ui_about_dialog.h"

struct AboutDialog::Impl {
    Ui::AboutDialog ui;
};

AboutDialog::AboutDialog(const QIcon &icon, QWidget *parent)
    : QDialog(parent), m_impl(new Impl)
{
    Ui::AboutDialog &ui = m_impl->ui;
    ui.setupUi(this);

    ui.txtDisplayName->setText(APPLICATION_DISPLAY_NAME);
    ui.txtVersion->setText(tr("Version %1").arg(APPLICATION_VERSION));

    ui.lblIcon->setPixmap(icon.pixmap(ui.lblIcon->size()));
}

AboutDialog::~AboutDialog()
{
}
