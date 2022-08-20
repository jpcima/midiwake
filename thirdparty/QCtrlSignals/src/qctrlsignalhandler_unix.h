#ifndef QCTRLSIGNALHANDLER_UNIX_H
#define QCTRLSIGNALHANDLER_UNIX_H

#include "qctrlsignalhandler_p.h"

#include <QSocketNotifier>
#include <QObject>

class QCtrlSignalHandlerUnix : public QObject, public QCtrlSignalHandlerPrivate
{
	Q_OBJECT

public:
	QCtrlSignalHandlerUnix(QCtrlSignalHandler *q);

	bool registerSignal(int signal) override;
	bool unregisterSignal(int signal) override;
	void changeAutoQuitMode(bool enabled) override;
	QReadWriteLock *lock() const override;

private Q_SLOTS:
	void socketNotifyTriggerd(int socket);

private:
	QSocketNotifier *socketNotifier = nullptr;

	bool isAutoQuitRegistered(int signal) const;
	bool updateSignalHandler(int signal, bool active);

	static int pipepair[2];
	static const QVector<int> shutSignals;

	static void unixSignalHandler(int signal);
};

#endif // QCTRLSIGNALHANDLER_UNIX_H
