#ifndef IActionAccessors_HPP_INCLUDED
#define IActionAccessors_HPP_INCLUDED
// <copyright file="IActionAccessors.hpp" company="3Dconnexion">
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

#include <SpaceMouse/IEvents.hpp>

namespace TDx {
namespace SpaceMouse {
namespace ActionInput {
/// <summary>
/// The accessor interface to the client action input properties.
/// </summary>
class IActionAccessors : public Navigation3D::IEvents {
};
} // namespace ActionInput
} // namespace SpaceMouse
} // namespace TDx
#endif // IActionAccessors_HPP_INCLUDED
