cmake_minimum_required(VERSION 3.14)
project(midiwake VERSION "0.0.0" LANGUAGES CXX)
set(APPLICATION_DISPLAY_NAME "MidiWake")
set(APPLICATION_BUNDLE_IDENTIFIER "org.sdf1.jpcima.MidiWake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig)
pkg_check_modules(alsa "alsa" REQUIRED IMPORTED_TARGET)
find_package(Qt5 COMPONENTS Widgets DBus Core REQUIRED)

###
add_library(QCtrlSignals STATIC
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
  Qt5::Core)

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
  Qt5::Widgets
  Qt5::DBus
  Qt5::Core
  PkgConfig::alsa)
install(TARGETS midiwake)
