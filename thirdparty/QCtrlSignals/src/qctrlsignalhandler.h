#ifndef QCTRLSIGNALHANDLER_H
#define QCTRLSIGNALHANDLER_H

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QLoggingCategory>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <csignal>
#endif

class QCtrlSignalHandlerPrivate;
//! A class to listen for "CTRL-Signals"
class Q_CTRL_SIGNALS_EXPORT QCtrlSignalHandler : public QObject
{
	Q_OBJECT
	friend class QCtrlSignalHandlerPrivate;
	friend class QCtrlSignalHandlerInstance;

	//! Specifies, whether the handler should automatically handle signals that "quit" the app
	Q_PROPERTY(bool autoQuitActive READ isAutoQuitActive WRITE setAutoQuitActive NOTIFY autoQuitActiveChanged)

public:
	//! Common signals, that are available on all platforms
	enum CommonSignals {
#ifdef Q_OS_WIN
		SigInt = CTRL_C_EVENT,
		SigTerm = CTRL_BREAK_EVENT
#else
		SigInt = SIGINT,//!< Mapped to `CTRL_C_EVENT` on windows and `SIGINT` on unix
		SigTerm = SIGTERM//!< Mapped to `CTRL_BREAK_EVENT` on windows and `SIGTERM` on unix
#endif
	};

	//! Returns the singleton instance of the signal handler
	static QCtrlSignalHandler *instance();

	//! Registers this handler for the given signal
	bool registerForSignal(int signal);
	//! Unregisters this handler from the given signal
	bool unregisterFromSignal(int signal);

	//! READ-Accessor for QCtrlSignalHandler::autoQuitActive
	bool isAutoQuitActive() const;

public Q_SLOTS:
	//! WRITE-Accessor for QCtrlSignalHandler::autoQuitActive
	void setAutoQuitActive(bool autoQuitActive);

Q_SIGNALS:
	//! Shortcut signal for QCtrlSignalHandler::SigInt
	void sigInt();
	//! Shortcut signal for QCtrlSignalHandler::SigTerm
	void sigTerm();

	//! Will be emitted if a registered signal occures
	void ctrlSignal(int signal);

	//! NOTIFY-Accessor for QCtrlSignalHandler::autoQuitActive
	void autoQuitActiveChanged(bool autoQuitActive);

private:
	QScopedPointer<QCtrlSignalHandlerPrivate> d;

	explicit QCtrlSignalHandler();
	~QCtrlSignalHandler() override;
};

Q_DECLARE_LOGGING_CATEGORY(logQCtrlSignals)

#endif // QCTRLSIGNALHANDLER_H
