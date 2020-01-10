# -------------------------------- Qt --------------------------------

if (NOT BUILD_QT5)
    # If using MacPorts, help the Qt4 finder.
    if(MACPORTS_PREFIX)
        if(NOT QT_QMAKE_EXECUTABLE)
            set(QT_QMAKE_EXECUTABLE ${MACPORTS_PREFIX}/libexec/qt4/bin/qmake)
        endif()
    endif()

    set(QT_MIN_VERSION 4.5.0)
    set(QT_USE_QTNETWORK TRUE)
    set(QT_USE_QTXML TRUE)
    if(BUILD_GUI)
        set(QT_USE_QTOPENGL TRUE)
        set(QT_USE_QTSVG TRUE)
        set(QT_USE_QTUITOOLS TRUE)
        set(QT_USE_QTWEBKIT TRUE)
    endif(BUILD_GUI)

    find_package(Qt4)# REQUIRED

    include(${QT_USE_FILE})

    if(NOT QT4_FOUND)
        message(FATAL_ERROR "========================\n"
                            "Qt4 libraries not found.\n"
                            "========================\n")
    endif(NOT QT4_FOUND)

    if(NOT QT_QTWEBKIT_FOUND)
        message("========================================================\n"
                "Qt Webkit not found, will not build browser integration.\n"
                "========================================================\n")
    endif(NOT QT_QTWEBKIT_FOUND)

    # This is a special version of the built in macro qt4_wrap_cpp
    # It is required since moc'ed files are now included instead of being added to projects directly
    # It adds a reverse dependency to solve this
    # This has the unfortunate side effect that some files are always rebuilt
    # There is probably a cleaner solution than this
    macro(fc_wrap_cpp outfiles)
        # get include dirs
        QT4_GET_MOC_FLAGS(moc_flags)
        QT4_EXTRACT_OPTIONS(moc_files moc_options moc_target ${ARGN})
        # fixes bug 0000585: bug with boost 1.48
        set(moc_options ${moc_options} -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

        foreach(it ${moc_files})
            get_filename_component(it ${it} ABSOLUTE)
            QT4_MAKE_OUTPUT_FILE(${it} moc_ cpp outfile)
            ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
                COMMAND ${QT_MOC_EXECUTABLE}
                ARGS ${moc_options} ${it} -o ${outfile}
                MAIN_DEPENDENCY ${it}
            )
            set(${outfiles} ${${outfiles}} ${outfile})
            add_file_dependencies(${it} ${outfile})
        endforeach(it)
    endmacro(fc_wrap_cpp)

elseif (BUILD_QT5)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Network REQUIRED)
    find_package(Qt5Xml REQUIRED)
    find_package(Qt5XmlPatterns REQUIRED)
    if(BUILD_GUI)
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5PrintSupport REQUIRED)
        find_package(Qt5OpenGL REQUIRED)
        find_package(Qt5Svg REQUIRED)
        find_package(Qt5UiTools REQUIRED)
        find_package(Qt5Concurrent REQUIRED)
        if (BUILD_WEB)
            if (${FREECAD_USE_QTWEBMODULE} MATCHES "Qt Webkit")
                find_package(Qt5WebKitWidgets REQUIRED)
            elseif(${FREECAD_USE_QTWEBMODULE} MATCHES "Qt WebEngine")
                find_package(Qt5WebEngineWidgets REQUIRED)
                if (Qt5WebEngineWidgets_VERSION VERSION_LESS 5.7.0)
                    message(FATAL_ERROR "** Minimum supported Qt5WebEngine version is 5.7.0!\n")
                endif()
            else() # Automatic
                find_package(Qt5WebKitWidgets QUIET)
                if(NOT Qt5WebKitWidgets_FOUND)
                    find_package(Qt5WebEngineWidgets REQUIRED)
                    if (Qt5WebEngineWidgets_VERSION VERSION_LESS 5.7.0)
                        message(FATAL_ERROR "** Minimum supported Qt5WebEngine version is 5.7.0!\n")
                    endif()
                endif()
            endif()
        endif()
        if(MSVC AND ${Qt5Core_VERSION} VERSION_GREATER "5.2.0")
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
        # fixes bug 0000585: bug with boost 1.48
        set(moc_options ${moc_options} -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

        foreach(it ${moc_files})
            get_filename_component(it ${it} ABSOLUTE)
            qt5_make_output_file(${it} moc_ cpp outfile)
            qt5_create_moc_command(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
            set(${outfiles} ${${outfiles}} ${outfile})
            add_file_dependencies(${it} ${outfile})
        endforeach(it)
    endmacro(fc_wrap_cpp)
endif (NOT BUILD_QT5)
