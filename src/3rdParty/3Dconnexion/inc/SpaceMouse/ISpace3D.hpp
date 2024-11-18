#ifndef ISpace3D_HPP_INCLUDED
#define ISpace3D_HPP_INCLUDED
// <copyright file="ISpace3D.hpp" company="3Dconnexion">
// ------------------------------------------------------------------------------------------------
// This file is part of the FreeCAD CAx development system.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU Library General Public License, (see "LICENSE").
// ------------------------------------------------------------------------------------------------
// </copyright>
// <history>
// ************************************************************************************************
// File History
//
// $Id: ISpace3D.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The interface to access the client coordinate system.
/// </summary>
class ISpace3D {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~ISpace3D() = default;
#else
  virtual ~ISpace3D() = 0 {
  }
#endif

  /// <summary>
  /// Gets the coordinate system used by the client.
  /// </summary>
  /// <param name="matrix">The coordinate system <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>The matrix describes the applications coordinate frame in the navlib coordinate
  /// system. i.e. the application to navlib transform.</remarks>
  virtual long GetCoordinateSystem(navlib::matrix_t &matrix) const = 0;

  /// <summary>
  /// Gets the orientation of the front view.
  /// </summary>
  /// <param name="matrix">The front view transform <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetFrontView(navlib::matrix_t &matrix) const = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // ISpace3D_HPP_INCLUDED
