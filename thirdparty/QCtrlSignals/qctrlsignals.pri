HEADERS += $$PWD/src/QCtrlSignals \
	$$PWD/src/qctrlsignalhandler.h \
	$$PWD/src/qctrlsignalhandler_p.h

SOURCES += \
	$$PWD/src/qctrlsignalhandler.cpp

win32 {
	HEADERS += $$PWD/src/qctrlsignalhandler_win.h
	SOURCES += $$PWD/src/qctrlsignalhandler_win.cpp
}

unix {
	HEADERS += $$PWD/src/qctrlsignalhandler_unix.h
	SOURCES += $$PWD/src/qctrlsignalhandler_unix.cpp
}

INCLUDEPATH += $$PWD/src

QDEP_PACKAGE_EXPORTS += Q_CTRL_SIGNALS_EXPORT
!qdep_build: DEFINES += "Q_CTRL_SIGNALS_EXPORT="
