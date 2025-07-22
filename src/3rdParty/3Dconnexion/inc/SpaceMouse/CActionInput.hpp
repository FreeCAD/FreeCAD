#ifndef CInputAction_HPP_INCLUDED
#define CInputAction_HPP_INCLUDED
// <copyright file="CInputAction.hpp" company="3Dconnexion">
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
// $Id: CNavigation3D.hpp 16056 2019-04-10 13:42:31Z mbonk $
//
// </history>

// SpaceMouse
#include <SpaceMouse/CActionInputImpl.hpp>
#include <SpaceMouse/CCategory.hpp>
#include <SpaceMouse/CCommand.hpp>
#include <SpaceMouse/CCommandSet.hpp>
#include <SpaceMouse/CImage.hpp>
#include <SpaceMouse/INavlib.hpp>

// stdlib
#include <memory>
#include <string>
#include <vector>

// navlib
#include <navlib/navlib.h>
#include <navlib/navlib_error.h>

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// Contains types that support 3Dconnexion SpaceMouse input action interface.
/// </summary>
namespace ActionInput {
/// <summary>
/// The base class for 3D navigation implements defaults for the <see cref="IAccessors"/> interface.
/// </summary>
/// <remarks>This class can be used as the base class for the application specific implementation of
/// the accessors.</remarks>
class CActionInput : public Navigation3D::INavlibProperty, protected IActionAccessors {
public:
  /// <summary>
  /// Initializes a new instance of the <see cref="CActionInput"/> class.
  /// </summary>
  /// <param name="multiThreaded">Allow multithreading.</param>
  /// <remarks>Set multithreaded to true for a windows console application. If the application
  /// requires that the callbacks are executed on the main thread, either leave as false, or marshall
  /// the callbacks back to the main thread.</remarks>
  CActionInput(bool multiThreaded = false)
      : m_enabled(false), m_pImpl(CActionInputImpl::CreateInstance(this, multiThreaded)) {
  }

#if defined(_MSC_EXTENSIONS)
  /// <summary>
  /// Gets or sets a value indicating whether the connection to the input device is enabled.
  /// </summary>
  __declspec(property(get = IsEnabled, put = PutEnable)) bool Enable;

  /// <summary>
  /// Gets or sets the text to pass to the 3Dconnexion driver to use in Properties Utility.
  /// </summary>
  /// <remarks>If the connection to the navlib is already open, the connection is closed and
  /// reopened.</remarks>
  __declspec(property(get = GetProfileHint, put = PutProfileHint)) std::string Profile;

  /// <summary>
  /// Gets or sets a value representing the active command set.
  /// </summary>
  /// <exception cref="std::system_error"></exception>
  __declspec(property(get = GetActiveCommands, put = PutActiveCommands)) std::string ActiveCommands;

#endif

  /// <summary>
  /// Gets a value indicating whether action interface is enabled.
  /// </summary>
  /// <returns>true if enabled, otherwise false.</returns>
  bool IsEnabled() const {
    return m_enabled;
  }

  /// <summary>
  /// Sets a value indicating whether the action interface is enabled.
  /// </summary>
  /// <param name="value">true to enable, false to disable.</param>
  /// <exception cref="std::invalid_argument">The text for the '3Dconnexion Properties' is
  /// empty.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  void PutEnable(bool value) {
    if (m_enabled == value) {
      return;
    }
    if (value) {
      m_pImpl->Open(m_profileHint);
      m_enabled = true;
    } else {
      m_pImpl->Close();
      m_enabled = false;
    }
  }

  /// <summary>
  /// Sets a value indicating whether the action interface is enabled.
  /// </summary>
  /// <param name="value">true to enable, false to disable.</param>
  /// <param name="ec">The <see cref="std::error_code"/> contains the error if something goes
  /// wrong.</param>
  /// <exception cref="std::invalid_argument">The text for the '3Dconnexion Properties' is
  /// empty.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  void PutEnable(bool value, std::error_code &ec) NOEXCEPT {
    try {
      PutEnable(value);
    }
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
    catch (const std::system_error &e) {
      ec = e.code();
      std::cerr << "system_error exception thrown in EnableNavigation(" << value << ") 0x"
                << std::hex << ec.value() << std::dec << ", " << ec.message() << ", " << e.what()
                << "\n";
    } catch (const std::invalid_argument &e) {
      ec = std::make_error_code(std::errc::invalid_argument);
      std::cerr << "invalid_argument exception thrown in EnableNavigation(" << value << ") 0x"
                << std::hex << ec.value() << std::dec << ", " << ec.message() << ", " << e.what()
                << "\n";
    } catch (const std::exception &e) {
      ec = std::make_error_code(std::errc::io_error);
      std::cerr << "exception thrown in EnableNavigation(" << value << ") 0x" << std::hex
                << ec.value() << std::dec << ", " << ec.message() << ", " << e.what() << "\n";
    }
#else
    catch (const std::system_error &e) {
      ec = e.code();
    } catch (const std::invalid_argument &) {
      ec = std::make_error_code(std::errc::invalid_argument);
    } catch (const std::exception &) {
      ec = std::make_error_code(std::errc::io_error);
    }
#endif
  }

  /// <summary>
  /// Gets or sets the text to pass to the 3Dconnexion driver to use in Properties Utility.
  /// </summary>
  /// <remarks>If the connection to the navlib is already open, the connection is closed and
  /// reopened.</remarks>
  std::string GetProfileHint() const {
    return m_profileHint;
  }
  void PutProfileHint(std::string const &value) {
    if (m_profileHint != value) {
      m_profileHint = value;
      if (IsEnabled()) {
        PutEnable(false);
        PutEnable(true);
      }
    }
  }

  /// <summary>
  /// Gets or sets a value representing the active command set.
  /// </summary>
  /// <exception cref="std::system_error"></exception>
  std::string GetActiveCommands() const {
    std::string result;
    long error = m_pImpl->Read(navlib::commands_activeSet_k, result);
    if (error != 0) {
      throw std::system_error(make_error_code(error));
    }
    return result;
  }
  void PutActiveCommands(const std::string &id) {
    long error = m_pImpl->Write(navlib::commands_activeSet_k, id);
    if (error != 0) {
      throw std::system_error(make_error_code(error));
    }
  }

  /// <summary>
  /// Add commands to the sets of commands.
  /// </summary>
  /// <param name="commands">The <see cref="CCommandTree"/> to add.</param>
  /// <exception cref="std::system_error"></exception>
  void AddCommands(const CCommandTree &commands) {
    const SiActionNodeEx_t *pnode = &commands.GetSiActionNode();
    long error = m_pImpl->Write(navlib::commands_tree_k, pnode);
    if (error != 0) {
      throw std::system_error(make_error_code(error));
    }
  }

  /// <summary>
  /// Add a set of commands to the sets of commands.
  /// </summary>
  /// <param name="commands">The <see cref="CCommandSet"/> to add.</param>
  /// <exception cref="std::system_error"></exception>
  void AddCommandSet(const CCommandSet &commands) {
    AddCommands(commands);
  }

  /// <summary>
  /// Add to the images available to the 3Dconnexion properties utility.
  /// </summary>
  /// <param name="images">The <see cref="std::vector"/> container containing the images to
  /// add.</param>
  /// <exception cref="std::system_error"></exception>
  template <class T>
  void AddImages(const std::vector<T> &images,
                 typename std::enable_if<std::is_base_of<SiImage_t, T>::value &&
                                         sizeof(T) == sizeof(SiImage_t)>::type * = nullptr) {
    long error;
    navlib::imagearray_t imagearray = {images.data(), images.size()};
    error = m_pImpl->Write(navlib::images_k, imagearray);
    if (error != 0) {
      throw std::system_error(make_error_code(error));
    }
  }

  /// <summary>
  /// Add to the images available to the 3Dconnexion properties utility.
  /// </summary>
  /// <param name="images">The <see cref="std::vector{T}"/> container containing the images to
  /// add.</param>
  /// <exception cref="std::system_error"></exception>
  template <class T>
  void
  AddImages(const std::vector<T> &images,
            typename std::enable_if<std::is_base_of<TDx::CImage, T>::value>::type * = nullptr) {
    std::vector<SiImage_t> siImages;
    for (auto const &image : images) {
      siImages.push_back(static_cast<SiImage_t>(image));
    }

    const navlib::imagearray_t imagearray = {siImages.data(), siImages.size()};
    long error = m_pImpl->Write(navlib::images_k, imagearray);
    if (error != 0) {
      throw std::system_error(make_error_code(error));
    }
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
    return m_pImpl->Write(propertyName, value);
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
    return m_pImpl->Read(propertyName, value);
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
    return m_pImpl->Read(propertyName, string);
  }

protected:
  // IEvents overrides
  /// <summary>
  /// Default for SetSettingsChanged.
  /// </summary>
  /// <param name="count">The change count.</param>
  /// <returns>navlib::navlib_errc::function_not_supported error.</returns>
  long SetSettingsChanged(long count) override {
    (void)count;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  ///  Default for SetKeyPress.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key pressed.</param>
  /// <returns>navlib::navlib_errc::function_not_supported error.</returns>
  long SetKeyPress(long vkey) override {
    (void)vkey;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  ///  Default for SetKeyRelease.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key pressed.</param>
  /// <returns>navlib::navlib_errc::function_not_supported error.</returns>
  long SetKeyRelease(long vkey) override {
    (void)vkey;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }


protected:
  std::error_code make_error_code(long result_code) const {
    int errc = result_code & 0xffff;
    int facility = result_code >> 16 & 0x7fff;
    if (facility == FACILITY_NAVLIB) {
      return std::error_code(errc, navlib_category);
    }
    return std::error_code(errc, std::generic_category());
  }

protected:
  bool m_enabled;
  std::string m_profileHint;
  std::shared_ptr<CActionInputImpl> m_pImpl;
};
} // namespace ActionInput
} // namespace SpaceMouse
} // namespace TDx
#endif // CInputAction_HPP_INCLUDED
