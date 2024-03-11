#ifndef IEvents_HPP_INCLUDED
#define IEvents_HPP_INCLUDED
// <copyright file="IEvents.hpp" company="3Dconnexion">
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
// $Id: IEvents.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>

// navlib
#include <navlib/navlib_types.h>

//stdlib
#include <string>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The Events interface
/// </summary>
class IEvents {
public:
#if !defined(_MSC_VER) || (_MSC_VER > 1700)
  virtual ~IEvents() = default;
#else
  virtual ~IEvents() = 0 {
  }
#endif

  /// <summary>
  /// Is called when the user invokes an application command from the SpaceMouse.
  /// </summary>
  /// <param name="commandId">The id of the command to invoke.</param>
  /// <returns>The result of the function: 0 = no error, otherwise &lt;0.</returns>
  virtual long SetActiveCommand(std::string commandId) = 0;

  /// <summary>
  /// Is called when the navigation settings change.
  /// </summary>
  /// <param name="count">The change count.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetSettingsChanged(long count) = 0;

  /// <summary>
  /// Is invoked when the user releases a key on the 3D Mouse, which has been programmed to send a
  /// virtual key code.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key pressed.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetKeyPress(long vkey) = 0;

  /// <summary>
  /// Is invoked when the user releases a key on the 3D Mouse, which has been programmed to send a
  /// virtual key code.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key released.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetKeyRelease(long vkey) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IEvents_HPP_INCLUDED
