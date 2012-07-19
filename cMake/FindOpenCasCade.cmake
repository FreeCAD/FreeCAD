# Try to find OCC
# Once done this will define
#
# OCC_FOUND        - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR  - where the OCC include directory can be found
# OCC_LIBRARY_DIR  - where the OCC library directory can be found
# OCC_LIBRARIES    - Link this to use OCC
#
 

IF (WIN32)
  IF (CYGWIN OR MINGW)

    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
      /usr/include/opencascade
      /usr/local/include/opencascade
    )

    FIND_LIBRARY(OCC_LIBRARY TKernel
      /usr/lib
      /usr/local/lib
      /opt/opencascade/lib
    )

  ELSE (CYGWIN OR MINGW)

    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/include"
    )

    FIND_LIBRARY(OCC_LIBRARY TKernel
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/lib"
    )

  ENDIF (CYGWIN OR MINGW)

ELSE (WIN32)

  FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
    /usr/include/opencascade
    /usr/local/include/opencascade
    /opt/opencascade/include
  )

  FIND_LIBRARY(OCC_LIBRARY TKernel
    /usr/lib
    /usr/local/lib
    /opt/opencascade/lib
  )

ENDIF (WIN32)


SET(OCC_FOUND FALSE)
IF(OCC_LIBRARY)
  GET_FILENAME_COMPONENT(OCC_LIBRARY_DIR ${OCC_LIBRARY} PATH)
  SET(OCC_FOUND TRUE)
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
ENDIF(OCC_LIBRARY)

