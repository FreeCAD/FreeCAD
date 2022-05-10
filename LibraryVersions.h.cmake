#ifndef LIBRARY_VERSIONS_H
#define LIBRARY_VERSIONS_H

/* LibraryVersions.h.  Generated from LibraryVersions.h.cmake by cmake.  */

#cmakedefine HAVE_OCC_VERSION

#if defined(HAVE_OCC_VERSION)
#  include <${OCC_INCLUDE_DIR}/Standard_Version.hxx>
#endif


/* No need for Boost (version info is always included in the source) */
/* No need for Python (version info is always included in the source) */
/* No need for Qt (version info is always included in the source) */
/* No need for zlib (version info is always included in the source) */

// No CMake version info for SMESH, libarea, Zipios, or KDL

// Coin3D
constexpr char * FC_COIN3D_VERSION = "${COIN3D_VERSION}";

// Eigen3
constexpr char * FC_EIGEN3_VERSION = "${EIGEN3_VERSION}";

// FreeType
constexpr char * FC_FREETYPE_VERSION = "${FREETYPE_VERSION_STRING}";

// pcl
constexpr char * FC_PCL_VERSION = "${PCL_VERSION}";

// PyCXX
constexpr char * FC_PYCXX_VERSION = "${PYCXX_VERSION}";

// PySide
constexpr char * FC_PYSIDE_VERSION = "${PySide2_VERSION}";

// PyBind
constexpr char * FC_PYBIND11_VERSION = "${pybind11_VERSION}";

// Shiboken
constexpr char * FC_SHIBOKEN_VERSION = "${Shiboken2_VERSION}";

// vtk
constexpr char * FC_VTK_VERSION = "${VTK_VERSION}";

// Xerces-C
constexpr char * FC_XERCESC_VERSION = "${XercesC_VERSION}";

// Pivy
constexpr char * FC_PIVY_VERSION = "${PIVY_VERSION}";


#endif // LIBRARY_VERSIONS_H

