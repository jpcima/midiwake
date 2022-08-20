#include "qctrlsignalhandler_unix.h"
#include <QCoreApplication>
#include <unistd.h>
#include <fcntl.h>

int QCtrlSignalHandlerUnix::pipepair[2];
const QVector<int> QCtrlSignalHandlerUnix::shutSignals = {SIGINT, SIGTERM, SIGQUIT, SIGHUP};

QCtrlSignalHandlerPrivate *QCtrlSignalHandlerPrivate::createInstance(QCtrlSignalHandler *q_ptr)
{
	return new QCtrlSignalHandlerUnix{q_ptr};
}

QCtrlSignalHandlerUnix::QCtrlSignalHandlerUnix(QCtrlSignalHandler *q_ptr) :
	QObject{},
	QCtrlSignalHandlerPrivate{q_ptr}
{
	if(::pipe2(pipepair, O_CLOEXEC) == 0) {
		socketNotifier = new QSocketNotifier{pipepair[0], QSocketNotifier::Read, this};
		connect(socketNotifier, &QSocketNotifier::activated,
				this, &QCtrlSignalHandlerUnix::socketNotifyTriggerd);
		socketNotifier->setEnabled(true);
	} else {
		qCWarning(logQCtrlSignals).noquote() << "Failed to create socket pair with error:"
											 << qt_error_string();
	}
}

bool QCtrlSignalHandlerUnix::registerSignal(int signal)
{
	if(isAutoQuitRegistered(signal))
		return true;
	else
		return updateSignalHandler(signal, true);
}

bool QCtrlSignalHandlerUnix::unregisterSignal(int signal)
{
	if(isAutoQuitRegistered(signal))
		return true;
	else
		return updateSignalHandler(signal, false);
}

void QCtrlSignalHandlerUnix::changeAutoQuitMode(bool enabled)
{
	for(auto sig : shutSignals) {
		if(!activeSignals.contains(sig))
			updateSignalHandler(sig, enabled);
	}
}

QReadWriteLock *QCtrlSignalHandlerUnix::lock() const
{
	return nullptr;//no locks needed on unix
}

void QCtrlSignalHandlerUnix::socketNotifyTriggerd(int socket)
{
	int signal;
	if(::read(socket, &signal, sizeof(int)) == sizeof(int)) {
		if(!reportSignalTriggered(signal) &&
		   isAutoQuitRegistered(signal))
			qApp->quit();
	} else
		qCWarning(logQCtrlSignals) << "Failed to read signal from socket pair";
}

bool QCtrlSignalHandlerUnix::isAutoQuitRegistered(int signal) const
{
	if(autoQuit)
		return shutSignals.contains(signal);
	else
		return false;
}

bool QCtrlSignalHandlerUnix::updateSignalHandler(int signal, bool active)
{
	struct sigaction action;
	action.sa_handler = active ? QCtrlSignalHandlerUnix::unixSignalHandler : SIG_DFL;
	sigemptyset(&action.sa_mask);
	//action.sa_flags |= SA_RESTART;
	if(::sigaction(signal, &action, nullptr) == 0)
		return true;
	else {
		qCWarning(logQCtrlSignals).noquote() << "Failed to"
											 << (active ? "register" : "unregister")
											 << "signal with error:"
											 << qt_error_string();
		return false;
	}
}

void QCtrlSignalHandlerUnix::unixSignalHandler(int signal)
{
	const auto wr = ::write(pipepair[1], &signal, sizeof(int));
	Q_UNUSED(wr);
}
