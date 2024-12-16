macro(InitializeFreeCADBuildOptions)
    # ==============================================================================
    # =================   All the options for the build process    =================
    # ==============================================================================

    option(BUILD_FORCE_DIRECTORY "The build directory must be different to the source directory." OFF)
    option(BUILD_GUI "Build FreeCAD Gui. Otherwise you have only the command line and the Python import module." ON)
    option(FREECAD_USE_EXTERNAL_ZIPIOS "Use system installed zipios++ instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_SMESH "Use system installed smesh instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_KDL "Use system installed orocos-kdl instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_FMT "Use system installed fmt library if available instead of fetching the source." ON)
    option(FREECAD_USE_EXTERNAL_ONDSELSOLVER "Use system installed OndselSolver instead of git submodule." OFF)
    option(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
    option(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)
    option(BUILD_WITH_CONDA "Set ON if you build FreeCAD with conda" OFF)
    option(BUILD_DYNAMIC_LINK_PYTHON "If OFF extension-modules do not link against python-libraries" ON)
    option(INSTALL_TO_SITEPACKAGES "If ON the freecad root namespace (python) is installed into python's site-packages" ON)
    option(OCCT_CMAKE_FALLBACK "disable usage of occt-config files" OFF)
    if (WIN32 OR APPLE)
        option(FREECAD_USE_QT_FILEDIALOG "Use Qt's file dialog instead of the native one." OFF)
    else()
        option(FREECAD_USE_QT_FILEDIALOG "Use Qt's file dialog instead of the native one." ON)
    endif()

    # == Win32 is default behaviour use the LibPack copied in Source tree ==========
    if(MSVC)
        option(FREECAD_RELEASE_PDB "Create PDB files for Release version." ON)
        option(FREECAD_RELEASE_SEH "Enable Structured Exception Handling for Release version." ON)
        option(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." ON)
        option(FREECAD_USE_PCH "Activate precompiled headers where it's used." ON)

        if (DEFINED ENV{FREECAD_LIBPACK_DIR})
            set(FREECAD_LIBPACK_DIR $ENV{FREECAD_LIBPACK_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
            message(STATUS "Found libpack env variable: ${FREECAD_LIBPACK_DIR}")
        else()
            set(FREECAD_LIBPACK_DIR ${CMAKE_SOURCE_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
        endif()

        set(LIBPACK_FOUND OFF )
        if (EXISTS ${FREECAD_LIBPACK_DIR}/plugins/imageformats/qsvg.dll)
            set(LIBPACK_FOUND ON )
            set(COPY_LIBPACK_BIN_TO_BUILD OFF )
            # Create install commands for dependencies for INSTALL target in FreeCAD solution
            option(FREECAD_INSTALL_DEPEND_DIRS "Create install dependency commands for the INSTALL target found
                in the FreeCAD solution." ON)
            # Copy libpack smaller dependency folders to build folder per user request - if non-existent at destination
            if (NOT EXISTS ${CMAKE_BINARY_DIR}/bin/imageformats/qsvg.dll)
                option(FREECAD_COPY_DEPEND_DIRS_TO_BUILD "Copy smaller libpack dependency directories to build directory." OFF)
            endif()
            # Copy libpack 'bin' directory contents to build 'bin' per user request - only IF NOT EXISTS already
            if (NOT EXISTS ${CMAKE_BINARY_DIR}/bin/DLLs)
                set(COPY_LIBPACK_BIN_TO_BUILD ON )
                option(FREECAD_COPY_LIBPACK_BIN_TO_BUILD "Copy larger libpack dependency 'bin' folder to the build directory." OFF)
                # Copy only the minimum number of files to get a working application
                option(FREECAD_COPY_PLUGINS_BIN_TO_BUILD "Copy plugins to the build directory." OFF)
            endif()
        else()
            message(WARNING Could not locate ${FREECAD_LIBPACK_DIR}/plugins/imageformats/qsvg.dll)
            message("Libpack NOT found.\nIf you intend to use a Windows libpack, set the FREECAD_LIBPACK_DIR to the libpack directory.")
            message(STATUS "Visit: https://github.com/FreeCAD/FreeCAD-Libpack/releases/ for Windows libpack downloads.")
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
        option(FREECAD_WARN_ERROR "Make all warnings into errors. " OFF)
    else(MSVC)
        option(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." OFF)
        set(FREECAD_LIBPACK_DIR ""  CACHE PATH  "Directory of the FreeCAD LibPack")
    endif(MSVC)

    ChooseQtVersion()

    # https://blog.kitware.com/constraining-values-with-comboboxes-in-cmake-cmake-gui/
    set(FREECAD_USE_OCC_VARIANT "Community Edition"  CACHE STRING  "Official OpenCASCADE version or community edition")
    set_property(CACHE FREECAD_USE_OCC_VARIANT PROPERTY STRINGS
                 "Official Version"
                 "Community Edition"
    )

    configure_file(${CMAKE_SOURCE_DIR}/src/QtOpenGL.h.cmake ${CMAKE_BINARY_DIR}/src/QtOpenGL.h)

    option(BUILD_DESIGNER_PLUGIN "Build and install the designer plugin" OFF)

    if(APPLE)
        option(FREECAD_CREATE_MAC_APP "Create app bundle on install" OFF)

        if(FREECAD_CREATE_MAC_APP)
            install(
                DIRECTORY ${CMAKE_SOURCE_DIR}/src/MacAppBundle/FreeCAD.app/
                DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app
            )

            # It should be safe to assume we've got sed on OSX...
            install(CODE "
                execute_process(COMMAND
                    sed -i \"\" -e s/VERSION_STRING_FROM_CMAKE/${PACKAGE_VERSION}/
                    -e s/NAME_STRING_FROM_CMAKE/${PROJECT_NAME}/
                    ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Info.plist)
                   ")

            set(CMAKE_INSTALL_PREFIX
                ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents)
            set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib )
        endif(FREECAD_CREATE_MAC_APP)
        set(CMAKE_MACOSX_RPATH TRUE )
    endif(APPLE)

    option(BUILD_FEM "Build the FreeCAD FEM module" ON)
    option(BUILD_SANDBOX "Build the FreeCAD Sandbox module which is only for testing purposes" OFF)
    option(BUILD_TEMPLATE "Build the FreeCAD template module which is only for testing purposes" OFF)
    option(BUILD_ADDONMGR "Build the FreeCAD addon manager module" ON)
    option(BUILD_BIM "Build the FreeCAD BIM module" ON)
    option(BUILD_DRAFT "Build the FreeCAD draft module" ON)
    option(BUILD_DRAWING "Build the FreeCAD drawing module" OFF)
    option(BUILD_HELP "Build the FreeCAD help module" ON)
    option(BUILD_IDF "Build the FreeCAD idf module" ON)
    option(BUILD_IMPORT "Build the FreeCAD import module" ON)
    option(BUILD_INSPECTION "Build the FreeCAD inspection module" ON)
    option(BUILD_JTREADER "Build the FreeCAD jt reader module" OFF)
    option(BUILD_MATERIAL "Build the FreeCAD material module" ON)
    option(BUILD_MESH "Build the FreeCAD mesh module" ON)
    option(BUILD_MESH_PART "Build the FreeCAD mesh part module" ON)
    option(BUILD_FLAT_MESH "Build the FreeCAD flat mesh module" ON)
    option(BUILD_OPENSCAD "Build the FreeCAD openscad module" ON)
    option(BUILD_PART "Build the FreeCAD part module" ON)
    option(BUILD_PART_DESIGN "Build the FreeCAD part design module" ON)
    option(BUILD_CAM "Build the FreeCAD CAM module" ON)
    option(BUILD_ASSEMBLY "Build the FreeCAD Assembly module" ON)
    option(BUILD_PLOT "Build the FreeCAD plot module" ON)
    option(BUILD_POINTS "Build the FreeCAD points module" ON)
    option(BUILD_REVERSEENGINEERING "Build the FreeCAD reverse engineering module" ON)
    option(BUILD_ROBOT "Build the FreeCAD robot module" ON)
    option(BUILD_SHOW "Build the FreeCAD Show module (helper module for visibility automation)" ON)
    option(BUILD_SKETCHER "Build the FreeCAD sketcher module" ON)
    option(BUILD_SPREADSHEET "Build the FreeCAD spreadsheet module" ON)
    option(BUILD_START "Build the FreeCAD start module" ON)
    option(BUILD_TEST "Build the FreeCAD test module" ON)
    option(BUILD_MEASURE "Build the FreeCAD Measure module" ON)
    option(BUILD_TECHDRAW "Build the FreeCAD Technical Drawing module" ON)
    option(BUILD_TUX "Build the FreeCAD Tux module" ON)
    option(BUILD_WEB "Build the FreeCAD Web module" ON)
    option(BUILD_SURFACE "Build the FreeCAD surface module" ON)
    option(BUILD_VR "Build the FreeCAD Oculus Rift support (need Oculus SDK 4.x or higher)" OFF)
    option(BUILD_CLOUD "Build the FreeCAD cloud module" OFF)
    option(ENABLE_DEVELOPER_TESTS "Build the FreeCAD unit tests suit" ON)

    if(MSVC)
        set(FREECAD_3DCONNEXION_SUPPORT "NavLib" CACHE STRING "Select version of the 3Dconnexion device integration")
        set_property(CACHE FREECAD_3DCONNEXION_SUPPORT PROPERTY STRINGS "NavLib" "Raw input" "Both")
        option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
    elseif(APPLE)
        set(FREECAD_USE_3DCONNEXION_RAWINPUT ON)
        find_library(3DCONNEXIONCLIENT_FRAMEWORK 3DconnexionClient)
        if(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
            option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
        else(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
            option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." OFF)
        endif(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
    else(MSVC)
        set(FREECAD_USE_3DCONNEXION_RAWINPUT ON)
        set(FREECAD_USE_3DCONNEXION OFF )
    endif(MSVC)

    if(FREECAD_3DCONNEXION_SUPPORT STREQUAL "NavLib" AND FREECAD_USE_3DCONNEXION)
        set(FREECAD_USE_3DCONNEXION_NAVLIB ON)
        set(FREECAD_USE_3DCONNEXION OFF)
    elseif(FREECAD_3DCONNEXION_SUPPORT STREQUAL "Both" AND FREECAD_USE_3DCONNEXION)
        set(FREECAD_USE_3DCONNEXION_NAVLIB ON)
        set(FREECAD_USE_3DCONNEXION_RAWINPUT ON)
    endif()

    if(MSVC)
        option(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" ON)
        option(FREECAD_USE_PCL "Build the features that use PCL libs" OFF)
    endif(MSVC)
    if(NOT MSVC)
        option(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" OFF)
        option(FREECAD_USE_PCL "Build the features that use PCL libs" OFF)
    endif(NOT MSVC)

    # if this is set override some options
    if (FREECAD_BUILD_DEBIAN)
        # Disable it until the upstream package has been fixed. See
        # https://github.com/FreeCAD/FreeCAD/issues/13676#issuecomment-2539978468
        # https://github.com/FreeCAD/FreeCAD/issues/13676#issuecomment-2541513308
        set(FREECAD_USE_EXTERNAL_ZIPIOS OFF )
        # A Debian package for SMESH doesn't exist
        #set(FREECAD_USE_EXTERNAL_SMESH ON )
    endif (FREECAD_BUILD_DEBIAN)

    if(BUILD_FEM OR BUILD_MESH_PART)
        set(FREECAD_USE_SMESH ON)
        if(FREECAD_USE_EXTERNAL_SMESH)
            set(BUILD_SMESH OFF)
        else()
            set(BUILD_SMESH ON)
        endif()
    else()
        set(FREECAD_USE_SMESH OFF)
        set(BUILD_SMESH OFF)
    endif()

    # force build directory to be different to source directory
    if (BUILD_FORCE_DIRECTORY)
        if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
            message(FATAL_ERROR "The build directory (${CMAKE_BINARY_DIR}) must be different to the source directory (${CMAKE_SOURCE_DIR}).\n"
                                "Please choose another build directory! Or disable the option BUILD_FORCE_DIRECTORY.")
        endif()
    endif()
endmacro(InitializeFreeCADBuildOptions)
