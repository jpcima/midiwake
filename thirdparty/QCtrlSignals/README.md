# QCtrlSignals
A library to catch and handle windows and unix signals, in a cross platform manner.

With this class, you can easily register for the Operation system signals (like `CTRL_C_EVENT` on windows and `SIGINT` on unix/linux) and use them just like normal Qt signals.

## Features
- Register for common signals via a Cross-Platform enum
- You can register native signals as well
- Asynchronous signal handling. The class emits a Qt signal, and you can connect to this one just like to any other Qt signal
- Automated Quit:
	- The QCtrlSignalHandler automatically registers for typical "quit" signals (like `SIGINT`, `SIGTERN`, ...)
	- It automatically handles those and then calles `qApp->quit();`
	- Windows: Can handle the `CTRL_CLOSE_EVENT`, which cannot be catched with the handler otherwise

## Installation
The package is provided via qdep, as `Skycoder42/QCtrlSignals`. To use it simply:

1. Install and enable qdep (See [qdep - Installing](https://github.com/Skycoder42/qdep#installation))
2. Add the following to your pro file:
```qmake
QDEP_DEPENDS += Skycoder42/QCtrlSignals
!load(qdep):error("Failed to load qdep feature! Run 'qdep.py prfgen --qmake $$QMAKE_QMAKE' to create it.")
```

## Usage
All you have to do is to get the signal handler instance via `QCtrlSignalHandler::instance()` and register your signals. See the example below

### Example
The following exaple uses the handler to:
1. Automatically quit the application (for example by using `CTRL_BREAK_EVENT` or `SIGQUIT`)
2. Instead if quitting, `SigInt` (`CTRL_C_EVENT`, `SIGINT`) will print a message

```cpp
#include <QCoreApplication>
#include <QCtrlSignals>
#include <QDebug>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	auto handler = QCtrlSignalHandler::instance();

	QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [](){
		qDebug() << "App about to quit!";
	});
	handler->setAutoQuitActive(true);

	qDebug() << "SigInt" << handler->registerForSignal(QCtrlSignalHandler::SigInt);
	QObject::connect(handler, &QCtrlSignalHandler::sigInt, qApp, [](){
		qDebug() << "SigInt triggered, shutdown was overwritten for this one!";
	});

	return a.exec();
}
```

### Logging
By default, QCtrlSignals prints some warning messages if something goes wrong (For example, a signal cannot be registered). All messages of QCtrlSignals are grouped into the QLoggingCategory "QCtrlSignals". If you want to simply disable the logging, call the folling function somewhere in your code:
```cpp
QLoggingCategory::setFilterRules(QStringLiteral("QCtrlSignals.warning=false"));
```
For more information about all the things you can do with the logging categories, check the Qt-Documentation

## Documentation
The documentation is available as release and on [github pages](https://skycoder42.github.io/QCtrlSignals/).

The documentation was created using [doxygen](http://www.doxygen.org). It includes an HTML-documentation and Qt-Help files that can be included into QtCreator (QtAssistant) to show F1-Help (See [Adding External Documentation](https://doc.qt.io/qtcreator/creator-help.html#adding-external-documentation) for more details).

### Known Limitations
- Only such signals, that the operating system allows, can be registered. For example, `SIGKILL` on linux cannot be registered
- On Windows, all signals are handeled in a second thread. Thus, the signals that require immediate handling, like `CTRL_CLOSE_EVENT`, cannot be registered
  - However, this signal is handeled for automatic shutdown
