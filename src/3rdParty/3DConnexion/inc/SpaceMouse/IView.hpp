#ifndef IView_HPP_INCLUDED
#define IView_HPP_INCLUDED
// <copyright file="IView.hpp" company="3Dconnexion">
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
// $Id: IView.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// View callback interface.
/// </summary>
class IView {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~IView() = default;
#else
  virtual ~IView() = 0 {
  }
#endif

  /// <summary>
  /// Gets the camera matrix of the view.
  /// </summary>
  /// <param name="matrix">The camera/view <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetCameraMatrix(navlib::matrix_t &matrix) const = 0;

  /// <summary>
  /// Gets the camera's target point.
  /// </summary>
  /// <param name="target">The position of the camera target in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>Free cameras do not have a target.</remarks>
  virtual long GetCameraTarget(navlib::point_t &target) const = 0;

  /// <summary>
  ///  Gets the position of the pointer on the near clipping plane.
  /// </summary>
  /// <param name="position">The <see cref="navlib::point_t"/> in world coordinates of the
  /// pointer.</param> <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetPointerPosition(navlib::point_t &position) const = 0;

  /// <summary>
  /// Gets the view's construction plane.
  /// </summary>
  /// <param name="plane">The plane equation of the construction plane.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetViewConstructionPlane(navlib::plane_t &plane) const = 0;

  /// <summary>
  /// Gets the extents of the view.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// view.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetViewExtents(navlib::box_t &extents) const = 0;

  /// <summary>
  /// Gets the camera's/view's distance to the focused object.
  /// </summary>
  /// <param name="fov">The distance in world units.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetViewFocusDistance(double &distance) const = 0;

  /// <summary>
  /// Gets the camera's/view's field of view.
  /// </summary>
  /// <param name="fov">The field of view in radians.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetViewFOV(double &fov) const = 0;

  /// <summary>
  /// Gets the camera/view frustum.
  /// </summary>
  /// <param name="frustum">The camera/view <see cref="navlib::frustum_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetViewFrustum(navlib::frustum_t &frustum) const = 0;

  /// <summary>
  /// Get's the view's projection type
  /// </summary>
  /// <param name="perspective">true for a perspective view, false for an orthographic view.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetIsViewPerspective(navlib::bool_t &perspective) const = 0;

  /// <summary>
  /// Gets a value indicating whether the view can be rotated.
  /// </summary>
  /// <param name="isRotatable">true if the view can be rotated, false otherwise.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetIsViewRotatable(navlib::bool_t &isRotatable) const = 0;

  /// <summary>
  /// Sets the camera affine matrix.
  /// </summary>
  /// <param name="matrix">The camera/view <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetCameraMatrix(const navlib::matrix_t& matrix) = 0;

  /// <summary>
  /// Sets the camera's target position.
  /// </summary>
  /// <param name="target">The position of the camera target in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>Free cameras do not have a target.</remarks>
  virtual long SetCameraTarget(const navlib::point_t &target) = 0;

  /// <summary>
  ///  Sets the position of the pointer on the near clipping plane.
  /// </summary>
  /// <param name="position">The <see cref="navlib::point_t"/> in world coordinates of the
  /// pointer.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetPointerPosition(const navlib::point_t& position) = 0;

  /// <summary>
  /// Sets the extents of the view.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// view.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetViewExtents(const navlib::box_t& extents) = 0;

  /// <summary>
  /// Sets the camera's/view's field of view.
  /// </summary>
  /// <param name="fov">The field of view in radians.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetViewFOV(double fov) = 0;

  /// <summary>
  /// Sets the camera/view frustum.
  /// </summary>
  /// <param name="frustum">The camera/view <see cref="navlib::frustum_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetViewFrustum(const navlib::frustum_t& frustum) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IView_HPP_INCLUDED
