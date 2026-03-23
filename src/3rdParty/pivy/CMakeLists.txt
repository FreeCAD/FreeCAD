cmake_minimum_required(VERSION 3.18...3.30)
project(pivy_cmake_setup CXX)

if (NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

option(DISABLE_SWIG_WARNINGS "if on no swig warnings are shown" OFF)
option(PIVY_USE_QT6 "Prefer Qt6 over Qt5 if available" OFF)

find_package(SWIG 4.0.0 REQUIRED)
include(${SWIG_USE_FILE})

if (NOT TARGET Coin)
    find_package(Coin CONFIG REQUIRED)
else()
    set(Coin_FOUND 1)
    get_target_property(Coin_LIB_DIR Coin LIBRARY_OUTPUT_DIRECTORY)
    # This seems to return the SOVERSION on linux so didn't use it to avoid confusion
    # get_target_property(Coin_VERSION Coin VERSION)
    set(Coin_VERSION "Check target Coin!")

    set(Coin_INCLUDE_DIR ${Coin_SOURCE_DIR}/include)
    set(Coin_GENERATED_INCLUDE_DIR ${Coin_BINARY_DIR}/include)

    if (NOT TARGET Coin::Coin)
        add_library(Coin::Coin ALIAS Coin)
    endif()
endif()
find_package(SoQt CONFIG)

if (SoQt_FOUND)
    if (PIVY_USE_QT6)
        find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)
    else (PIVY_USE_QT6)
        find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)
    endif (PIVY_USE_QT6)
endif()

find_package(Python REQUIRED COMPONENTS Interpreter Development)

if (NOT PIVY_Python_SITEARCH)
    SET(PIVY_Python_SITEARCH ${Python_SITEARCH})
endif()

file(TO_CMAKE_PATH "${PIVY_Python_SITEARCH}" PIVY_Python_SITEARCH)

if (Python_FOUND)
	MESSAGE(STATUS "Python_FOUND: TRUE")
	MESSAGE(STATUS "PIVY_Python_SITEARCH: ${PIVY_Python_SITEARCH}")
endif()

if (Coin_FOUND)
	MESSAGE(STATUS "COIN_FOUND: TRUE")
	MESSAGE(STATUS "COIN_INCLUDE_DIR: ${Coin_INCLUDE_DIR}")
	MESSAGE(STATUS "COIN_LIB_DIR: ${Coin_LIB_DIR}")
	MESSAGE(STATUS "COIN_VERSION: ${Coin_VERSION}")
endif()

if (SoQt_FOUND)
	MESSAGE(STATUS "SOQT_FOUND: True")
	MESSAGE(STATUS "SOQT_INCLUDE_DIRS: ${SoQt_INCLUDE_DIRS}")
	MESSAGE(STATUS "SOQT_LIB_DIRS: ${SoQt_LIBRARY_DIRS}")
	MESSAGE(STATUS "SOQT_VERSION: ${SoQt_VERSION}")
endif()

# SWIGIFY HEADERS
# doing this with the origin python functions

execute_process(COMMAND ${Python_EXECUTABLE} ${PROJECT_SOURCE_DIR}/install_helpers.py ${PROJECT_SOURCE_DIR} ${Coin_INCLUDE_DIR})


# copy the python module
include(TargetCopyFiles.cmake)

add_custom_target(pivy ALL)
add_copy_directory(pivy ${PROJECT_SOURCE_DIR}/pivy
    DESTINATION ${CMAKE_BINARY_DIR}/pivy
    PATTERN *.py
)

add_subdirectory(interfaces)

install(DIRECTORY
    ${CMAKE_BINARY_DIR}/pivy
    DESTINATION ${PIVY_Python_SITEARCH}
    FILES_MATCHING
    PATTERN "*.py"
    PATTERN "*.so"
    PATTERN "*.dylib"
    PATTERN "*.dll"
    PATTERN "*.pyd"
    )
