# Try to find OCE / OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework

# First try to find OpenCASCADE Community Edition
if(NOT DEFINED OCE_DIR)
  # Check for OSX needs to come first because UNIX evaluates to true on OSX
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(DEFINED MACPORTS_PREFIX)
      find_package(OCE HINTS ${MACPORTS_PREFIX}/Library/Frameworks)
    elseif(DEFINED HOMEBREW_PREFIX)
      find_package(OCE HINTS ${HOMEBREW_PREFIX}/Cellar/oce/*)
    endif()
  elseif(UNIX)
    set(OCE_DIR "/usr/local/share/cmake/")
  elseif(WIN32)
    set(OCE_DIR "c:/OCE-0.4.0/share/cmake")
  endif()
endif()

find_package(OCE QUIET)
if(OCE_FOUND)
  message(STATUS "-- OpenCASCADE Community Edition has been found.")
  # Disable this define. For more details see bug #0001872
  #add_definitions (-DHAVE_CONFIG_H)
  set(OCC_INCLUDE_DIR ${OCE_INCLUDE_DIRS})
  #set(OCC_LIBRARY_DIR ${OCE_LIBRARY_DIR})
else(OCE_FOUND) #look for OpenCASCADE
  if(WIN32)
    if(CYGWIN OR MINGW)
    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
        /usr/include/opencascade
        /usr/local/include/opencascade
        /opt/opencascade/include
        /opt/opencascade/inc
      )
      FIND_LIBRARY(OCC_LIBRARY TKernel
        /usr/lib
        /usr/local/lib
        /opt/opencascade/lib
      )
    else(CYGWIN OR MINGW)
    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/include"
      )
      FIND_LIBRARY(OCC_LIBRARY TKernel
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/lib"
      )
    endif(CYGWIN OR MINGW)
  else(WIN32)
    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
      /usr/include/opencascade
      /usr/local/include/opencascade
      /opt/opencascade/include
      /opt/opencascade/inc
    )
    FIND_LIBRARY(OCC_LIBRARY TKernel
      /usr/lib
      /usr/local/lib
      /opt/opencascade/lib
    )
  endif(WIN32)
  if(OCC_LIBRARY)
    GET_FILENAME_COMPONENT(OCC_LIBRARY_DIR ${OCC_LIBRARY} PATH)
    IF(NOT OCC_INCLUDE_DIR)
      FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
        ${OCC_LIBRARY_DIR}/../inc
      )
    ENDIF()
  endif(OCC_LIBRARY)
endif(OCE_FOUND)

if(OCC_INCLUDE_DIR)
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
endif(OCC_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set OCC_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OCC REQUIRED_VARS OCC_INCLUDE_DIR VERSION_VAR OCC_VERSION_STRING)

if(OCC_FOUND)
  set(OCC_LIBRARIES
    TKFillet
    TKMesh
    TKernel
    TKG2d
    TKG3d
    TKMath
    TKIGES
    TKSTL
    TKShHealing
    TKXSBase
    TKBin
    TKBool
    TKBO
    TKBRep
    TKTopAlgo
    TKGeomAlgo
    TKGeomBase
    TKOffset
    TKPrim
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKHLR
    TKFeat
  )
  set(OCC_OCAF_LIBRARIES
    TKCAF
    TKXCAF
    TKLCAF
    TKXDESTEP
    TKXDEIGES
    TKMeshVS
  )
  if(OCC_VERSION_STRING VERSION_LESS 6.7.3)
    list(APPEND OCC_OCAF_LIBRARIES TKAdvTools)
  endif(OCC_VERSION_STRING VERSION_LESS 6.7.3)
  message(STATUS "-- Found OCE/OpenCASCADE version: ${OCC_VERSION_STRING}")
  message(STATUS "-- OCE/OpenCASCADE include directory: ${OCC_INCLUDE_DIR}")
  message(STATUS "-- OCE/OpenCASCADE shared libraries directory: ${OCC_LIBRARY_DIR}")
else(OCC_FOUND)
  #message(SEND_ERROR "Neither OpenCASCADE Community Edition nor OpenCasCade were found: will not build CAD modules!")
endif(OCC_FOUND)
