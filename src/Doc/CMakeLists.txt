# SPDX-License-Identifier: LGPL-2.1-or-later

# Generated HTML doc into build tree
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdPartyLibraries.html.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ThirdPartyLibraries.html"
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.html"
    "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.html"
    COPYONLY
)

# Note: The CONTRIBUTORS file is compiled into the FreeCAD GUI Qt resource file.

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ThirdPartyLibraries.html"
        "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.html"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)


# FreeCAD source documentation via Doxygen (WebDoc and DevDoc targets)
# Note: see Doxygen official doc and comments in BuildDoc config file
# how to use and specify certain tags (patterns, macros, html options, etc.)
if(DOXYGEN_FOUND)

    # Note: this test is obsolete if DevDoc target is used.
    if(NOT BUILD_GUI)
        message(STATUS "Note: GUI is not built. Doc may lack some parts.")
    endif()

    # Git Branch and Commit info
    find_package(Git QUIET)
    if(DEFINED ENV{CI_COMMIT_BRANCH} AND DEFINED ENV{CI_COMMIT_SHA})
        set(GIT_BRANCH "$ENV{CI_COMMIT_BRANCH}")
        set(GIT_COMMIT "$ENV{CI_COMMIT_SHA}")
        set(GIT_INFO "Built from git branch ${GIT_BRANCH}, commit ${GIT_COMMIT}")
    elseif(Git_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        set(GIT_BRANCH "unknown")
        set(GIT_COMMIT "unknown")
    endif()

    if(Git_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -n1 "--pretty=format:%h %cD '%s'"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(GIT_INFO "Built from git branch ${GIT_BRANCH}, commit ${GIT_REVISION}")
    else()
        set(GIT_INFO "Built from unknown branch, commit unknown (Git not found).")
    endif()

    # Language
    set(DOXYGEN_LANGUAGE "English" CACHE STRING "Language used by Doxygen")

    # FreeCAD name and version for Doxygen
    set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
    set(DOXYGEN_PROJECT_NUMBER "${PACKAGE_VERSION}${PACKAGE_VERSION_SUFFIX}")

    # Doxygen project assets, layout, HTML templates, stylesheets and extra files
    set(DOXYGEN_PROJECT_LOGO "${CMAKE_CURRENT_BINARY_DIR}/templates/FreeCAD-wordmark-light.svg")
    set(DOXYGEN_PROJECT_ICON "${CMAKE_CURRENT_BINARY_DIR}/templates/favicon.svg")

    set(DOXYGEN_IMAGE_PATH "${CMAKE_SOURCE_DIR}/src/Gui/Icons")
    set(DOXYGEN_LAYOUT_FILE "${CMAKE_SOURCE_DIR}/src/Doc/FreecadDoxygenLayout.xml")

    set(DOXYGEN_HTML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/templates/header.html")
    set(DOXYGEN_HTML_FOOTER "${CMAKE_CURRENT_BINARY_DIR}/templates/footer.html")
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_CURRENT_BINARY_DIR}/templates/customdoxygen.css")
    set(DOXYGEN_HTML_EXTRA_FILES_LIST
        "${CMAKE_CURRENT_BINARY_DIR}/templates/doxy-boot.js"
    )
    list(JOIN DOXYGEN_HTML_EXTRA_FILES_LIST " " DOXYGEN_HTML_EXTRA_FILES)

    # Source directories
    # Note: order is important for correct macro expansion.
    # Files containing macros definitions must be parsed before the files using them.
    set(DOXYGEN_INPUT_DIR_LIST
        "${COIN3D_INCLUDE_DIRS}/Inventor/fields/SoSubField.h"
        "${CMAKE_SOURCE_DIR}/src/3rdParty/PyCXX/CXX"
        "${CMAKE_SOURCE_DIR}/src/3rdParty/zipios++"
        "${CMAKE_SOURCE_DIR}/src/3rdParty"
        "${CMAKE_SOURCE_DIR}/src/Build"
        "${CMAKE_SOURCE_DIR}/src/Base"
        "${CMAKE_BINARY_DIR}/src/Base"
        "${CMAKE_SOURCE_DIR}/src/App"
        "${CMAKE_BINARY_DIR}/src/App"
        "${CMAKE_SOURCE_DIR}/src/Gui"
        "${CMAKE_BINARY_DIR}/src/Gui"
        "${CMAKE_SOURCE_DIR}/src/Mod"
        "${CMAKE_BINARY_DIR}/src/Mod"
        "${CMAKE_SOURCE_DIR}/src/Main" # Nothing to document there ATM
        "${CMAKE_SOURCE_DIR}/src/Doc"
        "${CMAKE_BINARY_DIR}/src/Doc"
    )
    list(JOIN DOXYGEN_INPUT_DIR_LIST " " DOXYGEN_INPUT)

    # File patterns to use as source input (restrict other input file types)
    # Note: update list with new added supported file type
    set(DOXYGEN_FILE_PATTERNS_LIST
        "*.h"
        "*.hpp"
        "*.hxx"
        "*.inl"
        "*.c"
        "*.cpp"
        "*.cxx"
        "*.py"
        "*.dox"
    )
    list(JOIN DOXYGEN_FILE_PATTERNS_LIST " " DOXYGEN_FILE_PATTERNS)

    # Exclude directories (not relevant, legacy or deprecated)
    set(DOXYGEN_EXCLUDE_DIR_LIST
        "${CMAKE_SOURCE_DIR}/src/Tools"
        "${CMAKE_SOURCE_DIR}/src/Doc/legacy"
        "${CMAKE_SOURCE_DIR}/src/Mod/Complete"
    )
    list(JOIN DOXYGEN_EXCLUDE_DIR_LIST " " DOXYGEN_EXCLUDE)

    # File patterns to exclude from source input
    set(DOXYGEN_EXCLUDE_PATTERNS_LIST
        "*.moc.*"
        "moc_*"
        "*.ui.h"
        "*/swigpyrun*.*"
        "*/lex.*.c"
        "*/.svn/*"
        "*/CMakeFiles/*"
        "*_rc.py"
    )
    list(JOIN DOXYGEN_EXCLUDE_PATTERNS_LIST " " DOXYGEN_EXCLUDE_PATTERNS)

    # Symbols to exclude (namespaces, classes, functions, etc.)
    set(DOXYGEN_EXCLUDE_SYMBOLS_LIST
        "__init__"
        "None"
        "__setstate__"
        "__getstate__"
        "_*"
        "boost"
        "Wm4"
        "WildMagic4"
        "KDL"
        "ap203*"
        "Py"
        "SIM"
    )
    list(JOIN DOXYGEN_EXCLUDE_SYMBOLS_LIST " " DOXYGEN_EXCLUDE_SYMBOLS)

    # Custom directories to include and process
    # Note: only needed to resolve external headers not listed in DOXYGEN_INPUT.
    # FreeCAD headers are already included, so no need for now (keep empty).
    set(DOXYGEN_INCLUDE_PATH "")

    # Predefined macros
    set(DOXYGEN_PREDEFINED_MACROS_LIST
        HAVE_SWIG
        AppAssemblyExport=
        AppCamExport=
        AppCamGuiExport=
        AppCompleteExport=
        AppDrawingExport=
        AppExport=
        AppFemExport=
        AppJtReaderExport=
        AppMeshPartExport=
        AppPartDesignExport=
        AppPartExport=
        AppPartGuiExport=
        AppRobotExport=
        AppSketcherExport=
        AppStartExport=
        AppTestGuiExport=
        AssemblyGuiExport=
        BaseExport=
        CompleteGuiExport=
        DrawingGuiExport=
        FemGuiExport=
        GuiExport=
        ImageAppExport=
        ImageGuiExport=
        MainExport=
        MeshExport=
        MeshGuiExport=
        MeshPartGuiExport=
        ModuleExport=
        PartDesignGuiExport=
        PartExport=
        PointsAppExport=
        PointsExport=
        PointsGuiExport=
        ReenExport=
        ReenGuiExport=
        RobotGuiExport=
        SandboxAppExport=
        SketcherExport=
        SketcherGuiExport=
        StartGuiExport=
        WebGuiExport=
        WM4_MEMORY_MANAGER
        WM4_FOUNDATION_ITEM=
        IMETHOD=inline
        COIN_DLL_API=
        COIN_INTERNAL
        PYCXX_EXPORT=
        KDL_INLINE
        YY_NO_UNISTD_H
        DOXYGEN_SHOULD_SKIP_THIS
        DEBUG_MACRO=1
    )
    list(JOIN DOXYGEN_PREDEFINED_MACROS_LIST " " DOXYGEN_PREDEFINED_MACROS)

    # Macros to expand
    set(DOXYGEN_EXPAND_AS_DEFINED_LIST
        Py_Header
        PYFUNCDEF_S
        PYFUNCIMP_S
        PYFUNCDEF_D
        PYFUNCIMP_D
        PYMETHODEDEF
        TYPESYSTEM_HEADER
        TYPESYSTEM_SOURCE_P
        TYPESYSTEM_SOURCE_ABSTRACT_P
        TYPESYSTEM_SOURCE
        TYPESYSTEM_SOURCE_ABSTRACT
        DEF_STD_CMD
        DEF_STD_CMD_A
        DEF_STD_CMD_C
        DEF_STD_CMD_AC
        DEF_STD_CMD_ACL
        DEF_3DV_CMD
        ADD_PROPERTY
        ADD_PROPERTY_TYPE
        PROPERTY_HEADER
        PROPERTY_SOURCE
        PROPERTY_SOURCE_ABSTRACT
        TYPESYSTEM_SOURCE_TEMPLATE
        PROPERTY_SOURCE_TEMPLATE
        SO_SFIELD_SOURCE
        SO_SFIELD_CONSTRUCTOR_SOURCE
        SO_SFIELD_VALUE_SOURCE
        SO_SFIELD_REQUIRED_SOURCE
        SO_SFIELD_HEADER
        SO_SFIELD_CONSTRUCTOR_HEADER
        SO_SFIELD_REQUIRED_HEADER
        SO_SFIELD_VALUE_HEADER
        PRIVATE_SFIELD_IO_HEADER
        PRIVATE_TYPEID_SOURCE
        PRIVATE_EQUALITY_SOURCE
        USING_PART_OF_NAMESPACE_EIGEN
    )
    list(JOIN DOXYGEN_EXPAND_AS_DEFINED_LIST " " DOXYGEN_EXPAND_AS_DEFINED)

    # Optional Coin3D tag files
    find_package(Coin3DDoc)
    if(COIN3D_DOC_FOUND)
        set(DOXYGEN_TAGFILES ${COIN3D_DOC_TAGFILE}=${COIN3D_DOC_PATH})
    else()
        set(DOXYGEN_TAGFILES "")
    endif()

    # Graphviz check
    if(DOXYGEN_DOT_EXECUTABLE)
        set(DOXYGEN_HAVE_DOT YES)
        get_filename_component(DOXYGEN_DOT_PATH
            ${DOXYGEN_DOT_EXECUTABLE}
            DIRECTORY
        )
    else()
        message(STATUS "Note: Graphviz not found. Doxygen docs look better with Graphviz installed.")
        set(DOXYGEN_HAVE_DOT NO)
        set(DOXYGEN_DOT_PATH "")
    endif()

    # Configure Doxygen input files
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.in"
        "${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox"
        @ONLY
    )

    # Configure Doxygen profile (DEV or WEB) and set build commands for custom targets
    function(doxygen_profile PROFILE COMMANDS)
        string(TOUPPER "${PROFILE}" PROFILE)
        message(STATUS "Configuring Doxygen ${PROFILE} doc")

        # Output directories
        set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/doc/${PROFILE}")
        set(DOXYGEN_HTML_OUTPUT "html/${GIT_BRANCH}")

        # Profile specific options
        if(PROFILE STREQUAL "DEV")
            set(DOXYGEN_PROJECT_BRIEF "FreeCAD source doc - Full dev version - ${GIT_INFO}")
            set(DOXYGEN_EXTRACT_ALL YES)
            set(DOXYGEN_EXTRACT_STATIC YES)
            set(DOXYGEN_SOURCE_BROWSER YES)
            set(DOXYGEN_FULL_PATH_NAMES NO)
            set(DOXYGEN_STRIP_PATH "")
            set(DOXYGEN_ALPHABETICAL_INDEX YES)
            set(DOXYGEN_DISABLE_INDEX NO)
            set(DOXYGEN_GENERATE_TREEVIEW NO)
            set(DOXYGEN_SEARCHENGINE NO)
            set(DOXYGEN_CLASS_GRAPHS YES)
            set(DOXYGEN_GROUP_GRAPHS YES)
            set(DOXYGEN_INCLUDE_GRAPH YES)
        elseif(PROFILE STREQUAL "WEB")
            set(DOXYGEN_PROJECT_BRIEF "FreeCAD source doc - Light web version - ${GIT_INFO}")
            set(DOXYGEN_EXTRACT_ALL NO)
            set(DOXYGEN_EXTRACT_STATIC NO)
            set(DOXYGEN_SOURCE_BROWSER NO)
            set(DOXYGEN_FULL_PATH_NAMES YES)
            set(DOXYGEN_STRIP_PATH "${CMAKE_SOURCE_DIR}")
            set(DOXYGEN_ALPHABETICAL_INDEX NO)
            set(DOXYGEN_DISABLE_INDEX NO)
            set(DOXYGEN_GENERATE_TREEVIEW YES)
            set(DOXYGEN_SEARCHENGINE YES)
            set(DOXYGEN_CLASS_GRAPHS NO)
            set(DOXYGEN_GROUP_GRAPHS NO)
            set(DOXYGEN_INCLUDE_GRAPH NO)
        else()
            message(FATAL_ERROR "Unknown Doxygen profile: ${PROFILE}")
        endif()

        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/BuildDoc.cfg.in"
            "${CMAKE_CURRENT_BINARY_DIR}/Build${PROFILE}Doc.cfg"
            @ONLY
        )

        # Build list of commands for the target
        set(COMMANDS_LIST
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/templates"
                "${CMAKE_CURRENT_BINARY_DIR}/templates"

            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${DOXYGEN_OUTPUT_DIR}/${DOXYGEN_HTML_OUTPUT}"

            COMMAND "${DOXYGEN_EXECUTABLE}"
                "${CMAKE_CURRENT_BINARY_DIR}/Build${PROFILE}Doc.cfg"
        )
        set(${COMMANDS} "${COMMANDS_LIST}" PARENT_SCOPE)
    endfunction()

    # Note: targets do not pick up changes in *.pyi and *Py.xml files.
    # For these changes to propagate, execute a build before using these targets.

    # Generate DevDoc target (full doc for local builds)
    doxygen_profile(DEV DEV_COMMANDS)
    add_custom_target(DevDoc
        ${DEV_COMMANDS}
        COMMAND_EXPAND_LISTS
        COMMENT "Generating full developer doc"
        VERBATIM
    )

    # Generate WebDoc target (lightweight web doc)
    doxygen_profile(WEB WEB_COMMANDS)
    add_custom_target(WebDoc
        ${WEB_COMMANDS}
        COMMAND_EXPAND_LISTS
        COMMENT "Generating light web doc"
        VERBATIM
    )

else()
    add_custom_target(DevDoc
        COMMAND "${CMAKE_COMMAND}" -E echo
            "Doxygen not found. Install it and rerun CMake to build the source dev doc."
    )
    add_custom_target(WebDoc
        COMMAND "${CMAKE_COMMAND}" -E echo
            "Doxygen not found. Install it and rerun CMake to build the source web doc."
    )
endif()
