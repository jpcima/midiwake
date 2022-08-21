#pragma once
#include <QtWidgets/QDialog>
#include <memory>

class AboutDialog : public QDialog {
public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
