// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#include "SoToggleSwitch.h"

SO_NODE_SOURCE(SoToggleSwitch)

void SoToggleSwitch::initClass()
{
    SO_NODE_INIT_CLASS(SoToggleSwitch, SoSwitch, "Switch");
}

SoToggleSwitch::SoToggleSwitch()
{
    SO_NODE_CONSTRUCTOR(SoToggleSwitch);
    SO_NODE_ADD_FIELD(on, (1));

    whichChild = SO_SWITCH_ALL;
}

void SoToggleSwitch::toggle()
{
    on = !on.getValue();
}

void SoToggleSwitch::notify(SoNotList* notList)
{
    assert(notList);
    if (notList->getLastField() == &on) {
        whichChild = on.getValue()? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }

    inherited::notify(notList);
}
