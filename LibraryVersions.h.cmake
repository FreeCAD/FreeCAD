#ifndef LIBRARY_VERSIONS_H
#define LIBRARY_VERSIONS_H

/* LibraryVersions.h.  Generated from LibraryVersions.h.cmake by cmake.  */

#cmakedefine HAVE_OCC_VERSION

#if defined(HAVE_OCC_VERSION)
#  include <${OCC_INCLUDE_DIR}/Standard_Version.hxx>
#endif

#cmakedefine BUILD_SMESH

#if defined(BUILD_SMESH)
#  include <SMESH_Version.h>
#endif


/* No need for Boost (version info is always included in the source) */
/* No need for Python (version info is always included in the source) */
/* No need for Qt (version info is always included in the source) */
/* No need for SMESH (version info is always included in the source) */
/* No need for zlib (version info is always included in the source) */

// No CMake version info for libarea, Zipios, or KDL

// Coin3D
constexpr char * fcCoin3dVersion = "${COIN3D_VERSION}";

// Eigen3
constexpr char * fcEigen3Version = "${EIGEN3_VERSION}";

// FreeType
constexpr char * fcFreetypeVersion = "${FREETYPE_VERSION_STRING}";

// pcl
constexpr char * fcPclVersion = "${PCL_VERSION}";

// PyCXX
constexpr char * fcPycxxVersion = "${PYCXX_VERSION}";

// PySide
constexpr char * fcPysideVersion = "${PySide2_VERSION}";

// PyBind
constexpr char * fcPybind11Version = "${pybind11_VERSION}";

// Shiboken
constexpr char * fcShibokenVersion = "${Shiboken2_VERSION}";

// vtk
constexpr char * fcVtkVersion = "${VTK_VERSION}";

// Xerces-C
constexpr char * fcXercescVersion = "${XercesC_VERSION}";

// Pivy
constexpr char * fcPivyVersion = "${PIVY_VERSION}";


#endif // LIBRARY_VERSIONS_H
