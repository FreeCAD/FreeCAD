macro(InitializeFreeCADBuildOptions)
    # ==============================================================================
    # =================   All the options for the build process    =================
    # ==============================================================================

    option(BUILD_FORCE_DIRECTORY "The build directory must be different to the source directory." OFF)
    option(BUILD_GUI "Build FreeCAD Gui. Otherwise you have only the command line and the Python import module." ON)
    option(FREECAD_USE_EXTERNAL_ZIPIOS "Use system installed zipios++ instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_SMESH "Use system installed smesh instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_KDL "Use system installed orocos-kdl instead of the bundled." OFF)
    option(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
    option(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)
    option(BUILD_WITH_CONDA "Set ON if you build FreeCAD with conda" OFF)
    option(BUILD_DYNAMIC_LINK_PYTHON "If OFF extension-modules do not link against python-libraries" ON)
    option(INSTALL_TO_SITEPACKAGES "If ON the freecad root namespace (python) is installed into python's site-packages" OFF)
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
        option(FREECAD_LIBPACK_USEPYSIDE "Use PySide in LibPack rather to PyQt and Swig." ON)
        option(FREECAD_USE_PCH "Activate precompiled headers where it's used." ON)

        if (DEFINED ENV{FREECAD_LIBPACK_DIR})
            set(FREECAD_LIBPACK_DIR $ENV{FREECAD_LIBPACK_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
            message(STATUS "Found libpack env variable: ${FREECAD_LIBPACK_DIR}")
            if (EXISTS ${FREECAD_LIBPACK_DIR}/lib/Qt5Core.lib)
                option(BUILD_QT5 "Build with Qt5." ON)
            else()
                option(BUILD_QT5 "Build with Qt5." OFF)
            endif()
        else()
            set(FREECAD_LIBPACK_DIR ${CMAKE_SOURCE_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")
            if (EXISTS ${FREECAD_LIBPACK_DIR}/lib/Qt5Core.lib)
                option(BUILD_QT5 "Build with Qt5." ON)
            else()
                option(BUILD_QT5 "Build with Qt5." OFF)
            endif()
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
            message("Libpack NOT found.\nIf you intend to use a Windows libpack, set the FREECAD_LIBPACK_DIR to the libpack directory.")
            message(STATUS "Visit: https://github.com/apeltauer/FreeCAD/releases/ for Windows libpack downloads.")
        endif()
    else(MSVC)
        option(BUILD_QT5 "Build with Qt5." OFF)
        option(FREECAD_LIBPACK_USE "Use the LibPack to Build FreeCAD (only Win32 so far)." OFF)
        set(FREECAD_LIBPACK_DIR ""  CACHE PATH  "Directory of the FreeCAD LibPack")
    endif(MSVC)

    # https://blog.kitware.com/constraining-values-with-comboboxes-in-cmake-cmake-gui/
    set(FREECAD_USE_OCC_VARIANT "Community Edition"  CACHE STRING  "Official OpenCASCADE version or community edition")
    set_property(CACHE FREECAD_USE_OCC_VARIANT PROPERTY STRINGS
                 "Official Version"
                 "Community Edition"
    )

    if (BUILD_QT5)
      option(FREECAD_USE_QTOPENGL_WIDGET "Replace QGLWidget with QOpenGLWidget." ON)
      if (FREECAD_USE_QTOPENGL_WIDGET)
          set(HAVE_QT5_OPENGL 1 )
      endif()
      set(FREECAD_USE_QTWEBMODULE "Automatic"  CACHE STRING  "Qt Webkit or Qt WebEngine")
      set_property(CACHE FREECAD_USE_QTWEBMODULE PROPERTY STRINGS
                   "Automatic"
                   "Qt Webkit"
                   "Qt WebEngine"
      )
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/src/QtOpenGL.h.cmake ${CMAKE_BINARY_DIR}/src/QtOpenGL.h)

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
    option(BUILD_ARCH "Build the FreeCAD Architecture module" ON)
    option(BUILD_ASSEMBLY "Build the FreeCAD Assembly module" OFF)
    option(BUILD_COMPLETE "Build the FreeCAD complete module" OFF)
    option(BUILD_DRAFT "Build the FreeCAD draft module" ON)
    option(BUILD_DRAWING "Build the FreeCAD drawing module" ON)
    option(BUILD_IDF "Build the FreeCAD idf module" ON)
    option(BUILD_IMAGE "Build the FreeCAD image module" ON)
    option(BUILD_IMPORT "Build the FreeCAD import module" ON)
    option(BUILD_INSPECTION "Build the FreeCAD inspection module" ON)
    option(BUILD_JTREADER "Build the FreeCAD jt reader module" OFF)
    option(BUILD_MATERIAL "Build the FreeCAD material module" ON)
    option(BUILD_MESH "Build the FreeCAD mesh module" ON)
    option(BUILD_MESH_PART "Build the FreeCAD mesh part module" ON)
    option(BUILD_FLAT_MESH "Build the FreeCAD flat mesh module" OFF)
    option(BUILD_OPENSCAD "Build the FreeCAD openscad module" ON)
    option(BUILD_PART "Build the FreeCAD part module" ON)
    option(BUILD_PART_DESIGN "Build the FreeCAD part design module" ON)
    option(BUILD_PATH "Build the FreeCAD path module" ON)
    option(BUILD_PLOT "Build the FreeCAD plot module" OFF)
    option(BUILD_POINTS "Build the FreeCAD points module" ON)
    option(BUILD_RAYTRACING "Build the FreeCAD ray tracing module" ON)
    option(BUILD_REVERSEENGINEERING "Build the FreeCAD reverse engineering module" ON)
    option(BUILD_ROBOT "Build the FreeCAD robot module" ON)
    option(BUILD_SHIP "Build the FreeCAD ship module" OFF)
    option(BUILD_SHOW "Build the FreeCAD Show module (helper module for visibility automation)" ON)
    option(BUILD_SKETCHER "Build the FreeCAD sketcher module" ON)
    option(BUILD_SPREADSHEET "Build the FreeCAD spreadsheet module" ON)
    option(BUILD_START "Build the FreeCAD start module" ON)
    option(BUILD_TEST "Build the FreeCAD test module" ON)
    option(BUILD_TECHDRAW "Build the FreeCAD Technical Drawing module" ON)
    option(BUILD_TUX "Build the FreeCAD Tux module" ON)
    option(BUILD_WEB "Build the FreeCAD web module" ON)
    option(BUILD_SURFACE "Build the FreeCAD surface module" ON)
    option(BUILD_VR "Build the FreeCAD Oculus Rift support (need Oculus SDK 4.x or higher)" OFF)
    option(BUILD_CLOUD "Build the FreeCAD cloud module" OFF)

    if(MSVC)
        option(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" ON)
        option(FREECAD_USE_PCL "Build the features that use PCL libs" ON)
        option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
    elseif(APPLE)
        find_library(3DCONNEXIONCLIENT_FRAMEWORK 3DconnexionClient)
        if(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
            option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." ON)
        else(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
            option(FREECAD_USE_3DCONNEXION "Use the 3D connexion SDK to support 3d mouse." OFF)
        endif(IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK})
    else(MSVC)
        set(FREECAD_USE_3DCONNEXION OFF )
    endif(MSVC)
    if(NOT MSVC)
        option(BUILD_FEM_NETGEN "Build the FreeCAD FEM module with the NETGEN mesher" OFF)
        option(FREECAD_USE_PCL "Build the features that use PCL libs" OFF)
    endif(NOT MSVC)

    # if this is set override some options
    if (FREECAD_BUILD_DEBIAN)
        set(FREECAD_USE_EXTERNAL_ZIPIOS ON )
        # A Debian package for SMESH doesn't exist
        #set(FREECAD_USE_EXTERNAL_SMESH ON )
    endif (FREECAD_BUILD_DEBIAN)

    if(BUILD_FEM)
        set(BUILD_SMESH ON )
    endif()

    # for Windows the minimum required cmake version is 3.4.3 to build the Path module
    if(WIN32 AND CMAKE_VERSION VERSION_LESS 3.4.3)
        message(WARNING "Disable Path, requires cmake >= 3.4.3 in order to build this module")
        set(BUILD_PATH OFF )
    endif()

    # force build directory to be different to source directory
    if (BUILD_FORCE_DIRECTORY)
        if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
            message(FATAL_ERROR "The build directory (${CMAKE_BINARY_DIR}) must be different to the source directory (${CMAKE_SOURCE_DIR}).\n"
                                "Please choose another build directory! Or disable the option BUILD_FORCE_DIRECTORY.")
        endif()
    endif()
endmacro(InitializeFreeCADBuildOptions)
