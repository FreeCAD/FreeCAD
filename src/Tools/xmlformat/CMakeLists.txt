cmake_minimum_required(VERSION 3.14)

project(xmlformat LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Xml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Xml REQUIRED)

add_executable(xmlformat
    main.cpp
)
target_link_libraries(xmlformat Qt${QT_VERSION_MAJOR}::Core
                                Qt${QT_VERSION_MAJOR}::Xml)
