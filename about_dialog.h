#pragma once
#include <QtWidgets/QDialog>
#include <QtGui/QIcon>
#include <memory>

class AboutDialog : public QDialog {
public:
    AboutDialog(const QIcon &icon, QWidget *parent = nullptr);
    ~AboutDialog();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
