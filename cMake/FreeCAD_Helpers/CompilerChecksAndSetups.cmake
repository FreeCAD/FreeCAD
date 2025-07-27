# Some resources
# https://github.com/dev-cafe/cmake-cookbook
# https://cmake.org/cmake/help/v3.8/manual/cmake-compile-features.7.html

macro(CompilerChecksAndSetups)
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        set(CMAKE_COMPILER_IS_CLANGXX TRUE)
    endif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

    # ================================================================================

    # Use a heuristic of 1 GiB of RAM needed per compiler job and limit to
    # a single link job. Modern linkers are multithreaded and running them concurrently
    # can exhaust resources.
    cmake_host_system_information(RESULT avail_mem_MiB QUERY TOTAL_PHYSICAL_MEMORY)
    math(EXPR max_compile_procs "${avail_mem_MiB} / 1024")
    set_property(GLOBAL PROPERTY JOB_POOLS compile_jobs=${max_compile_procs} link_jobs=1)

    # Allow developers to use Boost < 1.74
    if (NOT BOOST_MIN_VERSION)
        set(BOOST_MIN_VERSION 1.74)
    endif()

    # For older cmake versions the variable 'CMAKE_CXX_COMPILER_VERSION' is missing
    if(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                        OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
    endif(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_CXX_COMPILER_VERSION)

    # Enabled C++20 for Freecad 1.1 and later
    set(BUILD_ENABLE_CXX_STD "C++20"  CACHE STRING  "Enable C++ standard")
    set_property(CACHE BUILD_ENABLE_CXX_STD PROPERTY STRINGS
                 "C++20"
                 "C++23"
    )

    if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.2)
        message(FATAL_ERROR "FreeCAD 1.1 and later requires C++20.  G++ must be 11.2 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
    elseif(CMAKE_COMPILER_IS_CLANGXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
        message(FATAL_ERROR "FreeCAD 1.1 and later requires C++20.  Clang must be 14.0 or later, the used version is ${CMAKE_CXX_COMPILER_VERSION}")
    endif()

    # Escape the two plus chars as otherwise cmake complains about invalid regex
    if(${BUILD_ENABLE_CXX_STD} MATCHES "C\\+\\+23")
        set(CMAKE_CXX_STANDARD 23)
    else()
        set(CMAKE_CXX_STANDARD 20)
    endif()
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Log the compiler and version
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}, version: ${CMAKE_CXX_COMPILER_VERSION}")

    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
        include(${CMAKE_SOURCE_DIR}/cMake/ConfigureChecks.cmake)
        configure_file(${CMAKE_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
        add_definitions(-DHAVE_CONFIG_H)

        set(CMAKE_CXX_FLAGS "-fdiagnostics-color ${CMAKE_CXX_FLAGS}")

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
                if(APPLE)
                    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
                elseif(UNIX)
                    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--undefined,dynamic_lookup")
                endif()
            endif()
        endif(BUILD_DYNAMIC_LINK_PYTHON)

        if(BUILD_USE_LIBCXX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif()

        if(BUILD_ENABLE_TIME_TRACE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftime-trace")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
        endif()
    endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

    if(CMAKE_COMPILER_IS_CLANGXX)
        # Disable warning about potentially uninstantiated static members
        # because it leads to a lot of false-positives.
        #
        # https://en.wikipedia.org/wiki/Xcode#Latest_versions
        if (APPLE)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
            add_definitions(-DGL_SILENCE_DEPRECATION)
        elseif (UNIX)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
        endif()

        # boost.preprocessor 1.74 and earlier turns off variadics for clang regardless of version, even though they
        # work in all versions of clang that we support. Manually force variadic macro support until our oldest
        # supported version of boost is 1.75 or higher.
        add_definitions(-DBOOST_PP_VARIADICS=1)
        message(STATUS "Force BOOST_PP_VARIADICS=1 for clang")
    endif()

    set (COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} BOOST_NO_CXX98_FUNCTION_BASE)

endmacro(CompilerChecksAndSetups)
