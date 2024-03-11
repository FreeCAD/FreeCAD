#ifndef CNavlibInterface_HPP_INCLUDED
#define CNavlibInterface_HPP_INCLUDED
// <copyright file="CNavilibInterface.hpp" company="3Dconnexion">
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
// $Id: CNavlibInterface.hpp 16051 2019-04-09 11:29:53Z mbonk $
//
// 07/23/19 MSB Do not set the cookie to zero when the open fails.
// </history>
#include <SpaceMouse/CCookieCollection.hpp>
#include <SpaceMouse/IAccessors.hpp>
#include <SpaceMouse/IActionAccessors.hpp>
#include <SpaceMouse/INavlib.hpp>

// stdlib
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

// navlib
#include <navlib/navlib.h>
#include <navlib/navlib_error.h>
#include <navlib/navlib_ostream.h>

#if defined(WAMP_CLIENT) && (WAMP_CLIENT==1)
// wamp
#include <wamp/net.hpp>
#include <wamp/navlib_client_session.hpp>
#endif


namespace TDx {
namespace SpaceMouse {
/// <summary>
/// Template to get a specific interface from a unique pointer.
/// </summary>
template <typename Ty_, typename I_> Ty_ *GetInterface(const std::unique_ptr < I_>& p) {
  I_ *i = p.get();
  if (i == nullptr) {
    return nullptr;
  }

  Ty_ *t = static_cast<Ty_ *>(*p.get());
  return t;
}

/// <summary>
/// The base class for the Accessors items.
/// </summary>
/// <remarks> The purpose of the class is to use polymorphism to avoid dynamic casts.
/// <remarks/>
class AccessorInterface {
public:
  explicit operator Navigation3D::ISpace3D *() {
    return GetISpace3DInterface();
  }
  explicit operator Navigation3D::IView *() {
    return GetIViewInterface();
  }
  explicit operator Navigation3D::IModel *() {
    return GetIModelInterface();
  }
  explicit operator Navigation3D::IPivot *() {
    return GetIPivotInterface();
  }
  explicit operator Navigation3D::IHit *() {
    return GetIHitInterface();
  }
  explicit operator Navigation3D::IEvents *() {
    return GetIEventsInterface();
  }
  explicit operator Navigation3D::IState *() {
    return GetIStateInterface();
  }

#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  AccessorInterface() = default;
  virtual ~AccessorInterface() = default;
  AccessorInterface(const AccessorInterface &) = delete;
  AccessorInterface &operator=(const AccessorInterface &) = delete;
#else
  AccessorInterface(){};
  virtual ~AccessorInterface(){};
private:
  AccessorInterface(const AccessorInterface &);
  AccessorInterface &operator=(const AccessorInterface &);
#endif

protected:
  virtual Navigation3D::ISpace3D *GetISpace3DInterface() {
    return nullptr;
  }
  virtual Navigation3D::IView *GetIViewInterface() {
    return nullptr;
  }
  virtual Navigation3D::IModel *GetIModelInterface() {
    return nullptr;
  }
  virtual Navigation3D::IPivot *GetIPivotInterface() {
    return nullptr;
  }
  virtual Navigation3D::IHit *GetIHitInterface() {
    return nullptr;
  }
  virtual Navigation3D::IEvents *GetIEventsInterface() {
    return nullptr;
  }
  virtual Navigation3D::IState *GetIStateInterface() {
    return nullptr;
  }
};


class IWeakPtr {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~IWeakPtr() = default;
#else
  virtual ~IWeakPtr() = 0 {
  }
#endif
  virtual std::unique_ptr<AccessorInterface> lock() = 0;
};


// General template for a <see cref="WeakAccessorPtr{T}"/>.
template <typename T_> class WeakAccessorPtr : public IWeakPtr {
public:
  std::unique_ptr<AccessorInterface> lock() override {
    return std::unique_ptr<AccessorInterface>();
  }
};


/// <summary>
/// Specialization of the <see cref="WeakAccessorPtr{T}"/> template for the <see cref="Navigation3D::IAccessors"/> interface.
/// </summary>
template <> class WeakAccessorPtr<Navigation3D::IAccessors> : public IWeakPtr {
  // Implementation of the accessors interfaces.
  class Accessors : public AccessorInterface {
  public:
    Accessors(std::shared_ptr<Navigation3D::IAccessors> accessors)
        : m_interface(std::forward<std::shared_ptr<Navigation3D::IAccessors>>(accessors)) {
    }

  protected:
    Navigation3D::ISpace3D *GetISpace3DInterface() override {
      return static_cast<Navigation3D::ISpace3D *>(m_interface.get());
    }
    Navigation3D::IView *GetIViewInterface() override {
      return static_cast<Navigation3D::IView *>(m_interface.get());
    }
    Navigation3D::IModel *GetIModelInterface() override {
      return static_cast<Navigation3D::IModel *>(m_interface.get());
    }
    Navigation3D::IPivot *GetIPivotInterface() override {
      return static_cast<Navigation3D::IPivot *>(m_interface.get());
    }
    Navigation3D::IHit *GetIHitInterface() override {
      return static_cast<Navigation3D::IHit *>(m_interface.get());
    }
    Navigation3D::IEvents *GetIEventsInterface() override {
      return static_cast<Navigation3D::IEvents *>(m_interface.get());
    }
    Navigation3D::IState *GetIStateInterface() override {
      return static_cast<Navigation3D::IState *>(m_interface.get());
    }

  private:
    std::shared_ptr<Navigation3D::IAccessors> m_interface;
  };

public:
  WeakAccessorPtr(std::shared_ptr<Navigation3D::IAccessors> &&accessors)
      : m_interface(std::forward<std::shared_ptr<Navigation3D::IAccessors>>(accessors)) {
  }

  std::unique_ptr<AccessorInterface> lock() override {
    return std::unique_ptr<AccessorInterface>(new Accessors(m_interface.lock()));
  }

protected:
  std::weak_ptr<Navigation3D::IAccessors> m_interface;
};

/// <summary>
/// Specialization of the <see cref="WeakAccessorPtr{T}"/> template for the <see cref="ActionInput::IActionAccessors"/> interface.
/// </summary>
template <> class WeakAccessorPtr<ActionInput::IActionAccessors> : public IWeakPtr {
  // Implementation of the accessors interfaces.
  class Accessors : public AccessorInterface {
  public:
    Accessors(std::shared_ptr<ActionInput::IActionAccessors> accessors)
        : m_interface(std::forward<std::shared_ptr<ActionInput::IActionAccessors>>(accessors)) {
    }

  protected:
    Navigation3D::IEvents *GetIEventsInterface() override {
      return static_cast<Navigation3D::IEvents *>(m_interface.get());
    }

  private:
    std::shared_ptr<ActionInput::IActionAccessors> m_interface;
  };

public:
  WeakAccessorPtr(std::shared_ptr<ActionInput::IActionAccessors> &&accessors)
      : m_interface(std::forward<std::shared_ptr<ActionInput::IActionAccessors>>(accessors)) {
  }

  std::unique_ptr<AccessorInterface> lock() override {
    return std::unique_ptr<AccessorInterface>(new Accessors(m_interface.lock()));
  }

protected:
  std::weak_ptr<ActionInput::IActionAccessors> m_interface;
};

/// <summary>
/// Template to allow defining the static members in the header file
/// </summary>
template <typename Ty_, typename I_> struct StaticSinkCollection {
protected:
  static CCookieCollection<I_> s_sinkCollection;
  static std::mutex s_mutex;
};

template <class Ty_, class I_>
CCookieCollection<I_> StaticSinkCollection<Ty_, I_>::s_sinkCollection;

/// <summary>
/// Mutex used to synchronize the trace output.
/// </summary>
template <class Ty_, class I_> std::mutex StaticSinkCollection<Ty_, I_>::s_mutex;

namespace Navigation3D {
/// <summary>
/// Class implements the interface to the navlib.
/// </summary>
class CNavlibInterface : public INavlib,
                         private StaticSinkCollection<CNavlibInterface, IWeakPtr> {
public:
  /// <summary>
  /// Initializes a new instance of the CNavlibInterface class.
  /// </summary>
  /// <param name="sink">Shared pointer to the instance implementing the IAccessors interface
  /// accessors and mutators.</param>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="rowMajor">true for row-major ordered matrices, false for column-major.</param>
  template <class T_>
  explicit CNavlibInterface(std::shared_ptr<T_> sink, bool multiThreaded = false,
                            bool rowMajor = false)
      : m_hdl(INVALID_NAVLIB_HANDLE),
        m_cookie(s_sinkCollection.insert(std::make_shared<WeakAccessorPtr<T_>>(std::move(sink))))
#if defined(_MSC_VER) && (_MSC_VER < 1800)
  {
    navlib::nlCreateOptions_t options = {sizeof(navlib::nlCreateOptions_t), multiThreaded,
                                         rowMajor ? navlib::row_major_order : navlib::none};

    m_createOptions = options;
  }
#else
        ,
        m_createOptions{sizeof(navlib::nlCreateOptions_t), multiThreaded,
                        rowMajor ? navlib::row_major_order : navlib::none} {
  }
#endif


  /// <summary>
  /// Initializes a new instance of the CNavlibInterface class.
  /// </summary>
  /// <param name="sink">Shared pointer to the instance implementing the IAccessors interface
  /// accessors and mutators.</param>
  /// <param name="multiThreaded">true to use multi-threading, false for single-threaded.</param>
  /// <param name="options">A combination of the <see cref="navlib::nlOptions_t"/> values.</param>
  template <class T_>
  explicit CNavlibInterface(std::shared_ptr<T_> sink, bool multiThreaded,
                            navlib::nlOptions_t options)
      : m_hdl(INVALID_NAVLIB_HANDLE),
        m_cookie(s_sinkCollection.insert(std::make_shared<WeakAccessorPtr<T_>>(std::move(sink))))
#if defined(_MSC_VER) && (_MSC_VER < 1800)
  {
    navlib::nlCreateOptions_t createOptions = {sizeof(navlib::nlCreateOptions_t), multiThreaded,
                                               options};

    m_createOptions = createOptions;
  }
#else
        ,
        m_createOptions({sizeof(navlib::nlCreateOptions_t), multiThreaded, options}) {
  }
#endif

  /// <summary>
  /// Clean up the resources
  /// </summary>
  ~CNavlibInterface() override {
    using namespace ::navlib;
    if (m_cookie) {
      s_sinkCollection.erase(m_cookie);
    }
    if (m_hdl != INVALID_NAVLIB_HANDLE) {
      NlClose(m_hdl);
    }
  }

#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  CNavlibInterface(const CNavlibInterface &) = delete;
  CNavlibInterface& operator=(const CNavlibInterface &) = delete;
  #else
private:
  CNavlibInterface(const CNavlibInterface &);
  CNavlibInterface &operator=(const CNavlibInterface &);
#endif

public :
  /// <summary>
  /// Close the connection to the 3D navigation library.
  /// </summary>
  void Close() override {
    using namespace ::navlib;
    if (m_hdl != INVALID_NAVLIB_HANDLE) {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (m_hdl != INVALID_NAVLIB_HANDLE) {
        NlClose(m_hdl);
        m_hdl = INVALID_NAVLIB_HANDLE;
      }
    }
  }

  /// <summary>
  /// Opens a connection to the 3D navigation library.
  /// </summary>
  void Open() override {
    Open(m_name);
  }

  /// <summary>
  /// Opens a connection to the 3D navigation library
  /// </summary>
  /// <param name="profileText">The text to display in the 3Dconnexion profile.</param>
  /// <exception cref="std::system_error">The connection to the library is already open.</exception>
  /// <exception cref="std::system_error">Cannot create a connection to the library.</exception>
  /// <exception cref="std::invalid_argument">The text for the profile is empty.</exception>
  void Open(std::string profileText) override {
    using namespace ::navlib;

    if (profileText.empty()) {
      throw std::invalid_argument("The text for the profile is empty.");
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_hdl != INVALID_NAVLIB_HANDLE) {
      throw std::system_error(navlib::make_error_code(navlib_errc::already_connected),
                              "Connection to the library is already open.");
    }

#if defined(WAMP_CLIENT) && (WAMP_CLIENT == 1)
    if (!m_session) {
      // The SSL context is required, and holds certificates
      ssl::context ctx{ssl::context::tlsv12_client};

      m_session = std::make_shared<tdx::wamp::client::NlSession>(ctx);
    }
    if (!m_session->is_running()) {
      std::error_code ec;
      m_session->run(ec);
      if (ec) {
        throw std::system_error(ec, "Cannot run navlib session.");
      }
    }
#endif

    auto isink = s_sinkCollection.at(m_cookie)->lock();
    std::vector<accessor_t> accessors;
    // Event accessors
    if (nullptr != GetInterface<IEvents>(isink)) {
      accessors.insert(
          accessors.end(),
          {{commands_activeCommand_k, nullptr, &CNavlibInterface::SetActiveCommand, m_cookie},
           {events_keyPress_k, nullptr, &CNavlibInterface::SetKeyPress, m_cookie},
           {events_keyRelease_k, nullptr, &CNavlibInterface::SetKeyRelease, m_cookie},
           {settings_changed_k, nullptr, &CNavlibInterface::SetSettingsChanged, m_cookie}});
    }

    // 3D space accessors
    if (nullptr != GetInterface<ISpace3D>(isink)) {
      accessors.insert(
          accessors.end(),
          {{coordinate_system_k, &CNavlibInterface::GetCoordinateSystem, nullptr, m_cookie},
           {views_front_k, &CNavlibInterface::GetFrontView, nullptr, m_cookie}});
    }

    // state accessors
    if (nullptr != GetInterface<IState>(isink)) {
      accessors.insert(
          accessors.end(),
          {{motion_k, nullptr, &CNavlibInterface::SetMotionFlag, m_cookie},
           {transaction_k, nullptr, &CNavlibInterface::SetTransaction, m_cookie}});
    }

    // view access
    if (nullptr != GetInterface<IView>(isink)) {
      accessors.insert(accessors.end(),
          {{view_affine_k, &CNavlibInterface::GetCameraMatrix, &CNavlibInterface::SetCameraMatrix,
            m_cookie},
           {view_constructionPlane_k, &CNavlibInterface::GetViewConstructionPlane, nullptr,
            m_cookie},
           {view_extents_k, &CNavlibInterface::GetViewExtents, &CNavlibInterface::SetViewExtents,
            m_cookie},
           {view_focusDistance_k, &CNavlibInterface::GetViewFocusDistance, nullptr, m_cookie},
           {view_fov_k, &CNavlibInterface::GetViewFOV, &CNavlibInterface::SetViewFOV, m_cookie},
           {view_frustum_k, &CNavlibInterface::GetViewFrustum, &CNavlibInterface::SetViewFrustum,
            m_cookie},
           {view_perspective_k, &CNavlibInterface::GetIsViewPerspective, nullptr, m_cookie},
           {view_target_k, &CNavlibInterface::GetCameraTarget, &CNavlibInterface::SetCameraTarget,
            m_cookie},
           {view_rotatable_k, &CNavlibInterface::GetIsViewRotatable, nullptr, m_cookie},
           {pointer_position_k, &CNavlibInterface::GetPointerPosition,
            &CNavlibInterface::SetPointerPosition, m_cookie}});
    }

    // pivot accessors
    if (nullptr != GetInterface<IPivot>(isink)) {
      accessors.insert(
          accessors.end(),
          {{pivot_position_k, &CNavlibInterface::GetPivotPosition, &CNavlibInterface::SetPivotPosition, m_cookie},
            {pivot_user_k, &CNavlibInterface::IsUserPivot, nullptr, m_cookie},
            {pivot_visible_k, &CNavlibInterface::GetPivotVisible, &CNavlibInterface::SetPivotVisible, m_cookie}});
    }

    // hit testing for auto pivot algorithm etc.
    if (nullptr != GetInterface<IHit>(isink)) {
      accessors.insert(accessors.end(),
          {{hit_lookfrom_k, nullptr, &CNavlibInterface::SetHitLookFrom, m_cookie},
           {hit_direction_k, nullptr, &CNavlibInterface::SetHitDirection, m_cookie},
           {hit_aperture_k, nullptr, &CNavlibInterface::SetHitAperture, m_cookie},
           {hit_lookat_k, &CNavlibInterface::GetHitLookAt, nullptr, m_cookie},
           {hit_selectionOnly_k, nullptr, &CNavlibInterface::SetHitSelectionOnly, m_cookie}});
    }

    // model access
    if (nullptr != GetInterface<IModel>(isink)) {
      accessors.insert(accessors.end(),
          {{model_extents_k, &CNavlibInterface::GetModelExtents, nullptr, m_cookie},
              {selection_empty_k, &CNavlibInterface::GetIsSelectionEmpty, nullptr, m_cookie},
              {selection_extents_k, &CNavlibInterface::GetSelectionExtents, nullptr, m_cookie},
              {selection_affine_k, &CNavlibInterface::GetSelectionTransform,
               &CNavlibInterface::SetSelectionTransform, m_cookie},
              {model_unitsToMeters_k, &CNavlibInterface::GetUnitsToMeters, nullptr, m_cookie},
              {model_floorPlane_k, &CNavlibInterface::GetFloorPlane, nullptr, m_cookie}});
    }

    // Create the navlib instance
    long error = NlCreate(&m_hdl, profileText.c_str(), accessors.data(),
                 accessors.size(),  &m_createOptions);

    if (error != 0) {
      throw std::system_error(
          navlib::make_error_code(static_cast<navlib_errc::navlib_errc_t>(error & 0xffff)),
          "Cannot create a connection to the 3DMouse.");
    }

    m_name = std::move(profileText);
  }

  /// <summary>
  /// Writes the value of a property to the navlib.
  /// </summary>
  /// <param name="propertyName">The <see cref="navlib::property_t"/> name of the navlib property to
  /// write.</param>
  /// <param name="value">The <see cref="navlib::value"/> to write.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib.</exception>
  long Write(const std::string &propertyName, const navlib::value &value) override {
    if (m_hdl == INVALID_NAVLIB_HANDLE) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
    }

    long resultCode = WriteValue(m_hdl, propertyName.c_str(), &value);

    return resultCode;
  }

  /// <summary>
  /// Reads the value of a navlib property.
  /// </summary>
  /// <param name="propertyName">The <see cref="navlib::property_t"/> name of the navlib property to
  /// read.</param>
  /// <param name="value">The <see cref="navlib::value"/> to read.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib.</exception>
  long Read(const std::string &propertyName, navlib::value &value) const override {
    if (m_hdl == INVALID_NAVLIB_HANDLE) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
    }

    long resultCode = ReadValue(m_hdl, propertyName.c_str(), &value);

    return resultCode;
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
    if (m_hdl == INVALID_NAVLIB_HANDLE) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
    }

    navlib::value value(&string[0], string.length());
    long resultCode = ReadValue(m_hdl, propertyName.c_str(), &value);
    if ((resultCode & 0xffff) == static_cast<int>(navlib::navlib_errc::insufficient_buffer)) {
      string.resize(value.string.length);
      value = navlib::value(&string[0], string.length());
      resultCode = ReadValue(m_hdl, propertyName.c_str(), &value);
    }

    if (resultCode == 0) {
      string.resize(value.string.length);
    }

    return resultCode;
  }

private:
  typedef std::unique_ptr<AccessorInterface> isink_t;

  template <typename F>
  static long GetValue(navlib::param_t cookie, navlib::property_t property, navlib::value_t *value,
                       F fn) {
    try {
      isink_t isink = s_sinkCollection.at(cookie)->lock();
      long result = fn(std::move(isink));
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
      std::unique_lock<std::mutex> lock(s_mutex);
      std::clog << "GetValue(0x" << std::hex << cookie << std::dec << ", " << property << ", "
                << *value << ") result =0x" << std::hex << result << std::endl;
#endif
      return result;
    }
    catch (const std::out_of_range &e) {
      std::unique_lock<std::mutex> lock(s_mutex);
      std::cerr << "std::out_of_range exception thrown in GetValue(0x" << std::hex << cookie
                << std::dec << ", " << property << ", value)\n"
                << *value << e.what() << std::endl;
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    } catch (const std::exception &e) {
      std::cerr << "Uncaught exception thrown in GetValue(0x" << std::hex << cookie << std::dec
                << ", " << property << ", value)\n"
                << *value << e.what() << std::endl;
    }
    return navlib::make_result_code(navlib::navlib_errc::error);
  }

  template <typename F>
  static long SetValue(navlib::param_t cookie, navlib::property_t property,
                       const navlib::value_t *value, F fn) {
    try {
      isink_t isink = s_sinkCollection.at(cookie)->lock();
      long result = fn(std::move(isink));
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
      std::clog << "SetValue(0x" << std::hex << cookie << std::dec << ", " << property << ", "
                << *value << ") result =0x" << std::hex << result << std::endl;
#endif
      return result;
    }
    catch (const std::out_of_range &e) {
      std::unique_lock<std::mutex> lock(s_mutex);
      std::cerr << "std::out_of_range exception thrown in SetValue(0x" << std::hex << cookie
                << std::dec << ", " << property << ", value)\n"
                << *value << e.what() << std::endl;
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    } catch (const std::exception &e) {
      std::unique_lock<std::mutex> lock(s_mutex);
      std::cerr << "Uncaught exception thrown in SetValue(0x" << std::hex << cookie << std::dec
                << ", " << property << "," << *value << ")\n"
                << e.what() << std::endl;
    }
    return navlib::make_result_code(navlib::navlib_errc::error);
  }

  /// <summary>
  /// IEvents accessors and mutators
  /// </summary>
  static long SetActiveCommand(navlib::param_t cookie, navlib::property_t property,
                               const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IEvents>(isink)->SetActiveCommand(static_cast<const char *>(*value));
    });
  }

  static long SetSettingsChanged(navlib::param_t cookie, navlib::property_t property,
                                 const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IEvents>(isink)->SetSettingsChanged(*value);
    });
  }

  static long SetKeyPress(navlib::param_t cookie, navlib::property_t property,
                          const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IEvents>(isink)->SetKeyPress(*value);
    });
  }
  static long SetKeyRelease(navlib::param_t cookie, navlib::property_t property,
                            const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IEvents>(isink)->SetKeyRelease(*value);
    });
  }

  /// <summary>
  /// IHit accessors and mutators
  /// </summary>
  static long GetHitLookAt(navlib::param_t cookie, navlib::property_t property,
                           navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IHit>(isink)->GetHitLookAt(*value);
    });
  }
  static long SetHitAperture(navlib::param_t cookie, navlib::property_t property,
                             const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IHit>(isink)->SetHitAperture(*value);
    });
  }
  static long SetHitDirection(navlib::param_t cookie, navlib::property_t property,
                              const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IHit>(isink)->SetHitDirection(*value);
    });
  }
  static long SetHitLookFrom(navlib::param_t cookie, navlib::property_t property,
                             const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IHit>(isink)->SetHitLookFrom(*value);
    });
  }
  static long SetHitSelectionOnly(navlib::param_t cookie, navlib::property_t property,
                                  const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IHit>(isink)->SetHitSelectionOnly(*value);
    });
  }

  /// <summary>
  /// IModel accessors and mutators
  /// </summary>
  static long GetModelExtents(navlib::param_t cookie, navlib::property_t property,
                              navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetModelExtents(*value);
    });
  }
  static long GetSelectionExtents(navlib::param_t cookie, navlib::property_t property,
                                  navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetSelectionExtents(*value);
    });
  }
  static long GetSelectionTransform(navlib::param_t cookie, navlib::property_t property,
                                    navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetSelectionTransform(*value);
    });
  }
  static long GetIsSelectionEmpty(navlib::param_t cookie, navlib::property_t property,
                                  navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetIsSelectionEmpty(*value);
    });
  }
  static long SetSelectionTransform(navlib::param_t cookie, navlib::property_t property,
                                    const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->SetSelectionTransform(*value);
    });
  }
  static long GetUnitsToMeters(navlib::param_t cookie, navlib::property_t property,
                                  navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetUnitsToMeters(*value);
    });
  }
  static long GetFloorPlane(navlib::param_t cookie, navlib::property_t property,
                               navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IModel>(isink)->GetFloorPlane(*value);
    });
  }

  /// <summary>
  /// IPivot accessors and mutators
  /// </summary>
  static long GetPivotPosition(navlib::param_t cookie, navlib::property_t property,
                               navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IPivot>(isink)->GetPivotPosition(*value);
    });
  }
  static long IsUserPivot(navlib::param_t cookie, navlib::property_t property,
                          navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IPivot>(isink)->IsUserPivot(*value);
    });
  }
  static long SetPivotPosition(navlib::param_t cookie, navlib::property_t property,
                               const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IPivot>(isink)->SetPivotPosition(*value);
    });
  }
  static long GetPivotVisible(navlib::param_t cookie, navlib::property_t property,
                              navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IPivot>(isink)->GetPivotVisible(*value);
    });
  }
  static long SetPivotVisible(navlib::param_t cookie, navlib::property_t property,
                              const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IPivot>(isink)->SetPivotVisible(*value);
    });
  }

  /// <summary>
  /// ISpace3D accessors and mutators
  /// </summary>
  static long GetCoordinateSystem(navlib::param_t cookie, navlib::property_t property,
                                  navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<ISpace3D>(isink)->GetCoordinateSystem(*value);
    });
  }
  static long GetFrontView(navlib::param_t cookie, navlib::property_t property,
                           navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<ISpace3D>(isink)->GetFrontView(*value);
    });
  }

  /// <summary>
  /// IState accessors and mutators
  /// </summary>
  static long SetTransaction(navlib::param_t cookie, navlib::property_t property,
                             const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IState>(isink)->SetTransaction(*value);
    });
  }
  static long SetMotionFlag(navlib::param_t cookie, navlib::property_t property,
                            const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IState>(isink)->SetMotionFlag(*value);
    });
  }

  /// <summary>
  /// IView accessors and mutators
  /// </summary>
  static long GetCameraMatrix(navlib::param_t cookie, navlib::property_t property,
                              navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetCameraMatrix(*value);
    });
  }
  static long GetCameraTarget(navlib::param_t cookie, navlib::property_t property,
                              navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetCameraTarget(*value);
    });
  }
  static long GetPointerPosition(navlib::param_t cookie, navlib::property_t property,
                                 navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetPointerPosition(*value);
    });
  }
  static long GetViewConstructionPlane(navlib::param_t cookie, navlib::property_t property,
                                       navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetViewConstructionPlane(*value);
    });
  }
  static long GetViewExtents(navlib::param_t cookie, navlib::property_t property,
                             navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetViewExtents(*value);
    });
  }
  static long GetViewFOV(navlib::param_t cookie, navlib::property_t property,
                         navlib::value_t *value) {
    return GetValue(cookie, property, value,
                    [&](isink_t isink) { return GetInterface<IView>(isink)->GetViewFOV(*value); });
  }
  static long GetViewFrustum(navlib::param_t cookie, navlib::property_t property,
                             navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetViewFrustum(*value);
    });
  }
  static long GetIsViewPerspective(navlib::param_t cookie, navlib::property_t property,
                                   navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetIsViewPerspective(*value);
    });
  }
  static long GetIsViewRotatable(navlib::param_t cookie, navlib::property_t property,
                                 navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetIsViewRotatable(*value);
    });
  }
  static long GetViewFocusDistance(navlib::param_t cookie, navlib::property_t property,
                                 navlib::value_t *value) {
    return GetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->GetViewFocusDistance(*value);
    });
  }
  static long SetCameraMatrix(navlib::param_t cookie, navlib::property_t property,
                              const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->SetCameraMatrix(*value);
    });
  }
  static long SetCameraTarget(navlib::param_t cookie, navlib::property_t property,
                              const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->SetCameraTarget(*value);
    });
  }
  static long SetPointerPosition(navlib::param_t cookie, navlib::property_t property,
                                 const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->SetPointerPosition(*value);
    });
  }
  static long SetViewExtents(navlib::param_t cookie, navlib::property_t property,
                             const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->SetViewExtents(*value);
    });
  }
  static long SetViewFOV(navlib::param_t cookie, navlib::property_t property,
                         const navlib::value_t *value) {
    return SetValue(cookie, property, value,
                    [&](isink_t isink) { return GetInterface<IView>(isink)->SetViewFOV(*value); });
  }
  static long SetViewFrustum(navlib::param_t cookie, navlib::property_t property,
                             const navlib::value_t *value) {
    return SetValue(cookie, property, value, [&](isink_t isink) {
      return GetInterface<IView>(isink)->SetViewFrustum(*value);
    });
  }

private:
  /// <summary>
  /// Read a <see cref="navlib::property_t"/> value from the navlib.
  /// </summary>
  /// <param name="nh">The <see cref="navlib::nlHandle_t"/> to the navigation library returned by
  /// a previous call to <see cref="navlib::NlCreate"/>.</param>
  /// <param name="name">The name of the navlib property to read.</param>
  /// <param name="value">Pointer to a <see cref="navlib::value_t"/> to receive the value.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib / 3D Mouse.</exception>
  long ReadValue(navlib::nlHandle_t nh, navlib::property_t name, navlib::value_t *value) const {
    using namespace ::navlib;
    try {
      long resultCode = NlReadValue(nh, name, value);
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
      std::unique_lock<std::mutex> lock(s_mutex);
      std::clog << "NlReadValue(0x" << std::hex << nh << std::dec << ", " << name << ", " << *value
                << ") result =0x" << std::hex << resultCode << std::endl;
#endif
      return resultCode;
    }
    catch (const std::exception &e) {
      std::unique_lock<std::mutex> lock(s_mutex);
      std::cerr << "exception thrown in NlReadValue(0x" << std::hex << nh << std::dec
                << ", " << name << ", value)\n"
                << *value << " " << e.what() << std::endl;
    }
    return navlib::make_result_code(navlib::navlib_errc::error);
  }

  /// <summary>
  /// Write a <see cref="navlib::property_t"/> value to the navlib.
  /// </summary>
  /// <param name="nh">The <see cref="navlib::nlHandle_t"/> to the navigation library returned by
  /// a previous call to <see cref="navlib::NlCreate"/>.</param>
  /// <param name="name">The name of the navlib property to read.</param>
  /// <param name="value">Pointer to a <see cref="navlib::value_t"/> to receive the value.</param>
  /// <returns>0 =no error, otherwise a value from <see cref="navlib::make_result_code"/>.</returns>
  /// <exception cref="std::system_error">No connection to the navlib / 3D Mouse.</exception>
  long WriteValue(navlib::nlHandle_t nh, navlib::property_t name,
                  const navlib::value_t *value) {
    using namespace ::navlib;
    try {
      long resultCode = NlWriteValue(nh, name, value);
#if defined(_DEBUG) && defined(TRACE_NAVLIB)
      std::unique_lock<std::mutex> lock(s_mutex);
      std::clog << "NlWriteValue(0x" << std::hex << nh << std::dec << ", " << name << ", " << *value
                << ") result =0x" << std::hex << resultCode << std::endl;
#endif
      return resultCode;
    }
    catch (const std::exception &e) {
      std::unique_lock<std::mutex> lock(s_mutex);
      std::cerr << "exception thrown in NlWriteValue(0x" << std::hex << nh << std::dec
                << ", " << name << ", value)\n"
                << *value << " " << e.what() << std::endl;
    }
    return navlib::make_result_code(navlib::navlib_errc::error);
  }

#if defined(WAMP_CLIENT) && (WAMP_CLIENT == 1)
private:
  long NlCreate(navlib::nlHandle_t* nh, const char *appname,
                const navlib::accessor_t property_accessors[], size_t accessor_count,
                const navlib::nlCreateOptions_t *options) {
    if (nh == nullptr) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    }
    return m_session->NlCreate(*nh, appname, property_accessors, accessor_count, options);
  }

  long NlClose(navlib::nlHandle_t nh) {
    return m_session->NlClose(nh);
  }

  long NlReadValue(navlib::nlHandle_t nh, navlib::property_t name, navlib::value_t *value) const {
    if (value == nullptr) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    }

    if (!m_session->is_running()) {
      throw std::system_error(navlib::make_error_code(navlib::navlib_errc::invalid_operation),
                              "No active NL-Session.");
    }

    return m_session->NlReadValue(nh, name, *value);
  }

  long NlWriteValue(navlib::nlHandle_t nh, navlib::property_t name,
                                           const navlib::value_t *value) {
    if (value == nullptr) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    }

    if (!m_session->is_running()) {
      throw std::system_error(navlib::make_error_code(navlib::navlib_errc::invalid_operation),
                              "No active NL-Session.");
    }

    return m_session->NlWriteValue(nh, name, *value);
  }

  mutable std::shared_ptr<tdx::wamp::client::NlSession> m_session;
#endif

private:
  navlib::nlHandle_t m_hdl;
  std::mutex m_mutex;
  navlib::param_t m_cookie;
  std::string m_name;
  navlib::nlCreateOptions_t m_createOptions;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // CNavigationModelImpl_HPP_INCLUDED
