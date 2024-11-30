project(RegExp)
set(APP_VERSION "1.0")

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

find_package(Qt5)

include_directories(
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
link_directories(${QT_LIBRARY_DIR})

add_definitions(-D_UNICODE)

QT5_WRAP_CPP(RegExp_MOC_SRCS
    regexpdialog.h
)

QT5_WRAP_UI(RegExp_UIC_HDRS
    regexpdialog.ui
)

SET(RegExp_SRCS
    ${RegExp_UIC_HDRS}
    ${RegExp_MOC_SRCS}
    main.cpp
    regexpdialog.cpp
    regexpdialog.h
)

if (WIN32)
SET(RegExp_LIBS
    debug qtmaind.lib
    optimized qtmain.lib
)
endif(WIN32)

SET(RegExp_LIBS
    ${RegExp_LIBS}
    ${QT_QTCORE_LIBRARY_DEBUG}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY_DEBUG}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTXML_LIBRARY_DEBUG}
    ${QT_QTXML_LIBRARY}
)

add_executable(RegExp WIN32 ${RegExp_SRCS})

target_link_libraries(RegExp ${RegExp_LIBS})

set_target_properties(RegExp PROPERTIES OUTPUT_NAME "RegExp")
set_target_properties(RegExp PROPERTIES DEBUG_OUTPUT_NAME "RegExpD")
# dirty hack to avoid Debug/Release subdirectory
set_target_properties(RegExp PROPERTIES PREFIX "../")
set_target_properties(RegExp PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
