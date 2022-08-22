#include "inhibitors.h"
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtCore/QCoreApplication>

class GenericDbusInhibitor : public Inhibitor {
public:
    GenericDbusInhibitor(const QString &service, const QString &path, QObject *parent);
    void inhibit(const QString &reason) override;
    void uninhibit() override;
    bool isValid() override;
    QString getIdentifier() const override;

private:
    qint64 m_cookie = -1;
    QString m_service;
    QString m_path;
};

GenericDbusInhibitor::GenericDbusInhibitor(const QString &service, const QString &path, QObject *parent)
    : Inhibitor(parent), m_service(service), m_path(path)
{
}

void GenericDbusInhibitor::inhibit(const QString &reason)
{
    if (m_cookie == -1) {
        QDBusInterface interface(m_service, m_path, m_service);
        if (interface.isValid()) {
            QDBusReply<quint32> reply = interface.call("Inhibit", APPLICATION_BUNDLE_IDENTIFIER, reason);
            if (reply.isValid())
                m_cookie = reply.value();
        }
    }
}

void GenericDbusInhibitor::uninhibit()
{
    if (m_cookie != -1) {
        QDBusInterface interface(m_service, m_path, m_service);
        if (interface.isValid()) {
            interface.call("UnInhibit", (quint32)m_cookie);
            m_cookie = -1;
        }
    }
}

bool GenericDbusInhibitor::isValid()
{
    QDBusInterface interface(m_service, m_path, m_service);
    return interface.isValid();
}

QString GenericDbusInhibitor::getIdentifier() const
{
    return m_service;
}

//------------------------------------------------------------------------------
class GnomeInhibitor : public Inhibitor {
public:
    using Inhibitor::Inhibitor;

    void inhibit(const QString &reason) override;
    void uninhibit() override;
    bool isValid() override;
    QString getIdentifier() const override;

private:
    qint64 m_cookie = -1;
};

void GnomeInhibitor::inhibit(const QString &reason)
{
    if (m_cookie == -1) {
        QDBusInterface gnome(
            "org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager");
        if (gnome.isValid()) {
            quint32 flags = 8; // 8: Inhibit the session being marked as idle
            quint32 toplevel_xid = 0;
            QDBusReply<quint32> reply = gnome.call("Inhibit", APPLICATION_BUNDLE_IDENTIFIER, toplevel_xid, reason, flags);
            if (reply.isValid())
                m_cookie = reply.value();
        }
    }
}

void GnomeInhibitor::uninhibit()
{
    if (m_cookie != -1) {
        QDBusInterface gnome(
            "org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager");
        if (gnome.isValid()) {
            gnome.call("Uninhibit", (quint32)m_cookie);
            m_cookie = -1;
        }
    }
}

bool GnomeInhibitor::isValid()
{
    QDBusInterface gnome(
        "org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager");
    return gnome.isValid();
}

QString GnomeInhibitor::getIdentifier() const
{
    return "org.gnome.SessionManager";
}

//------------------------------------------------------------------------------
class NullInhibitor : public Inhibitor {
public:
    using Inhibitor::Inhibitor;

    void inhibit(const QString &reason) override;
    void uninhibit() override;
    bool isValid() override;
    QString getIdentifier() const override;
};

void NullInhibitor::inhibit(const QString &reason)
{
    (void)reason;
}

void NullInhibitor::uninhibit()
{
}

bool NullInhibitor::isValid()
{
    return false;
}

QString NullInhibitor::getIdentifier() const
{
    return "Null";
}

//------------------------------------------------------------------------------
Inhibitor *Inhibitor::createInhibitor(QObject *parent)
{
    Inhibitor *inh = nullptr;

    static const std::pair<const char *, const char *> generics[] = {
        {"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver"},
        {"org.xfce.ScreenSaver", "/org/xfce/ScreenSaver"},
        {"org.cinnamon.ScreenSaver", "/org/cinnamon/ScreenSaver"},
        {"org.mate.ScreenSaver", "/org/mate/ScreenSaver"},
    };

    for (const std::pair<const char *, const char *> &gi : generics) {
        QString service = gi.first;
        QString path = gi.second;
        inh = new GenericDbusInhibitor(service, path, parent);
        if (inh->isValid())
            return inh;
        delete inh;
        inh = nullptr;
    }

    inh = new GnomeInhibitor(parent);
    if (inh->isValid())
        return inh;
    delete inh;
    inh = nullptr;

    inh = new NullInhibitor(parent);
    return inh;
}
