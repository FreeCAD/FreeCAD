# The Qt for Python project officially recommends using pip to install Shiboken, so we expect to find Shiboken in the
# site-packages directory. FreeCAD also requires shiboken6_generator, so this find script also locates that package
# and ensures the inclusion of its include directory when using the pip finding mechanism


find_package(Shiboken6 CONFIG QUIET)
if(NOT Shiboken6_FOUND)
    if(NOT Shiboken6_INCLUDE_DIR AND TARGET Shiboken6::Shiboken6)
        get_property(Shiboken6_INCLUDE_DIR TARGET Shiboken6::Shiboken6 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    if(NOT Shiboken6_INCLUDE_DIR)
        find_pip_package(Shiboken6)
        if (Shiboken6_FOUND)
            set(SHIBOKEN_LIBRARY ${Shiboken6_LIBRARIES})
            set(SHIBOKEN_MAJOR_VERSION 6)
            set(SHIBOKEN_FOUND ON)
        endif()
        # The include directory we actually want is part of shiboken6-generator
        find_pip_package(shiboken6_generator)
        if (shiboken6_generator_FOUND)
            set(SHIBOKEN_INCLUDE_DIR ${shiboken6_generator_INCLUDE_DIRS})
        endif()
    endif()
else()
    set(SHIBOKEN_INCLUDE_DIR ${Shiboken6_INCLUDE_DIRS})
    set(SHIBOKEN_LIBRARY ${Shiboken6_LIBRARIES})
    set(SHIBOKEN_FOUND ON)
endif()
