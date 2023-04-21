# Some resources
# https://github.com/dev-cafe/cmake-cookbook
# https://cmake.org/cmake/help/v3.8/manual/cmake-compile-features.7.html

macro(CompilerChecksAndSetups)
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        set(CMAKE_COMPILER_IS_CLANGXX TRUE)
    endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

    # ================================================================================

    # Needed for boost1.69
    # Avoid that Python (pyerrors.h) defines snprintf and vsnprintf
    if (MSVC AND NOT MSVC_VERSION VERSION_LESS 1900)
        add_definitions(-DHAVE_SNPRINTF)
    elseif (MINGW)
        add_definitions(-DHAVE_SNPRINTF)
    endif()

    # Allow developers to use Boost < 1.65
    if (NOT BOOST_MIN_VERSION)
        set(BOOST_MIN_VERSION 1.65)
    endif()

    # For older cmake versions the variable 'CMAKE_CXX_COMPILER_VERSION' is missing
    if(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                        OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
    endif(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)

    # Enabled C++17 for Freecad 0.20 and later
        set(BUILD_ENABLE_CXX_STD "C++17"  CACHE STRING  "Enable C++ standard")
        set_property(CACHE BUILD_ENABLE_CXX_STD PROPERTY STRINGS
                     "C++17"
                     "C++20"
        )

        if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.3)
            message(FATAL_ERROR "FreeCAD 0.20 and later requires C++17.  G++ must be 7.3 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
        elseif(CMAKE_COMPILER_IS_CLANGXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
            message(FATAL_ERROR "FreeCAD 0.20 and later requires C++17.  Clang must be 6.0 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
        endif()

    # Escape the two plus chars as otherwise cmake complains about invalid regex
    if(${BUILD_ENABLE_CXX_STD} MATCHES "C\\+\\+20")
        set(CMAKE_CXX_STANDARD 20)
    elseif(${BUILD_ENABLE_CXX_STD} MATCHES "C\\+\\+17")
        set(CMAKE_CXX_STANDARD 17)
    endif()

    # Log the compiler and version
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}, version: ${CMAKE_CXX_COMPILER_VERSION}")

    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
        include(${CMAKE_SOURCE_DIR}/cMake/ConfigureChecks.cmake)
        configure_file(${CMAKE_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
        add_definitions(-DHAVE_CONFIG_H)

        # For now only set pedantic option for clang
        if(CMAKE_COMPILER_IS_CLANGXX)
            set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-write-strings ${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-write-strings ${CMAKE_CXX_FLAGS}")
        endif()
        include_directories(${CMAKE_CURRENT_BINARY_DIR})

        # get linker errors as soon as possible and not at runtime e.g. for modules
        if(BUILD_DYNAMIC_LINK_PYTHON)
            if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
            elseif(UNIX)
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
            endif()
        else(BUILD_DYNAMIC_LINK_PYTHON)
            if(CMAKE_COMPILER_IS_CLANGXX)
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
            endif()
        endif(BUILD_DYNAMIC_LINK_PYTHON)
    endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

    if(CMAKE_COMPILER_IS_CLANGXX)
        # Disable warning about potentially uninstantiated static members
        # because it leads to a lot of false-positives.
        #
        # https://en.wikipedia.org/wiki/Xcode#Latest_versions
        if (APPLE)
            if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
            endif()
            add_definitions(-DGL_SILENCE_DEPRECATION)
        elseif (UNIX)
            if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.9)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
            endif()
        endif()

        # older boost.preprocessor turn off variadics for clang
        add_definitions(-DBOOST_PP_VARIADICS=1)
        message(STATUS "Force BOOST_PP_VARIADICS=1 for clang")
    endif()

    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} BOOST_NO_CXX98_FUNCTION_BASE)

endmacro(CompilerChecksAndSetups)
