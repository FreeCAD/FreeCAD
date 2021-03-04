# -------------------------------- Qt --------------------------------

find_package(Qt5Core REQUIRED)

# For FreeCAD 0.20, pegged to Ubutu 18.04 LTS:
if(${Qt5Core_VERSION} VERSION_LESS "5.9")
    message (FATAL_ERROR "FreeCAD v0.20 requires Qt5 5.9 or later")
endif()
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5Concurrent REQUIRED)
if(BUILD_GUI)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5PrintSupport REQUIRED)
    find_package(Qt5OpenGL REQUIRED)
    find_package(Qt5Svg REQUIRED)
    find_package(Qt5UiTools REQUIRED)
    if (BUILD_WEB)
        if (${FREECAD_USE_QTWEBMODULE} MATCHES "Qt Webkit")
            find_package(Qt5WebKitWidgets REQUIRED)
        elseif(${FREECAD_USE_QTWEBMODULE} MATCHES "Qt WebEngine")
            find_package(Qt5WebEngineWidgets REQUIRED)
        else() # Automatic
            find_package(Qt5WebKitWidgets QUIET)
            if(NOT Qt5WebKitWidgets_FOUND)
                find_package(Qt5WebEngineWidgets REQUIRED)
            endif()
        endif()
    endif()
    if(MSVC)
      find_package(Qt5WinExtras QUIET)
    endif()
endif(BUILD_GUI)

# This is a special version of the built in macro qt5_wrap_cpp
# It is required since moc'ed files are now included instead of being added to projects directly
# It adds a reverse dependency to solve this
# This has the unfortunate side effect that some files are always rebuilt
# There is probably a cleaner solution than this
macro(fc_wrap_cpp outfiles )
    # get include dirs
    qt5_get_moc_flags(moc_flags)
    set(moc_files ${ARGN})

    foreach(it ${moc_files})
        get_filename_component(it ${it} ABSOLUTE)
        qt5_make_output_file(${it} moc_ cpp outfile)
        qt5_create_moc_command(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
        set(${outfiles} ${${outfiles}} ${outfile})
        add_file_dependencies(${it} ${outfile})
    endforeach(it)
endmacro(fc_wrap_cpp)
