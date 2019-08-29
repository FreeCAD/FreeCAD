# Try to find nglib/netgen
#
# Optional input NETGENDATA is path to the netgen libsrc source tree - this is
# required due to some headers not being installed by netgen.
#
# Once done this will define
#
# NGLIB_INCLUDE_DIR   - where the nglib include directory can be found
# NGLIB_LIBRARIES     - Link this to use nglib
# NETGEN_INCLUDE_DIRS - where the netgen include directories can be found
# NETGEN_DEFINITIONS  - C++ compiler defines required to use netgen/nglib
#
# See also: http://git.salome-platform.org/gitweb/?p=NETGENPLUGIN_SRC.git;a=summary


find_package(Netgen CONFIG)
if(Netgen_FOUND)
  set(NGLIB_INCLUDE_DIR ${NETGEN_INCLUDE_DIRS})
  set(NGLIB_LIBRARIES nglib)
  set(NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)
  # for external smesh only the following two variables are needed:
  set(NETGEN_FOUND True)
  set(NETGEN_INCLUDE_DIRS ${NETGEN_INCLUDE_DIRS})

else(Netgen_FOUND)


  SET(NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)

  IF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
      # We haven't supported Netgen prior to 5.3.1 on MacOS, and the current
      # plan is for the next Netgen version to be 6.1 (currently unreleased).
      IF(DEFINED HOMEBREW_PREFIX)
          SET(NGLIB_PREFIX ${HOMEBREW_PREFIX})
      ELSE(DEFINED HOMEBREW_PREFIX)
          SET(NGLIB_PREFIX ${MACPORTS_PREFIX})
      ENDIF(DEFINED HOMEBREW_PREFIX)

      FIND_PATH(NGLIB_INCLUDE_DIR nglib.h ${NGLIB_PREFIX}/include)

      FIND_LIBRARY(NGLIB_LIBNGLIB nglib ${NGLIB_PREFIX}/lib)
      FIND_LIBRARY(NGLIB_LIBMESH mesh ${NGLIB_PREFIX}/lib)
      FIND_LIBRARY(NGLIB_LIBOCC occ ${NGLIB_PREFIX}/lib)
      FIND_LIBRARY(NGLIB_LIBINTERFACE interface ${NGLIB_PREFIX}/lib)
      SET(NGLIB_LIBRARIES ${NGLIB_LIBNGLIB} ${NGLIB_LIBMESH}
                          ${NGLIB_LIBOCC} ${NGLIB_LIBINTERFACE})

      IF(NOT NETGENDATA)
          SET(NETGENDATA ${NGLIB_PREFIX}/include/netgen)
      ENDIF(NOT NETGENDATA)

  ELSEIF(WIN32)
      FIND_PATH(NGLIB_INCLUDE_DIR NAMES nglib.h PATHS ${NETGEN_INCLUDEDIR})
      SET(NETGEN_LIBS nglib mesh occ interface)
      SET(NGLIB_LIBRARIES "")
      FOREACH(it ${NETGEN_LIBS})
          FIND_LIBRARY(NGLIB ${it} PATHS ${NETGEN_LIBDIR})
          FIND_LIBRARY(NGLIBD ${it}d PATHS ${NETGEN_LIBDIR})
          IF(NGLIBD AND NGLIB)
              SET(NG_LIB optimized ${NGLIB}
                         debug ${NGLIBD})
              SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NG_LIB})
              UNSET(NGLIB CACHE)
              UNSET(NGLIBD CACHE)
          ELSEIF(NGLIB)
              SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB})
              UNSET(NGLIB CACHE)
          ENDIF()
      ENDFOREACH()

      IF(NOT NETGENDATA)
          SET(NETGENDATA netgen)
      ENDIF(NOT NETGENDATA)

  ELSE(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
      IF(NETGEN_ROOT)
          SET(NETGEN_INCLUDEDIR ${NETGEN_ROOT}/include)
          SET(NETGEN_LIBDIR ${NETGEN_ROOT}/lib)
          # allow to customize if NETGEN_ROOT is used
          IF(NOT NETGENDATA)
              SET(NETGENDATA ${NETGEN_ROOT}/libsrc)
          ENDIF(NOT NETGENDATA)
      ENDIF()

      FIND_PATH(NGLIB_INCLUDE_DIR NAMES nglib.h PATHS ${NETGEN_INCLUDEDIR} /usr/include /usr/include/netgen-mesher)
      FIND_LIBRARY(NGLIB_LIBNGLIB nglib PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
      SET(NGLIB_LIBRARIES ${NGLIB_LIBNGLIB})
      FIND_LIBRARY(NGLIB_LIBMESH  NAMES mesh ngmesh PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
      IF(NGLIB_LIBMESH)
          SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBMESH})
      ENDIF()
      FIND_LIBRARY(NGLIB_LIBOCC NAMES occ ngocc PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
      IF(NGLIB_LIBOCC)
          SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBOCC})
      ENDIF()
      FIND_LIBRARY(NGLIB_LIBINTERFACE NAMES interface nginterface PATHS ${NETGEN_LIBDIR} /usr/lib /usr/lib64 /usr/local/lib)
      IF(NGLIB_LIBINTERFACE)
          SET(NGLIB_LIBRARIES ${NGLIB_LIBRARIES} ${NGLIB_LIBINTERFACE})
      ENDIF()

      IF(NOT NETGENDATA)
          SET(NETGENDATA /usr/share/netgen/libsrc)
      ENDIF(NOT NETGENDATA)

  ENDIF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)

  FIND_PATH(NETGEN_DIR_include NAMES mydefs.hpp     NO_DEFAULT_PATH PATHS ${NGLIB_INCLUDE_DIR}/include ${NETGENDATA}/include ${NETGEN_INCLUDEDIR} ${NGLIB_INCLUDE_DIR})

  IF(NOT NGLIB_INCLUDE_DIR AND NOT NETGEN_DIR_include)
      MESSAGE(STATUS "Cannot find NETGEN header files.")
  ELSE()
      file(STRINGS ${NETGEN_DIR_include}/mydefs.hpp NETGEN_VERSION
          REGEX "#define PACKAGE_VERSION.*"
      )
  ENDIF()

  IF(NOT NGLIB_LIBRARIES)
      MESSAGE(STATUS "Cannot find NETGEN library.")
  ENDIF()

  IF(NGLIB_INCLUDE_DIR AND NGLIB_LIBRARIES)
      SET(Netgen_FOUND TRUE)
      SET(NETGEN_INCLUDE_DIRS ${NETGEN_DIR_include} ${NGLIB_INCLUDE_DIR})
      LIST(REMOVE_DUPLICATES NETGEN_INCLUDE_DIRS)
  ELSE()
      SET(Netgen_FOUND FALSE)
  ENDIF()
endif(Netgen_FOUND)

# Package-provided cMake file is not enough
IF(Netgen_FOUND)
  IF(NETGEN_VERSION)
      string(REGEX MATCHALL "[0-9]+" NETGEN_VERSION_expr ${NETGEN_VERSION})
      list(LENGTH NETGEN_VERSION_expr NETGEN_VERSION_COUNT)
      list(GET NETGEN_VERSION_expr 0 NETGEN_VERSION_MAJOR)
      IF(NETGEN_VERSION_COUNT GREATER 1)
          list(GET NETGEN_VERSION_expr 1 NETGEN_VERSION_MINOR)
      ELSE()
          set(NETGEN_VERSION_MINOR 0)
      ENDIF()
  ELSE()  # workaround for netgen 6.2 and newer. currently there is no easy way to detect the version
      # better use "find_package(netgen CONFIG REQUIRED)"
      set(NETGEN_VERSION_MAJOR 6)
      set(NETGEN_VERSION_MINOR 2)
  ENDIF()

  MATH(EXPR NETGEN_VERSION_C "(${NETGEN_VERSION_MAJOR} << 16) + (${NETGEN_VERSION_MINOR} << 8)")
  MATH(EXPR NETGEN_VERSION_62 "(6 << 16) + (2 << 8)")
  IF(NOT NETGEN_VERSION_C LESS NETGEN_VERSION_62) # Version >= 6.2
    IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang sometimes fails to include <cstdio>
        include(CMakePushCheckState)
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${NETGEN_CXX_FLAGS}")
        check_cxx_source_compiles("#include <cstdio>\nint main(){}" CSTDIO_INCLUDE_TRY1)
        IF(NOT CSTDIO_INCLUDE_TRY1)
            # Ugly hack to make <stdio.h> building gets function
            set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -U__cplusplus -D__cplusplus=201103L")
            check_cxx_source_compiles("#include <cstdio>\nint main(){}" CSTDIO_INCLUDE_TRY2)
            IF(NOT CSTDIO_INCLUDE_TRY2)
                message(FATAL_ERROR "Cannot #include <cstdio>.")
            ELSE()
                set(NETGEN_CXX_FLAGS "${NETGEN_CXX_FLAGS} -U__cplusplus -D__cplusplus=201103L")
            ENDIF()
         ENDIF()
         cmake_pop_check_state()
    ENDIF()
  ENDIF()
  MESSAGE(STATUS "Found NETGEN version ${NETGEN_VERSION_MAJOR}.${NETGEN_VERSION_MINOR}, calculated: ${NETGEN_VERSION_C}")
  LIST(APPEND NETGEN_DEFINITIONS -DNETGEN_VERSION=${NETGEN_VERSION_C})
ENDIF()
