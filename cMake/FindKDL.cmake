# Locate KDL install directory

# This module defines:
#
# * KDL_INSTALL where to find include, lib, bin, etc.
# * KDL_FOUND is set to true

# INCLUDE (${PROJECT_SOURCE_DIR}/config/FindPkgConfig.cmake)

if (CMAKE_PKGCONFIG_EXECUTABLE)

    message (STATUS "Detecting KDL")

    set (ENV{PKG_CONFIG_PATH} "${KDL_INSTALL}/lib/pkgconfig/")
    message ("Looking for KDL in: ${KDL_INSTALL}")
    pkgconfig ("orocos-kdl >= 0.99" KDL_FOUND KDL_INCLUDE_DIRS KDL_DEFINES KDL_LINK_DIRS KDL_LIBS)

    if (KDL_FOUND)
        message ("   Includes in: ${KDL_INCLUDE_DIRS}")
        message ("   Libraries in: ${KDL_LINK_DIRS}")
        message ("   Libraries: ${KDL_LIBS}")
        message ("   Defines: ${KDL_DEFINES}")

        include_directories (${KDL_INCLUDE_DIRS})
        link_directories (${KDL_LINK_DIRS})
        # OROCOS_PKGCONFIG_INCPATH("${KDLTK_INCLUDE_DIRS}") OROCOS_PKGCONFIG_LIBS("${KDL_LIBS}")
        # OROCOS_PKGCONFIG_LIBPATH("${KDL_LINK_DIRS}")

        set (ENV{PKG_CONFIG_PATH} "${KDL_INSTALL}/lib/pkgconfig/:${OROCOS_INSTALL}/lib/pkgconfig")
        message ("Looking for KDL Toolkit in: ${PKG_CONFIG_PATH}")
        pkgconfig ("orocos-kdltk-${OROCOS_TARGET} >= 0.99" KDLTK_FOUND KDLTK_INCLUDE_DIRS
                   KDLTK_DEFINES KDLTK_LINK_DIRS KDLTK_LIBS)
        if (KDLTK_FOUND)
            include_directories (${KDLTK_INCLUDE_DIRS})
            link_directories (${KDLTK_LINK_DIRS})
            orocos_pkgconfig_incpath ("${KDLTK_INCLUDE_DIRS}")
            orocos_pkgconfig_libpath ("${KDLTK_LINK_DIRS}")
            orocos_pkgconfig_libs ("${KDLTK_LIBS}")
            if (CORBA_ENABLED)
                set (ENV{PKG_CONFIG_PATH}
                     "${KDL_INSTALL}/lib/pkgconfig/:${OROCOS_INSTALL}/lib/pkgconfig")
                message ("Looking for KDL Toolkit CORBA extension in ${PKG_CONFIG_PATH}")
                pkgconfig (
                    "orocos-kdltk-corba-${OROCOS_TARGET} >= 0.99" KDLTKCORBA_FOUND
                    KDLTKCORBA_INCLUDE_DIRS KDLTKCORBA_DEFINES KDLTKCORBA_LINK_DIRS KDLTKCORBA_LIBS)
                if (KDLTKCORBA_FOUND)
                    include_directories (${KDLTKCORBA_INCLUDE_DIRS})
                    link_directories (${KDLTKCORBA_LINK_DIRS})
                    orocos_pkgconfig_incpath ("${KDLTKCORBA_INCLUDE_DIRS}")
                    orocos_pkgconfig_libpath ("${KDLTKCORBA_LINK_DIRS}")
                    orocos_pkgconfig_libs ("${KDLTKCORBA_LIBS}")
                endif (KDLTKCORBA_FOUND)
            endif (CORBA_ENABLED)
        endif (KDLTK_FOUND)
    endif (KDL_FOUND)

else (CMAKE_PKGCONFIG_EXECUTABLE)

    # Can't find pkg-config -- have to search manually
    message (FATAL_ERROR "Can't find KDL without pkgconfig !")

endif (CMAKE_PKGCONFIG_EXECUTABLE)
