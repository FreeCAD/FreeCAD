# - Try to find the Open Dynamics Engine library
# Once done this will define:
#
#  ODE_FOUND - system has the ODE library
#  ODE_INCLUDE_DIRS - the ODE include directory (include ODE headers with <ode/headername.h>)
#  ODE_LIBRARIES - the ODE library
#  ODE_DEFINITIONS - additional definitions necessary to use ODE (usually the precision flag)

if (ODE_INCLUDE_DIRS AND ODE_LIBRARIES)
    # ODE has already been found and the necessary variables are cached
    set(ODE_FOUND TRUE)
else (ODE_INCLUDE_DIRS AND ODE_LIBRARIES)
    # Find ODE

    # Use PkgConfig if possible
    if (NOT WIN32)
        find_package(PkgConfig)
        pkg_check_modules(PC_ODE ode)
    endif (NOT WIN32)

    # Find include dir
    find_path(ODE_INCLUDE_DIRS ode/ode.h
        HINTS ${PC_ODE_INCLUDEDIR}
        PATHS $ENV{OGRE_HOME}/include # ODE is shipped with the OGRE SDK
    )
    find_library(ODE_LIBRARIES ode
        NAMES ode ${PC_ODE_LIBRARIES}
        HINTS ${PC_ODE_LIBDIR}
        PATHS $ENV{OGRE_HOME}/lib
    )

    # Decide if ODE was found
    set(ODE_FOUND FALSE)
    if (ODE_INCLUDE_DIRS AND ODE_LIBRARIES)
        set(ODE_FOUND TRUE)
    endif (ODE_INCLUDE_DIRS AND ODE_LIBRARIES)

    # Find out extra definitions needed for ODE.
    # This is mostly needed for the ODE's precision selector:
    #  ODE can be built either in single-precision (default) or double-precision mode. The app has
    #  to be built in the same mode, otherwise things will crash.
    if (ODE_FOUND)
        # If PkgConfig found anything, then use its results, otherwise use ode-config script
        if (PC_ODE_FOUND)
            # Take the definitions from PkgConfig results
            set(ODE_DEFINITIONS ${PC_ODE_CFLAGS_OTHER}   CACHE STRING "")
            set(ODE_DEFINITIONS_FOUND TRUE)
        else (PC_ODE_FOUND)
            # Try to use ode-config
            find_program(ODECONFIG_EXECUTABLE ode-config)
            if (ODECONFIG_EXECUTABLE)
                execute_process(COMMAND ${ODECONFIG_EXECUTABLE} --cflags   OUTPUT_VARIABLE ODECONFIG_CFLAGS)
                set(ODE_DEFINITIONS ${ODECONFIG_CFLAGS}   CACHE STRING "")
                set(ODE_DEFINITIONS_FOUND TRUE)
            endif (ODECONFIG_EXECUTABLE)
        endif (PC_ODE_FOUND)
    endif (ODE_FOUND)

    # Show messages
    if (ODE_FOUND)
        if (NOT ODE_FIND_QUIETLY)
            message(STATUS "Found ODE: ${ODE_LIBRARIES}")
            # Show the ODE precision if the definitions were detected
            if (ODE_DEFINITIONS_FOUND)
                if (ODE_DEFINITIONS MATCHES -DdDOUBLE)
                    message(STATUS "ODE uses double precision")
                else (ODE_DEFINITIONS MATCHES -DdDOUBLE)
                    message(STATUS "ODE uses single precision")
                endif (ODE_DEFINITIONS MATCHES -DdDOUBLE)
            else (ODE_DEFINITIONS_FOUND)
                message(STATUS "Warning: couldn't determine ODE's precision")
            endif (ODE_DEFINITIONS_FOUND)
        endif (NOT ODE_FIND_QUIETLY)
    else (ODE_FOUND)
        if (ODE_FIND_REQUIRED)
            message(FATAL_ERROR "Could NOT find ODE")
        endif (ODE_FIND_REQUIRED)
    endif (ODE_FOUND)

endif(ODE_INCLUDE_DIRS AND ODE_LIBRARIES)
