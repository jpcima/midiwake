cmake_minimum_required(VERSION 3.14)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" package_version)
list(GET package_version 0 package_version)

project(midiwake VERSION "${package_version}" LANGUAGES CXX)
set(APPLICATION_DISPLAY_NAME "MidiWake")
set(APPLICATION_BUNDLE_IDENTIFIER "org.sdf1.jpcima.MidiWake")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SaneCompilerOptions)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(MIDIWAKE_QT Qt5 CACHE STRING "Which Qt package to build against")
set_property(CACHE MIDIWAKE_QT PROPERTY STRINGS "Qt5" "Qt6")

find_package(PkgConfig)
pkg_check_modules(alsa "alsa" REQUIRED IMPORTED_TARGET)
find_package(${MIDIWAKE_QT} COMPONENTS Widgets DBus Network Core LinguistTools REQUIRED)

include(GNUInstallDirs)

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
  "sources/midiwake.cpp"
  "sources/midiwake.h"
  "sources/inhibitors.cpp"
  "sources/inhibitors.h"
  "sources/settings_dialog.cpp"
  "sources/settings_dialog.h"
  "sources/settings_dialog.ui"
  "sources/about_dialog.cpp"
  "sources/about_dialog.h"
  "sources/about_dialog.ui"
  "resources/resources.qrc")
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
  PkgConfig::alsa
  sane-warning-flags)
if(MIDIWAKE_QT STREQUAL "Qt5")
  qt5_create_translation(MIDIWAKE_QM_FILES "sources"
    "resources/i18n/midiwake_fr.ts")
else()
  qt_create_translation(MIDIWAKE_QM_FILES "sources"
    "resources/i18n/midiwake_fr.ts")
endif()
#
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/midiwake_i18n.qrc" "<RCC><qresource prefix=\"/i18n/\">")
foreach(_qm IN LISTS MIDIWAKE_QM_FILES)
  get_filename_component(_baseqm "${_qm}" NAME)
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/midiwake_i18n.qrc" "<file>${_baseqm}</file>")
endforeach()
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/midiwake_i18n.qrc" "</qresource></RCC>")
#
target_sources(midiwake PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/midiwake_i18n.qrc"
  ${MIDIWAKE_QM_FILES})
install(TARGETS midiwake DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "midiwake.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES
  "resources/icons/icon.png"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
  RENAME "midiwake.png")
