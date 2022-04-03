project(FreeCAD_widgets)
cmake_minimum_required(VERSION 3.2.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Designer REQUIRED)

include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Designer_INCLUDE_DIRS}
)

add_library(FreeCAD_widgets SHARED
    customwidgets.cpp
    customwidgets.h
    plugin.cpp
    plugin.h
)

target_link_libraries(FreeCAD_widgets PRIVATE ${Qt5Widgets_LIBRARIES} ${Qt5Designer_LIBRARIES})
target_compile_options(FreeCAD_widgets PRIVATE ${COMPILE_OPTIONS})

# Get the install location of a plugin to determine the path to designer plguins
get_target_property(WEBENGINE_PLUGIN_LOCATION Qt5::QWebEngineViewPlugin LOCATION)
get_filename_component(DESIGNER_PLUGIN_LOCATION ${WEBENGINE_PLUGIN_LOCATION} DIRECTORY)
message(STATUS "Plugin will installed to: ${DESIGNER_PLUGIN_LOCATION}")

# Override the CMake variable
set(CMAKE_INSTALL_PREFIX ${DESIGNER_PLUGIN_LOCATION} CACHE PATH "Install path to Qt Designer plugins" FORCE)

INSTALL(TARGETS FreeCAD_widgets
    LIBRARY DESTINATION ${DESIGNER_PLUGIN_LOCATION}
)
