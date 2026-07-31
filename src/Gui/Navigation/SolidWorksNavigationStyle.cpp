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
constexpr auto CTRL = NavigationInputState::CtrlDown;
constexpr auto SHIFT = NavigationInputState::ShiftDown;

using Mode = NavigationStyle::ViewerMode;
constexpr auto Select = Mode::SELECTION;
constexpr auto Pan = Mode::PANNING;
constexpr auto Rotate = Mode::DRAGGING;
constexpr auto Zoom = Mode::ZOOMING;

constexpr NavigationRule solidWorksRules[] {
    // Primary bindings.
    bind(LMB, Select),
    bind(CTRL | LMB, Select),
    bind(CTRL | MMB, Pan, ownedBy(MMB)),
    bind(MMB, Rotate, ownedBy(MMB)),
    bind(SHIFT | MMB, Zoom, ownedBy(MMB)),
};

constexpr NavigationProfile solidWorksProfile {
    solidWorksRules,
    QT_TR_NOOP("Press left mouse button"),
    QT_TR_NOOP("Press Ctrl and middle mouse button"),
    QT_TR_NOOP("Press middle mouse button"),
    QT_TR_NOOP("Scroll mouse wheel"),
};

}  // namespace

/* TRANSLATOR Gui::SolidWorksNavigationStyle */

TYPESYSTEM_SOURCE(Gui::SolidWorksNavigationStyle, Gui::MappedNavigationStyle)

SolidWorksNavigationStyle::SolidWorksNavigationStyle() = default;

SolidWorksNavigationStyle::~SolidWorksNavigationStyle() = default;

const NavigationProfile& SolidWorksNavigationStyle::profile() const
{
    return solidWorksProfile;
}
