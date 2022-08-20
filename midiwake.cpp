#include "inhibitors.h"
#include "settings_dialog.h"
#include <alsa/asoundlib.h>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QCtrlSignals>
#include <vector>
#include <memory>
#include <cstdio>

#define DEFINE_AUTO_PTR(aptr, styp, freefn)                      \
    struct aptr##_deleter {                                      \
        void operator()(styp *x) const noexcept { (freefn)(x); } \
    }; using aptr = std::unique_ptr<styp, aptr##_deleter>

//------------------------------------------------------------------------------
DEFINE_AUTO_PTR(snd_seq_u, snd_seq_t, snd_seq_close);

//------------------------------------------------------------------------------
class Application : public QApplication {
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

protected:
    void sequencerNotified(QSocketDescriptor fd, QSocketNotifier::Type type);

private:
    snd_seq_u m_seq;
    int m_port = -1;
    bool m_inhibit = false;
    Inhibitor *m_inhibitor = nullptr;
    std::vector<QSocketNotifier *> m_notifiers;
    QTimer *m_deinhibitTimer = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;
    QSettings *m_settings = nullptr;
};

bool Application::init()
{
    setQuitOnLastWindowClosed(false);

    setApplicationVersion(APPLICATION_VERSION);
    setApplicationName(APPLICATION_NAME);
    setApplicationDisplayName(APPLICATION_DISPLAY_NAME);

    QSettings *settings = new QSettings;
    m_settings = settings;

    if (!settings->contains("wake-duration"))
        settings->setValue("wake-duration", 15);

    m_inhibitor = Inhibitor::createInhibitor(this);
    //fprintf(stderr, "Inhibitor: %s\n", m_inhibitor->getIdentifier().toUtf8().data());
    if (!m_inhibitor->isValid()) {
        fprintf(stderr, "No screen saver inhibitor is available.\n");
    }

    m_deinhibitTimer = new QTimer(this);
    m_deinhibitTimer->setInterval(1000 * 60 * settings->value("wake-duration").toInt());
    m_deinhibitTimer->setSingleShot(true);
    QObject::connect(m_deinhibitTimer, &QTimer::timeout, this, [this]() {
        setInhibited(false);
    });

    QObject::connect(this, &QCoreApplication::aboutToQuit, this, &Application::onQuit);

    QCtrlSignalHandler *termHandler = QCtrlSignalHandler::instance();
    termHandler->setAutoQuitActive(true);

    if (!initAlsaMidi()) {
        QMessageBox::warning(nullptr, tr("Error"), tr("Could not initialize ALSA MIDI."));
        return false;
    }

    QSystemTrayIcon *trayIcon = new QSystemTrayIcon;
    m_trayIcon = trayIcon;
    trayIcon->setIcon(QIcon(":/resources/icon.png"));
    trayIcon->show();

    QMenu *trayMenu = new QMenu;
    trayIcon->setContextMenu(trayMenu);
    QAction *settingsAction = trayMenu->addAction(tr("Settings"));
    QAction *quitAction = trayMenu->addAction(tr("Quit"));
    QObject::connect(settingsAction, &QAction::triggered, this, &Application::openSettingsDialog);
    QObject::connect(quitAction, &QAction::triggered, this, &QCoreApplication::quit);

    return true;
}

bool Application::initAlsaMidi()
{
    snd_seq_t *seq = nullptr;

    int ret = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (ret != 0) {
        fprintf(stderr, "Cannot open the sequencer.");
        return false;
    }

    m_seq.reset(seq);

    int pollType = POLLIN;
    QSocketNotifier::Type notifyType = QSocketNotifier::Read;

    int nfds = snd_seq_poll_descriptors_count(seq, pollType);
    if (nfds < 0) {
        fprintf(stderr, "Cannot get the number of poll descriptors.\n");
        return false;
    }

    std::vector<pollfd> pfds((size_t)nfds);
    nfds = snd_seq_poll_descriptors(seq, pfds.data(), nfds, pollType);
    if (nfds < 0) {
        fprintf(stderr, "Cannot get the list of poll descriptors.\n");
        return false;
    }

    for (int i = 0; i < nfds; ++i) {
        int fd = pfds[i].fd;
        QSocketNotifier *notifier = new QSocketNotifier(fd, notifyType, this);
        m_notifiers.push_back(notifier);
        connect(notifier, &QSocketNotifier::activated, this, &Application::sequencerNotified);
    }

    if (snd_seq_set_client_name(seq, applicationDisplayName().toStdString().c_str())) {
        fprintf(stderr, "Cannot set the client name.\n");
        return false;
    }

    int port = snd_seq_create_simple_port(
        seq, "Input",
        SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0) {
        fprintf(stderr, "Cannot create the port.\n");
        return false;
    }
    m_port = port;

    snd_seq_port_info_t *portInfo = nullptr;
    snd_seq_port_info_alloca(&portInfo);
    if (snd_seq_get_port_info(seq, port, portInfo) < 0) {
        fprintf(stderr, "Cannot get the port information.\n");
        return false;
    }

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    const snd_seq_addr_t src{SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE};
    snd_seq_port_subscribe_set_sender(sub, &src);
    snd_seq_port_subscribe_set_dest(sub, snd_seq_port_info_get_addr(portInfo));
    if (snd_seq_subscribe_port(seq, sub) < 0) {
        fprintf(stderr, "Cannot subscribe to the system port.\n");
        return false;
    }

    snd_seq_client_info_t *clientInfo = nullptr;
    snd_seq_client_info_alloca(&clientInfo);
    while (snd_seq_query_next_client(seq, clientInfo) >= 0) {
        int clientId = snd_seq_client_info_get_client(clientInfo);
        processNewAlsaClient(clientId);
    }

    return true;
}

void Application::onQuit()
{
    //fprintf(stderr, "Quitting...\n");

    setInhibited(false);
    m_deinhibitTimer->stop();

    for (QSocketNotifier *notifier : m_notifiers)
        notifier->deleteLater();
    m_notifiers.clear();
}

void Application::sequencerNotified(QSocketDescriptor fd, QSocketNotifier::Type type)
{
    snd_seq_t *seq = m_seq.get();
    snd_seq_event_t *ev = nullptr;

    int nb;
    do {
        nb = snd_seq_event_input(seq, &ev);
        if (nb < 0)
            break;

        if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
            ev->source.port == SND_SEQ_PORT_SYSTEM_ANNOUNCE)
        {
            switch (ev->type) {
            case SND_SEQ_EVENT_CLIENT_START: {
                int clientId = ev->data.addr.client;
                processNewAlsaClient(clientId);
                break;
            }
            }
        }
        else {
            processEventFromHardware(ev);
        }
    } while (nb > 0);
}

void Application::processNewAlsaClient(int clientId)
{
    //fprintf(stderr, "New ALSA client: %d\n", clientId);

    snd_seq_t *seq = m_seq.get();

    snd_seq_client_info_t *clientInfo = nullptr;
    snd_seq_client_info_alloca(&clientInfo);

    if (snd_seq_get_any_client_info(seq, clientId, clientInfo) < 0) {
        fprintf(stderr, "Cannot get client information: %d.\n", clientId);
        return;
    }

    snd_seq_port_info_t *portInfo = nullptr;
    snd_seq_port_info_alloca(&portInfo);
    snd_seq_port_info_set_client(portInfo, clientId);
    snd_seq_port_info_set_port(portInfo, -1);

    while (snd_seq_query_next_port(seq, portInfo) >= 0) {
        int portId = snd_seq_port_info_get_port(portInfo);

        // port must be readable
        int portCap = snd_seq_port_info_get_capability(portInfo);
        int portCapWant = SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_READ;
        if ((portCap & portCapWant) != portCapWant) {
            //fprintf(stderr, "Not a readable port: %d:%d \"%s\"\n", clientId, portId, snd_seq_port_info_get_name(portInfo));
            continue;
        }

        // port must be hardware
        int portType = snd_seq_port_info_get_type(portInfo);
        int portTypeWant = SND_SEQ_PORT_TYPE_HARDWARE;
        if ((portType & portTypeWant) != portTypeWant) {
            //fprintf(stderr, "Not a hardware port: %d:%d \"%s\"\n", clientId, portId, snd_seq_port_info_get_name(portInfo));
            continue;
        }

        // subscribe this port we found

        //fprintf(stderr, "Found port %d:%d\n", clientId, portId);

        snd_seq_port_subscribe_t *sub = nullptr;
        snd_seq_port_subscribe_alloca(&sub);

        snd_seq_addr_t src{};
        src.client = clientId;
        src.port = portId;
        snd_seq_addr_t dst{};
        dst.client = snd_seq_client_id(seq);
        dst.port = m_port;

        snd_seq_port_subscribe_set_sender(sub, &src);
        snd_seq_port_subscribe_set_dest(sub, &dst);
        if (snd_seq_subscribe_port(seq, sub) < 0)
            fprintf(stderr, "Cannot subscribe to the hardware port.\n");
    }
}

void Application::processEventFromHardware(snd_seq_event_t *event)
{
    if (event->type == SND_SEQ_EVENT_CLOCK || event->type == SND_SEQ_EVENT_SENSING) {
        // inactive devices can send these, ignore
        return;
    }

    setInhibited(true);

    m_deinhibitTimer->start();
}

void Application::setInhibited(bool inh)
{
    if (m_inhibit == inh)
        return;

    if (inh) {
        QString reason = tr("Playing hardware MIDI.");
        m_inhibitor->inhibit(reason);
    }
    else {
        m_inhibitor->uninhibit();
    }

    m_inhibit = inh;
}

void Application::openSettingsDialog()
{
    SettingsDialog *dlg = m_settingsDialog;

    if (!dlg) {
        dlg = new SettingsDialog(m_settings);
        m_settingsDialog = dlg;

        QObject::connect(
            dlg, &SettingsDialog::wakeDurationChanged,
            this, [this](int value) { m_deinhibitTimer->setInterval(1000 * 60 * value); });
    }

    dlg->show();
}

//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Application app(argc, argv);

    if (!app.init())
        return 1;

    int ret = app.exec();
    return ret;
}
