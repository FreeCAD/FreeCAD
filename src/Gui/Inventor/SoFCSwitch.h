// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 FreeCAD contributors                                *
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

#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/nodes/SoSwitch.h>

class SoPath;

/**
 * SoSwitch that, while an OverrideScope is active, traverses defaultChild for
 * switches on the scoped path even when whichChild is SO_SWITCH_NONE. Lets a
 * hidden object render on top (e.g. tree preselection) without changing its
 * visibility, while leaving its hidden children untouched.
 */
class GuiExport SoFCSwitch: public SoSwitch
{
    SO_NODE_HEADER(SoFCSwitch);

public:
    static void initClass();
    SoFCSwitch();

    // Child traversed under override when whichChild is SO_SWITCH_NONE
    SoSFInt32 defaultChild;

    void doAction(SoAction* action) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;

    // RAII activation of the default-child override, scoped to the given path
    struct GuiExport OverrideScope
    {
        explicit OverrideScope(const SoPath* path);
        ~OverrideScope();
        OverrideScope(const OverrideScope&) = delete;
        OverrideScope& operator=(const OverrideScope&) = delete;

    private:
        const SoPath* scoped = nullptr;
        const SoPath* prev = nullptr;
    };

protected:
    ~SoFCSwitch() override = default;

private:
    using inherited = SoSwitch;
};
