#ifndef CActionInputImpl_HPP_INCLUDED
#define CActionInputImpl_HPP_INCLUDED
// <copyright file="CActionInputImpl.hpp" company="3Dconnexion">
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
// $Id$
//
// </history>
#include <SpaceMouse/CNavlibInterface.hpp>
#include <SpaceMouse/IActionAccessors.hpp>
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
namespace ActionInput {
/// <summary>
/// Implementation for creating a shared pointer.
/// </summary>
class CActionInputImpl : public Navigation3D::INavlib,
#if defined(_MSC_VER) && _MSC_VER < 1800
                         public IActionAccessors,
#else
                         private IActionAccessors,
#endif
                         public std::enable_shared_from_this<CActionInputImpl> {
  typedef CActionInputImpl this_type;

  friend std::shared_ptr<IActionAccessors>
  std::static_pointer_cast<IActionAccessors, CActionInputImpl>(
      const std::shared_ptr<CActionInputImpl> &) NOEXCEPT;

  /// <summary>
  /// Make the constructors private to force creation of a shared pointer.
  /// </summary>
  CActionInputImpl() = default;
  CActionInputImpl(IActionAccessors *iclient) : m_iclient(iclient) {
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
  static std::shared_ptr<CActionInputImpl> CreateInstance(IActionAccessors *iclient,
                                                          bool multiThreaded = false) {
    return CreateInstance(iclient, multiThreaded, navlib::none);
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
  static std::shared_ptr<CActionInputImpl>
  CreateInstance(IActionAccessors *iclient, bool multiThreaded, navlib::nlOptions_t options) {
    if (iclient == nullptr) {
      throw std::logic_error("The accessor interface is null");
    }

    // So that std::make_shared<> can be used with the private constructor.
    struct make_shared_enabler : public this_type {
      make_shared_enabler(IActionAccessors *iclient) : this_type(iclient) {
      }
    };

    std::shared_ptr<CActionInputImpl> result = std::make_shared<make_shared_enabler>(iclient);
    result->m_pNavlib =
        std::unique_ptr<Navigation3D::CNavlibInterface>(new Navigation3D::CNavlibInterface(
            std::static_pointer_cast<IActionAccessors>(result), multiThreaded, options));

    return result;
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

private:
  IActionAccessors *m_iclient = nullptr;
  std::unique_ptr<Navigation3D::CNavlibInterface> m_pNavlib;
};
} // namespace ActionInput
} // namespace SpaceMouse
} // namespace TDx
#endif // CNavlibImpl_HPP_INCLUDED
