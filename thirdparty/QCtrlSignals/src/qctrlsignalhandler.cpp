#include "qctrlsignalhandler.h"
#include "qctrlsignalhandler_p.h"
#include <QGlobalStatic>
#include <qdebug.h>

Q_LOGGING_CATEGORY(logQCtrlSignals, "QCtrlSignals")

class QCtrlSignalHandlerInstance : public QCtrlSignalHandler {
public:
	inline QCtrlSignalHandlerInstance() :
		QCtrlSignalHandler{}
	{}
};
Q_GLOBAL_STATIC(QCtrlSignalHandlerInstance, handler)

QCtrlSignalHandler::QCtrlSignalHandler() :
	QObject{},
	d{QCtrlSignalHandlerPrivate::createInstance(this)}
{}

QCtrlSignalHandler::~QCtrlSignalHandler() = default;

QCtrlSignalHandler *QCtrlSignalHandler::instance()
{
	return handler;
}

bool QCtrlSignalHandler::registerForSignal(int signal)
{
	QWriteLocker lock{d->lock()};
	if(!d->activeSignals.contains(signal)) {
		if(d->registerSignal(signal)) {
			d->activeSignals.insert(signal);
			return true;
		} else
			return false;
	} else
		return true;
}

bool QCtrlSignalHandler::unregisterFromSignal(int signal)
{
	QWriteLocker lock{d->lock()};
	if(d->activeSignals.contains(signal)) {
		if(d->unregisterSignal(signal)) {
			d->activeSignals.remove(signal);
			return true;
		} else
			return false;
	} else
		return true;
}

bool QCtrlSignalHandler::isAutoQuitActive() const
{
	QReadLocker lock{d->lock()};
	return d->autoQuit;
}

void QCtrlSignalHandler::setAutoQuitActive(bool autoQuitActive)
{
	QWriteLocker lock{d->lock()};
	if (d->autoQuit == autoQuitActive)
		return;

	d->autoQuit = autoQuitActive;
	d->changeAutoQuitMode(autoQuitActive);
	emit autoQuitActiveChanged(autoQuitActive);
}



QCtrlSignalHandlerPrivate::QCtrlSignalHandlerPrivate(QCtrlSignalHandler *q_ptr) :
	q{q_ptr}
{}

QCtrlSignalHandlerPrivate::~QCtrlSignalHandlerPrivate() = default;

bool QCtrlSignalHandlerPrivate::reportSignalTriggered(int signal)
{
	//Not locked, because this method is called by the signal handler, which should do the lock, if required
	if(!activeSignals.contains(signal))
		return false;

	if(signal == QCtrlSignalHandler::SigInt)
		QMetaObject::invokeMethod(q, "sigInt", Qt::QueuedConnection);
	else if(signal == QCtrlSignalHandler::SigTerm)
		QMetaObject::invokeMethod(q, "sigTerm", Qt::QueuedConnection);

	return QMetaObject::invokeMethod(q, "ctrlSignal", Qt::QueuedConnection,
									 Q_ARG(int, signal));
}
