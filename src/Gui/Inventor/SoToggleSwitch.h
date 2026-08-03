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

#pragma once

#include <FCGlobal.h>

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoSwitch.h>

#include <FCGlobal.h>

/**
 * A switch that can be used to show or hide all child nodes
 */
class GuiExport SoToggleSwitch: public SoSwitch
{
    SO_NODE_HEADER(SoToggleSwitch);

public:
    static void initClass();
    SoToggleSwitch();

    // the switch is on be default
    SoSFBool on;
    // toggles the switch state
    void toggle();

protected:
    ~SoToggleSwitch() override = default;

    void notify(SoNotList* notList) override;

private:
    using inherited = SoSwitch;
};
