#include "qctrlsignalhandler_win.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QDebug>

namespace {
static QSemaphore shutdownLock;
}

QCtrlSignalHandlerPrivate *QCtrlSignalHandlerPrivate::createInstance(QCtrlSignalHandler *q_ptr)
{
	return new QCtrlSignalHandlerWin{q_ptr};
}

QCtrlSignalHandlerWin::QCtrlSignalHandlerWin(QCtrlSignalHandler *q_ptr) :
	QCtrlSignalHandlerPrivate{q_ptr},
	rwLock{QReadWriteLock::Recursive}
{
#ifndef Q_OS_WINRT
	if(!::SetConsoleCtrlHandler(HandlerRoutine, true)) {
		qCWarning(logQCtrlSignals).noquote()
			<< "Failed to create signal handler with error:"
			<< qt_error_string();
	}
#else
	qCWarning(logQCtrlSignals) << "Signal handlers are not supported on WINRT";
#endif
}

QCtrlSignalHandlerWin::~QCtrlSignalHandlerWin()
{
#ifndef Q_OS_WINRT
	if(!::SetConsoleCtrlHandler(HandlerRoutine, false)) {
		qCWarning(logQCtrlSignals).noquote()
				<< "Failed to remove signal handler with error:"
				<< qt_error_string();
	}
#endif
}

bool QCtrlSignalHandlerWin::registerSignal(int signal)
{
	switch (signal) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		return true;
	default:
		return false;
	}
}

bool QCtrlSignalHandlerWin::unregisterSignal(int)
{
	return true;
}

void QCtrlSignalHandlerWin::changeAutoQuitMode(bool) {}

QReadWriteLock *QCtrlSignalHandlerWin::lock() const
{
	return &rwLock;
}

bool QCtrlSignalHandlerWin::handleAutoQuit(DWORD signal)
{
	if(!autoQuit)
		return false;

	switch (signal) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
		return true;
	case CTRL_CLOSE_EVENT:
		QObject::connect(qApp, &QCoreApplication::destroyed, qApp, [&](){//will be executed in the main thread
			shutdownLock.release();
			QThread::msleep(250);//give the killer thread time to exit the process
		}, Qt::DirectConnection);
		QMetaObject::invokeMethod(qApp, "quit", Qt::BlockingQueuedConnection);
		shutdownLock.acquire();
#ifndef Q_OS_WINRT
		::ExitProcess(EXIT_SUCCESS);
#endif
		return true;
	default:
		return false;
	}
}

BOOL QCtrlSignalHandlerWin::HandlerRoutine(DWORD dwCtrlType)
{
	auto self = p_instance<QCtrlSignalHandlerWin>();
	QReadLocker lock(self->lock());
	if(self->reportSignalTriggered((int)dwCtrlType))
		return TRUE;
	else
		return self->handleAutoQuit(dwCtrlType);
}
