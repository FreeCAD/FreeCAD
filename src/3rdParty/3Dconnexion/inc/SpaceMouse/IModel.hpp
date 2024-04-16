#ifndef IModel_HPP_INCLUDED
#define IModel_HPP_INCLUDED
// <copyright file="IModel.hpp" company="3Dconnexion">
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
// $Id: IModel.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The Model interface
/// </summary>
class IModel {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~IModel() = default;
#else
  virtual ~IModel() = 0 {
  }
#endif

  /// <summary>
  /// Is called when the navigation library needs to get the extents of the model.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// model.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetModelExtents(navlib::box_t &extents) const = 0;

  /// <summary>
  /// Is called when the navigation library needs to get the extents of the selection.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetSelectionExtents(navlib::box_t &extents) const = 0;

  /// <summary>
  /// Is called to get the selections's transform <see cref="navlib::matrix_t"/>.
  /// </summary>
  /// <param name="transform">The world affine <see cref="navlib::matrix_t"/> of the
  /// selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetSelectionTransform(navlib::matrix_t &transform) const = 0;

  /// <summary>
  /// Is called to query if the selection is empty.
  /// </summary>
  /// <param name="empty">true if nothing is selected.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetIsSelectionEmpty(navlib::bool_t &empty) const = 0;

  /// <summary>
  /// Is called to set the selections's transform <see cref="navlib::matrix_t"/>.
  /// </summary>
  /// <param name="matrix">The world affine <see cref="navlib::matrix_t"/> of the selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetSelectionTransform(const navlib::matrix_t& matrix) = 0;

  /// <summary>
  /// Is called to retrieve the length of the model/world units in meters.
  /// </summary>
  /// <param name="meters">The length of a model/world unit in meters.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>
  /// <para>The conversion factor is used by the Navigation Library to calculate the height above
  /// the floor in walk mode and the speed in the first-person motion model.</para>
  /// <para>The Navigation Library assumes that this value does not change and it is only queried
  /// once.</para>
  /// </remarks>
  virtual long GetUnitsToMeters(double &meters) const = 0;

  /// <summary>
  /// Is called to retrieve the plane equation of the floor.
  /// </summary>
  /// <param name="floor">The plane equation of the floor plane.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>
  /// <para>The plane equation is used by the Navigation Library to determine the floor for the
  /// walk navigation mode, where the height of the eye is fixed to 1.5m above the floor plane.
  /// The floor need not be parallel to the world ground plane.</para>
  /// </remarks>
  virtual long GetFloorPlane(navlib::plane_t &floor) const = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IModel_HPP_INCLUDED
