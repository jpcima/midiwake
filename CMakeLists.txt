cmake_minimum_required(VERSION 3.14)
project(midiwake VERSION "0.0.0" LANGUAGES CXX)
set(APPLICATION_DISPLAY_NAME "MidiWake")
set(APPLICATION_BUNDLE_IDENTIFIER "org.sdf1.jpcima.MidiWake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(MIDIWAKE_QT Qt5 CACHE STRING "Which Qt package to build against")
set_property(CACHE MIDIWAKE_QT PROPERTY STRINGS "Qt5" "Qt6")

find_package(PkgConfig)
pkg_check_modules(alsa "alsa" REQUIRED IMPORTED_TARGET)
find_package(${MIDIWAKE_QT} COMPONENTS Widgets DBus Network Core REQUIRED)

###
add_library(QCtrlSignals STATIC EXCLUDE_FROM_ALL
  "thirdparty/QCtrlSignals/src/qctrlsignalhandler.cpp"
  "thirdparty/QCtrlSignals/src/qctrlsignalhandler.h"
  "thirdparty/QCtrlSignals/src/qctrlsignalhandler_p.h"
  "thirdparty/QCtrlSignals/src/QCtrlSignals")
if(WIN32)
  target_sources(QCtrlSignals PRIVATE
    "thirdparty/QCtrlSignals/src/qctrlsignalhandler_win.cpp"
    "thirdparty/QCtrlSignals/src/qctrlsignalhandler_win.h")
else()
  target_sources(QCtrlSignals PRIVATE
    "thirdparty/QCtrlSignals/src/qctrlsignalhandler_unix.cpp"
    "thirdparty/QCtrlSignals/src/qctrlsignalhandler_unix.h")
endif()
target_include_directories(QCtrlSignals PUBLIC
  "thirdparty/QCtrlSignals/src")
target_compile_definitions(QCtrlSignals PUBLIC
  "Q_CTRL_SIGNALS_EXPORT=")
target_link_libraries(QCtrlSignals PRIVATE
  ${MIDIWAKE_QT}::Core)

###
add_library(QSingleInstance STATIC EXCLUDE_FROM_ALL
  "thirdparty/QSingleInstance/QSingleInstance/clientinstance.cpp"
  "thirdparty/QSingleInstance/QSingleInstance/clientinstance.h"
  "thirdparty/QSingleInstance/QSingleInstance/qsingleinstance.cpp"
  "thirdparty/QSingleInstance/QSingleInstance/qsingleinstance.h"
  "thirdparty/QSingleInstance/QSingleInstance/qsingleinstance_p.cpp"
  "thirdparty/QSingleInstance/QSingleInstance/qsingleinstance_p.h")
target_include_directories(QSingleInstance PUBLIC
  "thirdparty/QSingleInstance/QSingleInstance")
target_link_libraries(QSingleInstance PRIVATE
  ${MIDIWAKE_QT}::Network
  ${MIDIWAKE_QT}::Core)

###
add_executable(midiwake
  "midiwake.cpp"
  "inhibitors.cpp"
  "inhibitors.h"
  "settings_dialog.cpp"
  "settings_dialog.h"
  "settings_dialog.ui"
  "about_dialog.cpp"
  "about_dialog.h"
  "about_dialog.ui"
  "resources.qrc")
target_compile_definitions(midiwake PRIVATE
  "APPLICATION_VERSION=\"${PROJECT_VERSION}\""
  "APPLICATION_NAME=\"${PROJECT_NAME}\""
  "APPLICATION_DISPLAY_NAME=\"${APPLICATION_DISPLAY_NAME}\""
  "APPLICATION_BUNDLE_IDENTIFIER=\"${APPLICATION_BUNDLE_IDENTIFIER}\"")
target_link_libraries(midiwake PRIVATE
  QCtrlSignals
  QSingleInstance
  ${MIDIWAKE_QT}::Widgets
  ${MIDIWAKE_QT}::DBus
  ${MIDIWAKE_QT}::Core
  PkgConfig::alsa)
install(TARGETS midiwake)
