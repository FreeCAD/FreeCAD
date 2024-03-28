#ifndef CNavigation3D_HPP_INCLUDED
#define CNavigation3D_HPP_INCLUDED
// <copyright file="CNavigation3D.hpp" company="3Dconnexion">
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
#include <SpaceMouse/CCategory.hpp>
#include <SpaceMouse/CCommand.hpp>
#include <SpaceMouse/CCommandSet.hpp>
#include <SpaceMouse/CHitTest.hpp>
#include <SpaceMouse/CImage.hpp>
#include <SpaceMouse/CNavlibImpl.hpp>
#include <SpaceMouse/IAccessors.hpp>
#include <SpaceMouse/INavlib.hpp>

// stdlib
#include <memory>
#include <string>
#include <vector>
#if (!defined(_MSC_VER) || (_MSC_VER > 1600))
#include <chrono>
#else
#include <boost/chrono.hpp>
namespace std {
namespace chrono = boost::chrono;
using boost::milli;
} // namespace std
#endif

// navlib
#include <navlib/navlib.h>
#include <navlib/navlib_error.h>

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// Contains types that support 3DMouse navigation.
/// </summary>
namespace Navigation3D {
/// <summary>
/// The base class for 3D navigation implements defaults for the <see cref="IAccessors"/> interface.
/// </summary>
/// <remarks>This class can be used as the base class for the application specific implementation of
/// the accessors.</remarks>
class CNavigation3D : public INavlibProperty, protected IAccessors {
public:
  /// <summary>
  /// The timing source for the frame time.
  /// </summary>
  enum TimingSource {
    /// <summary>
    /// The space mouse is the source of the frame timing.
    /// </summary>
    SpaceMouse = 0,

    /// <summary>
    /// The application is the source of the frame timing.
    /// </summary>
    Application = 1,
  };

public:
  /// <summary>
  /// Initializes a new instance of the CNavigation3D class.
  /// </summary>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="columnVectors">true for column vectors, false for row vectors as used by OpenGL.
  /// </param>
  /// <remarks>The default is single-threaded, row vectors</remarks>
  explicit CNavigation3D(bool multiThreaded = false, bool columnVectors = false)
      : m_enabled(false), m_pImpl(CNavlibImpl::CreateInstance(this, multiThreaded, columnVectors)) {
  }

  /// <summary>
  /// Initializes a new instance of the CNavigation3D class.
  /// </summary>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="options">A combination of the <see cref="navlib::nlOptions_t"/> values.</param>
  explicit CNavigation3D(bool multiThreaded, navlib::nlOptions_t options)
      : m_enabled(false), m_pImpl(CNavlibImpl::CreateInstance(this, multiThreaded, options)) {
  }

#if defined(_MSC_EXTENSIONS)
  /// <summary>
  /// Gets or sets a value indicating whether the 3DMouse navigation is enabled.
  /// </summary>
  __declspec(property(get = IsEnabled, put = EnableNavigation)) bool Enable;

  /// <summary>
  /// Gets or sets the animation frame time.
  /// </summary>
  __declspec(property(get = GetFrameTime,
                      put = PutFrameTime)) std::chrono::high_resolution_clock::time_point FrameTime;

  /// <summary>
  /// Gets or sets the frame timing source.
  /// </summary>
  __declspec(property(get = GetFrameTimingSource,
                      put = PutFrameTimingSource)) TimingSource FrameTiming;
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
  /// Gets a value indicating whether 3DMouse navigation is enabled.
  /// </summary>
  /// <returns>true if enabled, otherwise false.</returns>
  bool IsEnabled() const {
    return m_enabled;
  }

  /// <summary>
  /// Sets a value indicating whether 3DMouse navigation is enabled.
  /// </summary>
  /// <param name="value">true to enable, false to disable.</param>
  /// <exception cref="std::invalid_argument">The text for the '3Dconnexion Properties' is
  /// empty.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  void EnableNavigation(bool value) {
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
  /// Sets a value indicating whether 3DMouse navigation is enabled.
  /// </summary>
  /// <param name="value">true to enable, false to disable.</param>
  /// <param name="ec">The <see cref="std::error_code"/> contains the error if something goes
  /// wrong.</param>
  /// <exception cref="std::invalid_argument">The text for the '3Dconnexion Properties' is
  /// empty.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  void EnableNavigation(bool value, std::error_code &ec) NOEXCEPT {
    try {
      EnableNavigation(value);
    }
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
    catch (const std::system_error &e) {
      ec = e.code();
      std::cout << "system_error exception thrown in EnableNavigation(" << value << ") 0x" << std::hex
                << ec.value() << std::dec << ", " << ec.message() << ", " << e.what() << "\n";
    } catch (const std::invalid_argument &e) {
      ec = std::make_error_code(std::errc::invalid_argument);
      std::cout << "invalid_argument exception thrown in EnableNavigation(" << value << ") 0x"
                << std::hex << ec.value() << std::dec << ", " << ec.message() << ", " << e.what()
                << "\n";
    } catch (const std::exception &e) {
      ec = std::make_error_code(std::errc::io_error);
      std::cout << "exception thrown in EnableNavigation(" << value << ") 0x" << std::hex << ec.value()
                << std::dec << ", " << ec.message() << ", " << e.what() << "\n";
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
  /// Gets or sets the animation frame time.
  /// </summary>
  std::chrono::high_resolution_clock::time_point GetFrameTime() {
    return m_frameTime;
  }
  void PutFrameTime(std::chrono::high_resolution_clock::time_point value) {
    if (m_frameTime != value) {
      m_frameTime = std::move(value);
      auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          m_frameTime.time_since_epoch());
      m_pImpl->Write(navlib::frame_time_k, elapsed.count());
    }
  }

  /// <summary>
  /// Gets or sets the frame timing source.
  /// </summary>
  TimingSource GetFrameTimingSource() {
    return m_frameTimingSource;
  }
  void PutFrameTimingSource(TimingSource value) {
    if (m_frameTimingSource != value) {
      m_frameTimingSource = value;
      m_pImpl->Write(navlib::frame_timing_source_k, static_cast<long>(value));
    }
  }

  /// <summary>
  /// Gets or sets the text to pass to the 3Dconnexion driver to use in Properties Utility.
  /// </summary>
  /// <remarks>If the connection to the navlib is already open, the connection is closed and
  /// reopened.</remarks>
  std::string GetProfileHint() const {
    return m_profileHint;
  }
  void PutProfileHint(std::string value) {
    if (m_profileHint != value) {
      m_profileHint = std::move(value);
      if (IsEnabled()) {
        EnableNavigation(false);
        EnableNavigation(true);
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
    for (auto iter = images.begin(); iter != images.end(); ++iter) {
      siImages.push_back(static_cast<SiImage_t>(*iter));
    }
    navlib::imagearray_t imagearray = {siImages.data(), siImages.size()};
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

  /// ISpace3D overrides
  /// <summary>
  /// Gets the coordinate system used by the client.
  /// </summary>
  /// <param name="matrix">The coordinate system <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>The matrix describes the applications coordinate frame in the navlib coordinate
  /// system. i.e. the application to navlib transform. The default is a  right-handed coordinate
  /// system X-right, Z-up, Y-in (column-major)</remarks>
  long GetCoordinateSystem(navlib::matrix_t &matrix) const override {
    // Use the right-handed coordinate system X-right, Y-up, Z-out (same as navlib)
    navlib::matrix_t cs = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // Use the right-handed coordinate system X-right, Z-up, Y-in (column-major/row vectors)
    // navlib::matrix_t cs = {{{1, 0, 0, 0,  0, 0, -1, 0,  0, 1, 0, 0,  0, 0, 0, 1}}};

    matrix = cs;

    return 0;
  }

  /// <summary>
  /// Gets the orientation of the front view.
  /// </summary>
  /// <param name="matrix">The front view transform <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>The default is the inverse of the coordinate system, i.e. in this case the identity
  /// matrix.</remarks>
  long GetFrontView(navlib::matrix_t &matrix) const override {
    navlib::matrix_t front = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    matrix = front;
    return 0;
  }

  // IModel overrides
  /// <summary>
  /// Gets the length of the model/world unit in meters.
  /// </summary>
  /// <param name="meters">The length of a model/world unit in meters.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>
  /// <para>The conversion factor is used by the Navigation Library to calculate the height above
  /// the floor in walk mode and the speed in the first-person motion model.</para>
  /// <para>The Navigation Library assumes that this value does not change and it is only queried
  /// once.</para>
  /// </remarks>
  long GetUnitsToMeters(double &meters) const override {
    (void)meters;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  /// Gets the plane equation of the floor.
  /// </summary>
  /// <param name="floor">The plane equation of the floor plane.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>
  /// <para>The plane equation is used by the Navigation Library to determine the floor for the
  /// walk navigation mode, where the height of the eye is fixed to 1.5m above the floor plane.
  /// The floor need not be parallel to the world ground plane.</para>
  /// </remarks>
  long GetFloorPlane(navlib::plane_t &floor) const override {
    (void)floor;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  // IState overrides
  /// <summary>
  /// Is called when the navigation library starts or stops a navigation transaction.
  /// </summary>
  /// <param name="transaction">The transaction number: >0 begin, ==0 end.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  long SetTransaction(long transaction) override {
    (void)transaction;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  /// Is called when the navigation instance starts or stops a sequence of motion frames.
  /// </summary>
  /// <param name="motion">The motion flag: true = start, false = end.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>This marks the start and end of user interaction with the scene using the
  /// 3D Mouse.</remarks>
  long SetMotionFlag(bool motion) override {
    (void)motion;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  /// Gets the camera's target.
  /// </summary>
  /// <param name="target">The position of the camera target in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>Free cameras do not have a target.</remarks>
  long GetCameraTarget(navlib::point_t &target) const override {
    (void)target;
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }
  /// <summary>
  /// Gets the view's construction plane.
  /// </summary>
  /// <param name="plane">The plane equation of the construction plane.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>Required to disable rotations when constructing.</remarks>
  long GetViewConstructionPlane(navlib::plane_t &plane) const override {
    (void)plane;
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  /// <summary>
  /// Gets a value indicating whether the view can be rotated.
  /// </summary>
  /// <param name="isRotatable">true if the view can be rotated, false otherwise.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>For paper space return false.</remarks>
  long GetIsViewRotatable(navlib::bool_t &isRotatable) const override {
    isRotatable = true;
    return 0;
  }

  /// <summary>
  /// Gets the distance to the view's focused object.
  /// </summary>
  /// <param name="distance">The distance to the object in world units.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>The distance to the object determines the translation velocity in camera mode
  /// navigation modes. Generally the navigation library attempts to determine this value by
  /// hit-testing the scene. There are, however,  cases when the user wants to move to a far
  /// off object and is not interested in what is nearby.</remarks>
  long GetViewFocusDistance(double &distance) const override {
    (void)distance;
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  /// <summary>
  /// Sets the camera's target position.
  /// </summary>
  /// <param name="target">The position of the camera target in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>Free cameras do not have a target.</remarks>
  long SetCameraTarget(const navlib::point_t &target) override {
    (void)target;
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
  }

  /// <summary>
  ///  Sets the position of the pointer on the projection plane.
  /// </summary>
  /// <param name="position">The <see cref="navlib::point_t"/> in world coordinates of the
  /// pointer.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  long SetPointerPosition(const navlib::point_t &position) override {
    (void)position;
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
  std::chrono::high_resolution_clock::time_point m_frameTime;
  TimingSource m_frameTimingSource;
  std::shared_ptr<CNavlibImpl> m_pImpl;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // CNavigationModelImpl_HPP_INCLUDED
