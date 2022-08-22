#pragma once
#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtGui/QIcon>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QTranslator>
#include <QtCore/QSocketNotifier>
#include <alsa/asoundlib.h>
#include <memory>
class Inhibitor;
class SettingsDialog;
class AboutDialog;

#ifndef DEFINE_AUTO_PTR
#define DEFINE_AUTO_PTR(aptr, styp, freefn)                      \
    struct aptr##_deleter {                                      \
        void operator()(styp *x) const noexcept { (freefn)(x); } \
    }; using aptr = std::unique_ptr<styp, aptr##_deleter>
#endif

//------------------------------------------------------------------------------
DEFINE_AUTO_PTR(snd_seq_u, snd_seq_t, snd_seq_close);

//------------------------------------------------------------------------------
class Application : public QApplication {
    Q_OBJECT

public:
    using QApplication::QApplication;

    bool init();

private:
    bool initAlsaMidi();
    void onQuit();
    void processNewAlsaClient(int clientId);
    void processEventFromHardware(snd_seq_event_t *event);
    void setInhibited(bool inh);
    void openSettingsDialog();
    void openAboutDialog();
    void updateStatusDisplay(bool active);
    static QString getAutoStartFileLocation();
    static bool isAutoStartFilePresent();
    static void installAutoStartFile();
    static void removeAutoStartFile();

protected:
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    void sequencerNotified(int fd);
#else
    void sequencerNotified(QSocketDescriptor fd, QSocketNotifier::Type type);
#endif

private:
    snd_seq_u m_seq;
    int m_port = -1;
    bool m_inhibit = false;
    Inhibitor *m_inhibitor = nullptr;
    std::vector<QSocketNotifier *> m_notifiers;
    QTimer *m_deinhibitTimer = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;
    AboutDialog *m_aboutDialog = nullptr;
    QSettings *m_settings = nullptr;
    QIcon m_mainIcon;
    QIcon m_activeIcon;
};
