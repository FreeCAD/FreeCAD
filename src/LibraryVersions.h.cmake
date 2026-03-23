// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

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
constexpr const char * fcCoin3dVersion = "${COIN3D_VERSION}";

// Eigen3
constexpr const char * fcEigen3Version = "${EIGEN3_VERSION}";

// FreeType
constexpr const char * fcFreetypeVersion = "${FREETYPE_VERSION_STRING}";

// pcl
constexpr const char * fcPclVersion = "${PCL_VERSION}";

// PyCXX
constexpr const char * fcPycxxVersion = "${PYCXX_VERSION}";

// PySide
constexpr const char * fcPysideVersion = "${PySide_VERSION}";

// PyBind
constexpr const char * fcPybind11Version = "${pybind11_VERSION}";

// Shiboken
constexpr const char * fcShibokenVersion = "${Shiboken_VERSION}";

// vtk
constexpr const char * fcVtkVersion = "${VTK_VERSION}";

// Xerces-C
constexpr const char * fcXercescVersion = "${XercesC_VERSION}";

// Pivy
constexpr const char * fcPivyVersion = "${PIVY_VERSION}";

