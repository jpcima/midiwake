#pragma once
#include <QtWidgets/QDialog>
#include <QtCore/QSettings>
#include <memory>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(QSettings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();

signals:
    void wakeDurationChanged(int value);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
