#ifndef CNavlibImpl_HPP_INCLUDED
#define CNavlibImpl_HPP_INCLUDED
// <copyright file="CNavlibImpl.hpp" company="3Dconnexion">
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
// $Id: CNavlibImpl.hpp 16062 2019-04-11 12:58:50Z mbonk $
//
// 05/25/20 MSB Fix C2280. Use std::static_pointer_cast<> instead of std::dynamic_pointer_cast<>.
// </history>
#include <SpaceMouse/CNavlibInterface.hpp>
#include <SpaceMouse/IAccessors.hpp>
#include <SpaceMouse/INavlib.hpp>

// stdlib
#include <map>
#include <memory>
#include <string>
#include <vector>

// navlib
#include <navlib/navlib.h>
#include <navlib/navlib_error.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// Implementation for creating a shared pointer.
/// </summary>
class CNavlibImpl : public INavlib,
#if defined(_MSC_VER) && _MSC_VER <= 1800
                    public IAccessors,
#else
                    private IAccessors,
#endif
                    public std::enable_shared_from_this<CNavlibImpl> {
  typedef CNavlibImpl this_type;

  friend std::shared_ptr<IAccessors>
  std::static_pointer_cast<IAccessors, CNavlibImpl>(const std::shared_ptr<CNavlibImpl> &) NOEXCEPT;

  /// <summary>
  /// Make the constructors private to force creation of a shared pointer.
  /// </summary>
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  CNavlibImpl() = default;
#else
  CNavlibImpl() {
  }
#endif

  CNavlibImpl(IAccessors *iclient) : m_iclient(iclient) {
  }

public:
  /// <summary>
  /// Creates a new instance of the CNavlibImpl class.
  /// </summary>
  /// <param name="iclient">pointer to the instance implementing the IAccessors interface.</param>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="rowMajor">true for row-major ordered matrices, false for column-major.</param>
  /// <returns>
  /// A <see cref="std::shared_ptr{CNavlibImpl}"/> to the new CNavlibImpl instance.
  /// </returns>
  static std::shared_ptr<CNavlibImpl>
  CreateInstance(IAccessors *iclient, bool multiThreaded = false, bool rowMajor = false) {
    return CreateInstance(iclient, multiThreaded,
                          rowMajor ? navlib::row_major_order : navlib::none);
  }

  /// <summary>
  /// Creates a new instance of the CNavlibImpl class.
  /// </summary>
  /// <param name="iclient">pointer to the instance implementing the IAccessors interface.</param>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="options">A combination of the <see cref="navlib::nlOptions_t"/> values.</param>
  /// <returns>
  /// A <see cref="std::shared_ptr{CNavlibImpl}"/> to the new CNavlibImpl instance.
  /// </returns>
  static std::shared_ptr<CNavlibImpl> CreateInstance(IAccessors *iclient, bool multiThreaded,
                                                     navlib::nlOptions_t options) {
    if (iclient == nullptr) {
      throw std::logic_error("The accessor interface is null");
    }

    // So that std::make_shared<> can be used with the private constructor.
    struct make_shared_enabler : public this_type {
      make_shared_enabler(IAccessors *iclient) : this_type(iclient) {
      }
    };

    std::shared_ptr<CNavlibImpl> result = std::make_shared<make_shared_enabler>(iclient);
    result->m_pNavlib = std::unique_ptr<CNavlibInterface>(
        new CNavlibInterface(std::static_pointer_cast<IAccessors>(result), multiThreaded, options));

    return result;
  }

  /// <summary>
  /// Clean up the resources
  /// </summary>
  virtual ~CNavlibImpl() {
  }

  // INavlib implementation
  /// <summary>
  /// Close the connection to the 3D navigation library.
  /// </summary>
  void Close() override {
    m_pNavlib->Close();
  }

  /// <summary>
  /// Opens a connection to the 3D navigation library.
  /// </summary>
  void Open() override {
    m_pNavlib->Open();
  }

  /// <summary>
  /// Opens a connection to the 3D navigation library
  /// </summary>
  /// <param name="profileName">The name of the 3Dconnexion profile to use.</param>
  /// <exception cref="std::system_error">The connection to the library is already open.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  /// <exception cref="std::invalid_argument">The name of the profile is empty.</exception>
  void Open(std::string profileName) override {
    m_pNavlib->Open(std::move(profileName));
  }

  /// <summary>
  /// Writes the value of a property to the navlib.
  /// </summary>
  /// <param name="propertyName">The <see cref="navlib::property_t"/> name of the navlib property to
  /// write.</param>
  /// <param name="value">The <see cref="navlib::value"/> to write.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib / 3D Mouse.</exception>
  long Write(const std::string &propertyName, const navlib::value &value) override {
    return m_pNavlib->Write(propertyName, value);
  }

  /// <summary>
  /// Reads the value of a navlib property.
  /// </summary>
  /// <param name="propertyName">The <see cref="navlib::property_t"/> name of the navlib property to
  /// read.</param>
  /// <param name="value">The <see cref="navlib::value"/> to read.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib / 3D Mouse.</exception>
  long Read(const std::string &propertyName, navlib::value &value) const override {
    return m_pNavlib->Read(propertyName, value);
  }

  /// <summary>
  /// Reads the value of a navlib string property.
  /// </summary>
  /// <param name="propertyName">The <see cref="navlib::property_t"/> name of the navlib property to
  /// read.</param>
  /// <param name="string">The <see cref="std::string"/> value of the property.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib.</exception>
  long Read(const std::string &propertyName, std::string &string) const override {
    return m_pNavlib->Read(propertyName, string);
  }

private:
  // IEvents overrides
  long SetActiveCommand(std::string commandId) override {
    return m_iclient->SetActiveCommand(commandId);
  }
  long SetSettingsChanged(long change) override {
    return m_iclient->SetSettingsChanged(change);
  }
  long SetKeyPress(long vkey) override {
    return m_iclient->SetKeyPress(vkey);
  }
  long SetKeyRelease(long vkey) override {
    return m_iclient->SetKeyRelease(vkey);
  }

  // IHit overrides
  long GetHitLookAt(navlib::point_t &position) const override {
    return m_iclient->GetHitLookAt(position);
  }
  long SetHitAperture(double aperture) override {
    return m_iclient->SetHitAperture(aperture);
  }
  long SetHitDirection(const navlib::vector_t &direction) override {
    return m_iclient->SetHitDirection(direction);
  }
  long SetHitLookFrom(const navlib::point_t &eye) override {
    return m_iclient->SetHitLookFrom(eye);
  }
  long SetHitSelectionOnly(bool onlySelection) override {
    return m_iclient->SetHitSelectionOnly(onlySelection);
  }

  // IModel overrides
  long GetModelExtents(navlib::box_t &extents) const override {
    return m_iclient->GetModelExtents(extents);
  }
  long GetSelectionExtents(navlib::box_t &extents) const override {
    return m_iclient->GetSelectionExtents(extents);
  }
  long GetSelectionTransform(navlib::matrix_t &transform) const override {
    return m_iclient->GetSelectionTransform(transform);
  }
  long GetIsSelectionEmpty(navlib::bool_t &empty) const override {
    return m_iclient->GetIsSelectionEmpty(empty);
  }
  long SetSelectionTransform(const navlib::matrix_t &matrix) override {
    return m_iclient->SetSelectionTransform(matrix);
  }
  long GetUnitsToMeters(double &meters) const override {
    return m_iclient->GetUnitsToMeters(meters);
  }
  long GetFloorPlane(navlib::plane_t &floor) const override {
    return m_iclient->GetFloorPlane(floor);
  }

  // IPivot overrides
  long GetPivotPosition(navlib::point_t &position) const override {
    return m_iclient->GetPivotPosition(position);
  }
  long IsUserPivot(navlib::bool_t &userPivot) const override {
    return m_iclient->IsUserPivot(userPivot);
  }
  long SetPivotPosition(const navlib::point_t &position) override {
    return m_iclient->SetPivotPosition(position);
  }
  long GetPivotVisible(navlib::bool_t &visible) const override {
    return m_iclient->GetPivotVisible(visible);
  }
  long SetPivotVisible(bool visible) override {
    return m_iclient->SetPivotVisible(visible);
  }
  // ISpace3D overrides
  long GetCoordinateSystem(navlib::matrix_t &matrix) const override {
    return m_iclient->GetCoordinateSystem(matrix);
  }
  long GetFrontView(navlib::matrix_t &matrix) const override {
    return m_iclient->GetFrontView(matrix);
  }
  // IState overrides
  long SetTransaction(long transaction) override {
    return m_iclient->SetTransaction(transaction);
  }
  long SetMotionFlag(bool motion) override {
    return m_iclient->SetMotionFlag(motion);
  }
  // IView overrides
  long GetCameraMatrix(navlib::matrix_t &matrix) const override {
    return m_iclient->GetCameraMatrix(matrix);
  }
  long GetCameraTarget(navlib::point_t &point) const override {
    return m_iclient->GetCameraTarget(point);
  }
  long GetPointerPosition(navlib::point_t &position) const override {
    return m_iclient->GetPointerPosition(position);
  }
  long GetViewConstructionPlane(navlib::plane_t &plane) const override {
    return m_iclient->GetViewConstructionPlane(plane);
  }
  long GetViewExtents(navlib::box_t &extents) const override {
    return m_iclient->GetViewExtents(extents);
  }
  long GetViewFocusDistance(double &distance) const override {
    return m_iclient->GetViewFocusDistance(distance);
  }
  long GetViewFOV(double &fov) const override {
    return m_iclient->GetViewFOV(fov);
  }
  long GetViewFrustum(navlib::frustum_t &frustum) const override {
    return m_iclient->GetViewFrustum(frustum);
  }
  long GetIsViewPerspective(navlib::bool_t &perspective) const override {
    return m_iclient->GetIsViewPerspective(perspective);
  }
  long GetIsViewRotatable(navlib::bool_t &isRotatable) const override {
    return m_iclient->GetIsViewRotatable(isRotatable);
  }
  long SetCameraMatrix(const navlib::matrix_t &matrix) override {
    return m_iclient->SetCameraMatrix(matrix);
  }
  long SetCameraTarget(const navlib::point_t &target) override {
    return m_iclient->SetCameraTarget(target);
  }
  long SetPointerPosition(const navlib::point_t &position) override {
    return m_iclient->SetPointerPosition(position);
  }
  long SetViewExtents(const navlib::box_t &extents) override {
    return m_iclient->SetViewExtents(extents);
  }
  long SetViewFOV(double fov) override {
    return m_iclient->SetViewFOV(fov);
  }
  long SetViewFrustum(const navlib::frustum_t &frustum) override {
    return m_iclient->SetViewFrustum(frustum);
  }

private:
  IAccessors *m_iclient = nullptr;
  std::unique_ptr<CNavlibInterface> m_pNavlib;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // CNavlibImpl_HPP_INCLUDED
