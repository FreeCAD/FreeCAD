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
    option(FREECAD_USE_EXTERNAL_E57FORMAT "Use system installed libE57Format instead of the bundled." OFF)
    option(FREECAD_USE_EXTERNAL_GTEST "Use system installed Google Test and Google Mock" OFF)
    option(FREECAD_USE_FREETYPE "Builds the features using FreeType libs" ON)
    option(FREECAD_BUILD_DEBIAN "Prepare for a build of a Debian package" OFF)
    option(FREECAD_CHECK_PIVY "Check for pivy version using Python at build time" ON)
    option(FREECAD_PARALLEL_COMPILE_JOBS "Compilation jobs pool size to fit memory limitations.")
    option(FREECAD_PARALLEL_LINK_JOBS "Linkage jobs pool size to fit memory limitations.")
    option(BUILD_WITH_CONDA "Set ON if you build FreeCAD with conda" OFF)
    option(BUILD_DYNAMIC_LINK_PYTHON "If OFF extension-modules do not link against python-libraries" ON)
    option(BUILD_TRACY_FRAME_PROFILER "If ON then enables support for the Tracy frame profiler" OFF)

    option(INSTALL_TO_SITEPACKAGES "If ON the freecad root namespace (python) is installed into python's site-packages" ON)
    option(INSTALL_PREFER_SYMLINKS "If ON then fc_copy_sources macro will create symlinks instead of copying files" OFF)
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
    option(BUILD_MATERIAL_EXTERNAL "Build the FreeCAD material external interface module" OFF)
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

    if(MSVC OR APPLE)
        set(FREECAD_3DCONNEXION_SUPPORT "NavLib" CACHE STRING "Select version of the 3Dconnexion device integration")
        set_property(CACHE FREECAD_3DCONNEXION_SUPPORT PROPERTY STRINGS "None" "NavLib" "Legacy" "Both")
    else(MSVC OR APPLE)
        option(FREECAD_USE_3DCONNEXION_LEGACY "Enable support for 3Dconnexion devices." ON)
    endif(MSVC OR APPLE)

    if(FREECAD_3DCONNEXION_SUPPORT STREQUAL "NavLib")
        set(FREECAD_USE_3DCONNEXION_NAVLIB ON)
    elseif(FREECAD_3DCONNEXION_SUPPORT STREQUAL "Both")
        set(FREECAD_USE_3DCONNEXION_NAVLIB ON)
        set(FREECAD_USE_3DCONNEXION_LEGACY ON)
    elseif(FREECAD_3DCONNEXION_SUPPORT STREQUAL "Legacy")
        set(FREECAD_USE_3DCONNEXION_LEGACY ON)
    elseif(FREECAD_3DCONNEXION_SUPPORT STREQUAL "None")
        set(FREECAD_USE_3DCONNEXION_NAVLIB OFF)
        set(FREECAD_USE_3DCONNEXION_LEGACY OFF)
    endif()

    if(APPLE AND FREECAD_USE_3DCONNEXION_LEGACY)
        find_library(3DCONNEXIONCLIENT_FRAMEWORK 3DconnexionClient)
        if(NOT (IS_DIRECTORY ${3DCONNEXIONCLIENT_FRAMEWORK}))
            set(FREECAD_USE_3DCONNEXION_LEGACY OFF)
        endif()
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

    if (BUILD_CAM OR BUILD_FLAT_MESH)
        set(FREECAD_USE_PYBIND11 ON)
    endif()

    # force build directory to be different to source directory
    if (BUILD_FORCE_DIRECTORY)
        if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
            message(FATAL_ERROR "The build directory (${CMAKE_BINARY_DIR}) must be different to the source directory (${CMAKE_SOURCE_DIR}).\n"
                                "Please choose another build directory! Or disable the option BUILD_FORCE_DIRECTORY.")
        endif()
    endif()

    if(FREECAD_PARALLEL_COMPILE_JOBS)
        if(CMAKE_GENERATOR MATCHES "Ninja")
            set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=${FREECAD_PARALLEL_COMPILE_JOBS})
            set(CMAKE_JOB_POOL_COMPILE compile_job_pool)
        else()
            message(WARNING "Job pooling is only available with Ninja generators.")
        endif()
    endif()

    if(FREECAD_PARALLEL_LINK_JOBS)
        if(CMAKE_GENERATOR MATCHES "Ninja")
            set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${FREECAD_PARALLEL_LINK_JOBS})
            set(CMAKE_JOB_POOL_LINK link_job_pool)
        else()
            message(WARNING "Job pooling is only available with Ninja generators.")
        endif()
    endif()
endmacro(InitializeFreeCADBuildOptions)
