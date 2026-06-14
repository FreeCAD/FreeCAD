# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework

# we first try to find opencascade directly:
if (NOT OCCT_CMAKE_FALLBACK)
    find_package(OpenCASCADE CONFIG QUIET)
    get_property(flags DIRECTORY PROPERTY COMPILE_DEFINITIONS)
    # OCCT 7.5 adds this define that causes hundreds of compiler warnings with Qt5.x, so remove it again
    list(FILTER flags EXCLUDE REGEX [[GL_GLEXT_LEGACY]])
    set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS ${flags})
endif ()
if (OpenCASCADE_FOUND)
    set(OCC_FOUND ${OpenCASCADE_FOUND})
    set(OCC_INCLUDE_DIR ${OpenCASCADE_INCLUDE_DIR})
    set(OCC_LIBRARY_DIR ${OpenCASCADE_LIBRARY_DIR})
    set(OCC_LIBRARIES ${OpenCASCADE_LIBRARIES})
    set(OCC_OCAF_LIBRARIES TKCAF TKXCAF)
else ()
    if (WIN32)
        if (CYGWIN OR MINGW)
            find_path(OCC_INCLUDE_DIR Standard_Version.hxx
                    /usr/include/opencascade
                    /usr/local/include/opencascade
                    /opt/opencascade/include
                    /opt/opencascade/inc
            )
            find_path(OCC_LIBRARY TKernel
                    /usr/lib
                    /usr/local/lib
                    /opt/opencascade/lib
            )
        else ()
            find_path(OCC_INCLUDE_DIR Standard_Version.hxx
                    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/include"
            )
            find_library(OCC_LIBRARY TKernel
                    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/lib"
            )
        endif ()
    else ()
        find_path(OCC_INCLUDE_DIR Standard_Version.hxx
                /usr/include/occt
                /usr/include/opencascade
                /usr/local/include/opencascade
                /opt/opencascade/include
                /opt/opencascade/inc
        )
        find_library(OCC_LIBRARY TKernel
                /usr/lib
                /usr/local/lib
                /opt/opencascade/lib
        )
    endif ()
    if (OCC_LIBRARY)
        get_filename_component(OCC_LIBRARY_DIR ${OCC_LIBRARY} PATH)
        if (NOT OCC_INCLUDE_DIR)
            find_path(OCC_INCLUDE_DIR Standard_Version.hxx
                    ${OCC_LIBRARY_DIR}/../inc
            )
        endif ()
    endif ()
endif ()

if (OCC_INCLUDE_DIR)
    file(STRINGS ${OCC_INCLUDE_DIR}/Standard_Version.hxx OCC_MAJOR
            REGEX "#define OCC_VERSION_MAJOR.*"
    )
    string(REGEX MATCH "[0-9]+" OCC_MAJOR ${OCC_MAJOR})
    file(STRINGS ${OCC_INCLUDE_DIR}/Standard_Version.hxx OCC_MINOR
            REGEX "#define OCC_VERSION_MINOR.*"
    )
    string(REGEX MATCH "[0-9]+" OCC_MINOR ${OCC_MINOR})
    file(STRINGS ${OCC_INCLUDE_DIR}/Standard_Version.hxx OCC_MAINT
            REGEX "#define OCC_VERSION_MAINTENANCE.*"
    )
    string(REGEX MATCH "[0-9]+" OCC_MAINT ${OCC_MAINT})

    set(OCC_VERSION_STRING "${OCC_MAJOR}.${OCC_MINOR}.${OCC_MAINT}")
endif ()

# handle the QUIETLY and REQUIRED arguments and set OCC_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OCC REQUIRED_VARS OCC_INCLUDE_DIR VERSION_VAR OCC_VERSION_STRING)

if (OCC_FOUND)
    set(OCC_LIBRARIES
            TKFillet
            TKMesh
            TKernel
            TKG2d
            TKG3d
            TKMath
            TKShHealing
            TKXSBase
            TKBool
            TKBO
            TKBRep
            TKTopAlgo
            TKGeomAlgo
            TKGeomBase
            TKOffset
            TKPrim
            TKHLR
            TKFeat
    )
    if (OCC_VERSION_STRING VERSION_GREATER_EQUAL 7.9.0)
        list(APPEND OCC_LIBRARIES TKExpress)
    endif ()

    set(OCC_OCAF_LIBRARIES
            TKBin
            TKBinL
            TKCAF
            TKXCAF
            TKLCAF
            TKVCAF
            TKCDF
            TKMeshVS
            TKService
            TKV3d
            TKRWMesh
    )

    if (OCC_VERSION_STRING VERSION_LESS 7.8.0)
        list(APPEND OCC_LIBRARIES TKIGES TKSTL TKSTEPBase TKSTEPAttr TKSTEP209 TKSTEP)
        list(APPEND OCC_OCAF_LIBRARIES TKXDESTEP TKXDEIGES)
    else ()
        list(APPEND OCC_LIBRARIES TKDESTEP TKDEIGES TKDEGLTF TKDESTL)
    endif ()
    message(STATUS "Found OpenCASCADE version: ${OCC_VERSION_STRING}")
    message(STATUS "  OpenCASCADE include directory: ${OCC_INCLUDE_DIR}")
    message(STATUS "  OpenCASCADE shared libraries directory: ${OCC_LIBRARY_DIR}")
endif ()
