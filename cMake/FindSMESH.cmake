# Try to find Salome SMESH
# Once done this will define
#
# SMESH_FOUND         - system has Salome SMESH
# SMESH_INCLUDE_DIR   - where the Salome SMESH include directory can be found
# SMESH_LIBRARIES     - Link this to use Salome SMESH
#
 

IF (CMAKE_COMPILER_IS_GNUCC)
    FIND_PATH(SMESH_INCLUDE_DIR SMESH_Mesh.hxx
     /usr/include
     /usr/local/include
    )
    FIND_LIBRARY(SMESH_LIBRARY SMESH
      /usr/lib
      /usr/local/lib
    )
ELSE (CMAKE_COMPILER_IS_GNUCC)
    # Not yet implemented
ENDIF (CMAKE_COMPILER_IS_GNUCC)

SET(SMESH_FOUND FALSE)
IF(SMESH_LIBRARY)
  SET(SMESH_FOUND TRUE)
  GET_FILENAME_COMPONENT(SMESH_LIBRARY_DIR ${SMESH_LIBRARY} PATH)
  set(SMESH_LIBRARIES
    ${SMESH_LIBRARY_DIR}/libDriver.so
    ${SMESH_LIBRARY_DIR}/libDriverDAT.so
    ${SMESH_LIBRARY_DIR}/libDriverSTL.so
    ${SMESH_LIBRARY_DIR}/libDriverUNV.so
    ${SMESH_LIBRARY_DIR}/libSMDS.so
    ${SMESH_LIBRARY_DIR}/libSMESH.so
    ${SMESH_LIBRARY_DIR}/libSMESHDS.so
    ${SMESH_LIBRARY_DIR}/libStdMeshers.so
  )
ENDIF(SMESH_LIBRARY)

