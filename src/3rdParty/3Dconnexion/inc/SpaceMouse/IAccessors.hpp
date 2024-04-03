#ifndef IAccessors_HPP_INCLUDED
#define IAccessors_HPP_INCLUDED
// <copyright file="IAccessors.hpp" company="3Dconnexion">
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
// $Id: IAccessors.hpp 16047 2019-04-05 12:51:24Z mbonk $
//
// </history>

#include <SpaceMouse/IEvents.hpp>
#include <SpaceMouse/IHit.hpp>
#include <SpaceMouse/IModel.hpp>
#include <SpaceMouse/IPivot.hpp>
#include <SpaceMouse/ISpace3D.hpp>
#include <SpaceMouse/IState.hpp>
#include <SpaceMouse/IView.hpp>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The accessor interface to the client 3D properties.
/// </summary>
class IAccessors : public ISpace3D,
                    public IView,
                    public IModel,
                    public IPivot,
                    public IHit,
                    public IEvents,
                    public IState {};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IAccessors_HPP_INCLUDED
