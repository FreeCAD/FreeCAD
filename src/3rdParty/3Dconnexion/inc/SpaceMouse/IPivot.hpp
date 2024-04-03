#ifndef IPivot_HPP_INCLUDED
#define IPivot_HPP_INCLUDED
// <copyright file="IPivot.hpp" company="3Dconnexion">
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
// $Id: IPivot.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
  /// <summary>
  /// The interface to access the pivot.
  /// </summary>
class IPivot {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~IPivot() = default;
#else
  virtual ~IPivot() = 0 {
  }
#endif

  /// <summary>
  /// Gets the position of the rotation pivot.
  /// </summary>
  /// <param name="position">The pivot <see cref="navlib::point_t"/> in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetPivotPosition(navlib::point_t &position) const = 0;

  /// <summary>
  /// Queries if the user has manually set a pivot point.
  /// </summary>
  /// <param name="userPivot">true if the user has set a pivot otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long IsUserPivot(navlib::bool_t &userPivot) const = 0;

  /// <summary>
  /// Sets the position of the rotation pivot.
  /// </summary>
  /// <param name="position">The pivot <see cref="navlib::point_t"/> in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetPivotPosition(const navlib::point_t& position) = 0;

  /// <summary>
  /// Queries the visibility of the pivot image.
  /// </summary>
  /// <param name="visible">true if the pivot is visible otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetPivotVisible(navlib::bool_t &visible) const = 0;

  /// <summary>
  /// Sets the visibility of the pivot image.
  /// </summary>
  /// <param name="visible">true if the pivot is visible otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetPivotVisible(bool visible) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IPivot_HPP_INCLUDED
