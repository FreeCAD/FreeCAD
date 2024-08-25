# Try to find nglib/netgen
#
# Optional input NETGENDATA is path to the netgen libsrc source tree - this is required due to some
# headers not being installed by netgen.
#
# Once done this will define
#
# NGLIB_INCLUDE_DIR   - where the nglib include directory can be found NGLIB_LIBRARIES     - Link
# this to use nglib NETGEN_INCLUDE_DIRS - where the netgen include directories can be found
# NETGEN_DEFINITIONS  - C++ compiler defines required to use netgen/nglib
#
# See also: http://git.salome-platform.org/gitweb/?p=NETGENPLUGIN_SRC.git;a=summary

find_package (Netgen CONFIG)
if (Netgen_FOUND)
    set (NGLIB_INCLUDE_DIR ${NETGEN_INCLUDE_DIRS})
    set (NGLIB_LIBRARIES nglib)
    set (NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)
    # for external smesh only the following two variables are needed:
    set (NETGEN_FOUND True)
    set (NETGEN_INCLUDE_DIRS ${NETGEN_INCLUDE_DIRS})

else ()

    set (NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)

    if (WIN32)
        find_path (
            NGLIB_INCLUDE_DIR
            NAMES nglib.h
            PATHS ${NETGEN_INCLUDEDIR})
        set (NETGEN_LIBS nglib mesh occ interface)
        set (NGLIB_LIBRARIES "")
        foreach (it ${NETGEN_LIBS})
            find_library (NGLIB ${it} PATHS ${NETGEN_LIBDIR})
            find_library (NGLIBD ${it}d PATHS ${NETGEN_LIBDIR})
            if (NGLIBD AND NGLIB)
                set (NG_LIB optimized ${NGLIB} debug ${NGLIBD})
                set (NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NG_LIB})
                unset (NGLIB CACHE)
                unset (NGLIBD CACHE)
            elseif (NGLIB)
                set (NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB})
                unset (NGLIB CACHE)
            endif ()
        endforeach ()

        if (NOT NETGENDATA)
            set (NETGENDATA netgen)
        endif (NOT NETGENDATA)

    elseif (DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
        if (NETGEN_ROOT)
            set (NETGEN_INCLUDEDIR ${NETGEN_ROOT}/include)
            set (NETGEN_LIBDIR ${NETGEN_ROOT}/lib)
            # allow to customize if NETGEN_ROOT is used
            if (NOT NETGENDATA)
                set (NETGENDATA ${NETGEN_ROOT}/libsrc)
            endif ()
        endif ()

        find_path (
            NGLIB_INCLUDE_DIR
            NAMES nglib.h
            PATHS ${NETGEN_INCLUDEDIR} /usr/include /usr/include/netgen-mesher)
        find_library (NGLIB_LIBNGLIB nglib PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64
                                                 /usr/local/lib)
        set (NGLIB_LIBRARIES ${NGLIB_LIBNGLIB})
        find_library (
            NGLIB_LIBMESH
            NAMES mesh ngmesh
            PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
        if (NGLIB_LIBMESH)
            set (NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBMESH})
        endif ()
        find_library (
            NGLIB_LIBOCC
            NAMES occ ngocc
            PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
        if (NGLIB_LIBOCC)
            set (NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBOCC})
        endif ()
        find_library (
            NGLIB_LIBINTERFACE
            NAMES interface nginterface
            PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
        if (NGLIB_LIBINTERFACE)
            set (NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBINTERFACE})
        endif ()

        if (NOT NETGENDATA)
            set (NETGENDATA /usr/share/netgen/libsrc)
        endif ()
    endif ()

    find_path (
        NETGEN_DIR_include
        NAMES mydefs.hpp
        NO_DEFAULT_PATH
        PATHS ${NGLIB_INCLUDE_DIR}/include ${NETGENDATA}/include ${NETGEN_INCLUDEDIR}
              ${NGLIB_INCLUDE_DIR})

    if (NOT NGLIB_INCLUDE_DIR AND NOT NETGEN_DIR_include)
        message (STATUS "Cannot find NETGEN header files.")
    elseif (NOT NETGEN_VERSION)
        file (STRINGS ${NETGEN_DIR_include}/mydefs.hpp NETGEN_VERSION
              REGEX "#define PACKAGE_VERSION.*")
    endif ()

    if (NOT NGLIB_LIBRARIES)
        message (STATUS "Cannot find NETGEN library.")
    endif ()

    if (NGLIB_INCLUDE_DIR AND NGLIB_LIBRARIES)
        set (Netgen_FOUND TRUE)
        set (NETGEN_INCLUDE_DIRS ${NETGEN_DIR_include} ${NGLIB_INCLUDE_DIR})
        list (REMOVE_DUPLICATES NETGEN_INCLUDE_DIRS)
    else ()
        set (Netgen_FOUND FALSE)
    endif ()
endif ()

# Package-provided cMake file is not enough
if (Netgen_FOUND)
    if (NOT NETGEN_VERSION_MAJOR)
        if (NETGEN_VERSION)
            string (REGEX MATCHALL "[0-9]+" NETGEN_VERSION_expr ${NETGEN_VERSION})
            list (LENGTH NETGEN_VERSION_expr NETGEN_VERSION_COUNT)
            list (GET NETGEN_VERSION_expr 0 NETGEN_VERSION_MAJOR)
            if (NETGEN_VERSION_COUNT GREATER 1)
                list (GET NETGEN_VERSION_expr 1 NETGEN_VERSION_MINOR)
            else ()
                set (NETGEN_VERSION_MINOR 0)
            endif ()
        else () # workaround for netgen 6.2 and newer. currently there is no easy way to detect the
                # version
            # better use "find_package(netgen CONFIG REQUIRED)"
            set (NETGEN_VERSION_MAJOR 6)
            set (NETGEN_VERSION_MINOR 2)
        endif ()
        set (NETGEN_VERSION_PATCH 0)
    endif ()

    math (
        EXPR
        NETGEN_VERSION_C
        "(${NETGEN_VERSION_MAJOR} << 16) + (${NETGEN_VERSION_MINOR} << 8) + (${NETGEN_VERSION_PATCH})"
    )
    math (EXPR NETGEN_VERSION_62 "(6 << 16) + (2 << 8)")
    math (EXPR NETGEN_VERSION_62_2004 "(6 << 16) + (2 << 8) + (2004)")
    if (NOT NETGEN_VERSION_C LESS NETGEN_VERSION_62) # Version >= 6.2
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Clang sometimes fails to include <cstdio>
            include (CMakePushCheckState)
            cmake_push_check_state (RESET)
            set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${NETGEN_CXX_FLAGS}")
            check_cxx_source_compiles ("#include <cstdio>\nint main(){}" CSTDIO_INCLUDE_TRY1)
            if (NOT CSTDIO_INCLUDE_TRY1)
                # Ugly hack to make <stdio.h> building gets function
                set (CMAKE_REQUIRED_FLAGS
                     "${CMAKE_REQUIRED_FLAGS} -U__cplusplus -D__cplusplus=201103L")
                check_cxx_source_compiles ("#include <cstdio>\nint main(){}" CSTDIO_INCLUDE_TRY2)
                if (NOT CSTDIO_INCLUDE_TRY2)
                    message (FATAL_ERROR "Cannot #include <cstdio>.")
                else ()
                    set (NETGEN_CXX_FLAGS "${NETGEN_CXX_FLAGS} -U__cplusplus -D__cplusplus=201103L")
                endif ()
            endif ()
            cmake_pop_check_state ()
        endif ()
    endif ()
    message (
        STATUS
            "Found NETGEN version ${NETGEN_VERSION_MAJOR}.${NETGEN_VERSION_MINOR}, calculated: ${NETGEN_VERSION_C}"
    )
    list (APPEND NETGEN_DEFINITIONS -DNETGEN_VERSION=${NETGEN_VERSION_C})
endif ()
