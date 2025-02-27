#ifndef CCookieCollection_HPP_INCLUDED
#define CCookieCollection_HPP_INCLUDED
// <copyright file="CCookieCollection.hpp" company="3Dconnexion">
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
// $Id: CCookieCollection.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>
#include <navlib/navlib_types.h>
// stdlib
#include <chrono>
#include <map>
#include <memory>
#include <stdexcept>
#if (!defined(_MSC_VER) || (_MSC_VER > 1600))
#include <mutex>
#else
#pragma warning(disable : 4482) // non-standard
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
namespace std {
using boost::lock_guard;
using boost::mutex;
using boost::unique_lock;
} // namespace std
#endif

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// The class maps a cookie to a shared_ptr<typeparam name="T"></typeparam>.
/// </summary>
template <class T>
class CCookieCollection : protected std::map<navlib::param_t, std::shared_ptr<T>> {
  typedef std::map<navlib::param_t, std::shared_ptr<T>> map_t;

public:
  typedef typename map_t::size_type size_type;

  /// <summary>
  /// Gets the <see cref="std::shared_ptr{T}"/> corresponding to the passed in cookie.
  /// </summary>
  /// <param name="cookie">The <see cref="navlib::param_t"/> to search for.</param>
  /// <returns>The <see cref="std::shared_ptr{T}"/>.</returns>
  /// <exception cref="std::out_of_range">If the cookie does not exist.</exception>
  std::shared_ptr<T> at(const navlib::param_t &cookie) {
    std::lock_guard<std::mutex> guard(m_mutex);
    typename map_t::iterator iter = map_t::find(cookie);
    if (iter != map_t::end()) {
      return iter->second;
    }

    throw std::out_of_range("Cookie does not exist in the Collection");
  }

  /// <summary>
  /// Removes the elements that match the cookie.
  /// </summary>
  /// <param name="cookie">The cookie entry to remove.</param>
  /// <returns>The number of elements that have been removed.</returns>
  size_type erase(const navlib::param_t &cookie) {
    std::lock_guard<std::mutex> guard(m_mutex);
    return map_t::erase(cookie);
  }

  /// <summary>
  /// Inserts a <see cref="std::shared_ptr{T}"/> and returns a cookie that is needed to retrieve it later.
  /// </summary>
  /// <param name="sp">The <see cref="std::shared_ptr{T}"/> to insert.</param>
  /// <returns>A cookie that is needed to find the shared pointer.</returns>
  navlib::param_t insert(std::shared_ptr<T> sp) {
    navlib::param_t param = 0;
    if (sp) {
      std::lock_guard<std::mutex> guard(m_mutex);
      do {
        using namespace std::chrono;
        param = static_cast<navlib::param_t>(
            duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
      } while (map_t::find(param) != map_t::end());
      (*this)[param] = std::move(sp);
    }
    return param;
  }

protected:
  /// <summary>
  /// When changing the contents of the collection always use the mutex as a guard
  /// </summary>
  std::mutex m_mutex;
};
} // namespace SpaceMouse
} // namespace TDx
#endif // CCookieCollection_HPP_INCLUDED
