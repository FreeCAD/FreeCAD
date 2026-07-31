// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "Navigation/MappedNavigationStyle.h"

#include <QCoreApplication>

using namespace Gui;

namespace
{

constexpr auto LMB = NavigationInputState::LeftDown;
constexpr auto MMB = NavigationInputState::MiddleDown;
constexpr auto RMB = NavigationInputState::RightDown;
constexpr auto CTRL = NavigationInputState::CtrlDown;
constexpr auto SHIFT = NavigationInputState::ShiftDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule blenderRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(LMB | RMB, Pan, ownedBy(LMB | RMB)),
    bind(SHIFT | MMB, Pan, ownedBy(MMB)),
    bind(MMB, Rotate, ownedBy(MMB)),
    bind(CTRL | SHIFT | RMB, Zoom, ownedBy(RMB)),
    bind(CTRL | MMB, Zoom, ownedBy(MMB)),
};

constexpr NavigationProfile blenderProfile {
    .rules = blenderRules,
    .selectionDescription = QT_TR_NOOP("Press left mouse button"),
    .panDescription = QT_TR_NOOP("Press Shift and middle mouse button"),
    .rotateDescription = QT_TR_NOOP("Press middle mouse button"),
    .zoomDescription = QT_TR_NOOP("Scroll mouse wheel"),
    .editingSelectionPolicy = EditingSelectionPolicy::CancelOnLeftRightChord,
};

}  // namespace

// ----------------------------------------------------------------------------------

/* TRANSLATOR Gui::BlenderNavigationStyle */

TYPESYSTEM_SOURCE(Gui::BlenderNavigationStyle, Gui::MappedNavigationStyle)

BlenderNavigationStyle::BlenderNavigationStyle() = default;

BlenderNavigationStyle::~BlenderNavigationStyle() = default;

const NavigationProfile& BlenderNavigationStyle::profile() const
{
    return blenderProfile;
}
