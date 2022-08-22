#include "midiwake.h"
#include "inhibitors.h"
#include "settings_dialog.h"
#include "about_dialog.h"
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtCore/QSocketNotifier>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QLibraryInfo>
#include <QCtrlSignals>
#include <qsingleinstance.h>
#include <vector>
#include <cstdio>

//------------------------------------------------------------------------------
bool Application::init()
{
    QTranslator *translatorApp = new QTranslator(this);
    if (translatorApp->load(QLocale(), "midiwake", "_", ":/i18n")) {
        installTranslator(translatorApp);
        QTranslator *translatorQt = new QTranslator(this);
        if (translatorQt->load(QLocale(), "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(translatorQt);
    }

    setQuitOnLastWindowClosed(false);

    setApplicationVersion(APPLICATION_VERSION);
    setApplicationName(APPLICATION_NAME);
    setApplicationDisplayName(APPLICATION_DISPLAY_NAME);

    QSettings *settings = new QSettings;
    m_settings = settings;

    if (!settings->contains("wake-duration"))
        settings->setValue("wake-duration", 15);

    m_inhibitor = Inhibitor::createInhibitor(this);
    if (m_inhibitor->isValid())
        fprintf(stderr, "Using screen saver inhibitor: %s\n",
                m_inhibitor->getIdentifier().toUtf8().data());
    else
        fprintf(stderr, "No screen saver inhibitor is available.\n");

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

    m_mainIcon = QIcon(":/icons/icon.png");
    m_activeIcon = QIcon(":/icons/icon-active.png");

    setWindowIcon(m_mainIcon);

    QSystemTrayIcon *trayIcon = new QSystemTrayIcon;
    m_trayIcon = trayIcon;
    trayIcon->setIcon(m_mainIcon);
    trayIcon->show();

    QMenu *trayMenu = new QMenu;
    trayIcon->setContextMenu(trayMenu);
    QAction *settingsAction = trayMenu->addAction(QIcon::fromTheme("preferences-system"), tr("Settings"));
    QAction *aboutAction = trayMenu->addAction(QIcon::fromTheme("help-about"), tr("About"));
    QAction *quitAction = trayMenu->addAction(QIcon::fromTheme("application-exit"), tr("Quit"));
    QObject::connect(settingsAction, &QAction::triggered, this, &Application::openSettingsDialog);
    QObject::connect(aboutAction, &QAction::triggered, this, &Application::openAboutDialog);
    QObject::connect(quitAction, &QAction::triggered, this, &QCoreApplication::quit);

    updateStatusDisplay(false);

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

    fprintf(stderr, "Registered ALSA client.\n");

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

    fprintf(stderr, "Created client port: %d:%d\n",
            snd_seq_port_info_get_client(portInfo), snd_seq_port_info_get_port(portInfo));

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    const snd_seq_addr_t src{SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE};
    snd_seq_port_subscribe_set_sender(sub, &src);
    snd_seq_port_subscribe_set_dest(sub, snd_seq_port_info_get_addr(portInfo));
    if (snd_seq_subscribe_port(seq, sub) < 0) {
        fprintf(stderr, "Cannot subscribe to the system port.\n");
        return false;
    }

    fprintf(stderr, "Subscribed to system announcements.\n");

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
    fprintf(stderr, "Quitting\n");

    setInhibited(false);
    m_deinhibitTimer->stop();

    for (QSocketNotifier *notifier : m_notifiers)
        notifier->deleteLater();
    m_notifiers.clear();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
void Application::sequencerNotified(int fd)
#else
void Application::sequencerNotified(QSocketDescriptor fd, QSocketNotifier::Type type)
#endif
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
                fprintf(stderr, "New client has arrived: %d\n", clientId);
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
        fprintf(stderr, "Cannot get client information: %d\n", clientId);
        return;
    }

    fprintf(stderr, "Processing client: (%d) %s\n",
            clientId, snd_seq_client_info_get_name(clientInfo));

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

        fprintf(stderr, "Found hardware output: %d:%d\n", clientId, portId);

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

        fprintf(stderr, "Subscribed hardware output: %d:%d\n", clientId, portId);
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

    updateStatusDisplay(inh);

    if (inh) {
        QString reason = tr("Using hardware MIDI.");
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
        dlg = new SettingsDialog(m_settings, isAutoStartFilePresent());
        m_settingsDialog = dlg;

        QObject::connect(
            dlg, &SettingsDialog::wakeDurationChanged,
            this, [this](int value) { m_deinhibitTimer->setInterval(1000 * 60 * value); });

        QObject::connect(
            dlg, &SettingsDialog::autostartChanged,
            this, [this](bool value) { if (value) installAutoStartFile(); else removeAutoStartFile(); });
    }

    dlg->show();
}

void Application::openAboutDialog()
{
    AboutDialog *dlg = m_aboutDialog;

    if (!dlg) {
        dlg = new AboutDialog(m_mainIcon);
        m_aboutDialog = dlg;
    }

    dlg->show();
}

void Application::updateStatusDisplay(bool active)
{
    if (active) {
        m_trayIcon->setToolTip(tr("%1 status: active").arg(APPLICATION_DISPLAY_NAME));
        m_trayIcon->setIcon(m_activeIcon);
    }
    else {
        m_trayIcon->setToolTip(tr("%1 status: inactive").arg(APPLICATION_DISPLAY_NAME));
        m_trayIcon->setIcon(m_mainIcon);
    }
}

QString Application::getAutoStartFileLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (!path.isEmpty())
        path.append("/autostart/" APPLICATION_BUNDLE_IDENTIFIER ".desktop");
    return path;
}

bool Application::isAutoStartFilePresent()
{
    return QFile(getAutoStartFileLocation()).exists();
}

void Application::installAutoStartFile()
{
    QString path = getAutoStartFileLocation();
    QFileInfo(path).dir().mkpath(".");

    const char contents[] =
"[Desktop Entry]\n"
"Name=" APPLICATION_DISPLAY_NAME "\n"
"Type=Application\n"
"Exec=" APPLICATION_NAME "\n"
"Terminal=false\n";
    size_t length = sizeof(contents) - 1;

    QFile file(path);
    if (!file.open(QFile::WriteOnly)) {
        fprintf(stderr, "Could not open the autostart file for writing.\n");
        return;
    }

    if (file.write(contents, length) != (qint64)length || !file.flush()) {
        fprintf(stderr, "Could not write the autostart file.\n");
        QFile::remove(path);
        return;
    }
}

void Application::removeAutoStartFile()
{
    QString path = getAutoStartFileLocation();
    QFile::remove(path);
}

//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Application app(argc, argv);
    QSingleInstance instance;

    instance.setStartupFunction([&app]() -> int {
        if (!app.init())
            return 1;
        return 0;
    });

    int ret = instance.singleExec();
    return ret;
}
