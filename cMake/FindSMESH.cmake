# Try to find Salome SMESH Once done this will define
#
# SMESH_FOUND         - system has Salome SMESH SMESH_INCLUDE_DIR   - where the Salome SMESH include
# directory can be found SMESH_LIBRARIES     - Link this to use Salome SMESH
#

# SMESH needs VTK
find_package (VTK REQUIRED)

# If this definition is not set, linker errors will occur against SMESH on 64 bit machines.
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions (-DSALOME_USE_64BIT_IDS)
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

if (CMAKE_COMPILER_IS_GNUCC)
    find_path (SMESH_INCLUDE_DIR SMESH_Mesh.hxx # These are default search paths, why specify them?
               PATH_SUFFIXES smesh SMESH smesh/SMESH)
    find_library (SMESH_LIBRARY SMESH)
else (CMAKE_COMPILER_IS_GNUCC)
    # Not yet implemented
endif (CMAKE_COMPILER_IS_GNUCC)

if (SMESH_INCLUDE_DIR)
    set (SMESH_INC_ROOT "${SMESH_INCLUDE_DIR}/..")
    # Append extra include dirs
    set (
        SMESH_INCLUDE_DIR
        "${SMESH_INCLUDE_DIR};
	${SMESH_INC_ROOT}/Controls;
	${SMESH_INC_ROOT}/Driver;
	${SMESH_INC_ROOT}/DriverDAT;
	${SMESH_INC_ROOT}/DriverGMF;
	${SMESH_INC_ROOT}/DriverSTL;
	${SMESH_INC_ROOT}/DriverUNV;
	${SMESH_INC_ROOT}/Geom;
	${SMESH_INC_ROOT}/Kernel;
	${SMESH_INC_ROOT}/MEFISTO2;
	${SMESH_INC_ROOT}/MeshVSLink;
	${SMESH_INC_ROOT}/Netgen;
	${SMESH_INC_ROOT}/NETGENPlugin;
	${SMESH_INC_ROOT}/SMDS;
	${SMESH_INC_ROOT}/SMESHDS;
	${SMESH_INC_ROOT}/SMESHUtils;
	${SMESH_INC_ROOT}/StdMeshers;")
else (SMESH_INCLUDE_DIR)
    message (FATAL_ERROR "SMESH include directories not found!")
endif (SMESH_INCLUDE_DIR)

set (SMESH_FOUND FALSE)
if (SMESH_LIBRARY)
    set (SMESH_FOUND TRUE)
    get_filename_component (SMESH_LIBRARY_DIR ${SMESH_LIBRARY} PATH)
    set (
        SMESH_LIBRARIES
        ${SMESH_LIBRARY_DIR}/libDriver.so
        ${SMESH_LIBRARY_DIR}/libDriverDAT.so
        ${SMESH_LIBRARY_DIR}/libDriverSTL.so
        ${SMESH_LIBRARY_DIR}/libDriverUNV.so
        ${SMESH_LIBRARY_DIR}/libSMDS.so
        ${SMESH_LIBRARY_DIR}/libSMESH.so
        ${SMESH_LIBRARY_DIR}/libSMESHDS.so
        ${SMESH_LIBRARY_DIR}/libStdMeshers.so)
    set (EXTERNAL_SMESH_LIBS ${SMESH_LIBRARIES})
else (SMESH_LIBRARY)
    message (FATAL_ERROR "SMESH libraries NOT FOUND!")
endif (SMESH_LIBRARY)
