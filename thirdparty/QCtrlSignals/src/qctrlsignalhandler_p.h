#ifndef QCTRLSIGNALHANDLERPRIVATE_H
#define QCTRLSIGNALHANDLERPRIVATE_H

#include "qctrlsignalhandler.h"

#include <QtCore/QHash>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSet>

class QCtrlSignalHandlerPrivate
{
	Q_DISABLE_COPY(QCtrlSignalHandlerPrivate)

public:
	static QCtrlSignalHandlerPrivate *createInstance(QCtrlSignalHandler *q);

	virtual ~QCtrlSignalHandlerPrivate();

	virtual bool registerSignal(int signal) = 0;
	virtual bool unregisterSignal(int signal) = 0;

	virtual void changeAutoQuitMode(bool enabled) = 0;

	QSet<int> activeSignals;
	bool autoQuit = false;

	virtual QReadWriteLock *lock() const = 0;

protected:
	QCtrlSignalHandlerPrivate(QCtrlSignalHandler *q);

	template <typename T>
	static T *p_instance() {
		return static_cast<T*>(QCtrlSignalHandler::instance()->d.data());
	}

	bool reportSignalTriggered(int signal);

	QCtrlSignalHandler *q;
};

#endif // QCTRLSIGNALHANDLERPRIVATE_H
