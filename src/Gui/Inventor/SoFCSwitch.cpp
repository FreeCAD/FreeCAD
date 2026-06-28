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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Inventor/SoPath.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/misc/SoChildList.h>
#endif

#include "Inventor/SoFCSwitch.h"

// Override path for the current thread; only switches on it are forced visible.
// thread_local because the action that sets it is the one that traverses.
static thread_local const SoPath* s_overridePath = nullptr;

static bool pathContains(const SoPath* path, const SoNode* node)
{
    for (int i = 0, count = path->getLength(); i < count; ++i) {
        if (path->getNode(i) == node) {
            return true;
        }
    }
    return false;
}

SO_NODE_SOURCE(SoFCSwitch)

void SoFCSwitch::initClass()
{
    SO_NODE_INIT_CLASS(SoFCSwitch, SoSwitch, "Switch");
}

SoFCSwitch::SoFCSwitch()
{
    SO_NODE_CONSTRUCTOR(SoFCSwitch);
    SO_NODE_ADD_FIELD(defaultChild, (SO_SWITCH_NONE));
}

SoFCSwitch::OverrideScope::OverrideScope(const SoPath* path)
    : prev(s_overridePath)
{
    if (path) {
        path->ref();
    }
    s_overridePath = path;
}

SoFCSwitch::OverrideScope::~OverrideScope()
{
    if (s_overridePath) {
        s_overridePath->unrefNoDelete();
    }
    s_overridePath = prev;
}

void SoFCSwitch::doAction(SoAction* action)
{
    if (s_overridePath && this->whichChild.getValue() == SO_SWITCH_NONE
        && pathContains(s_overridePath, this)) {
        int numindices = 0;
        const int* indices = nullptr;
        SoAction::PathCode pathcode = action->getPathCode(numindices, indices);
        if (pathcode == SoAction::IN_PATH) {
            // intermediate hidden switch: follow the child recorded in the path
            for (int i = 0; i < numindices; ++i) {
                SoSwitchElement::set(action->getState(), indices[i]);
                this->getChildren()->traverse(action, indices[i]);
            }
            return;
        }
        if (pathcode == SoAction::BELOW_PATH || pathcode == SoAction::NO_PATH) {
            // tail of the path: fall back to the child this object would show
            const int idx = this->defaultChild.getValue();
            if (idx >= 0 && idx < this->getNumChildren()) {
                SoSwitchElement::set(action->getState(), idx);
                this->getChildren()->traverse(action, idx);
                return;
            }
        }
    }
    inherited::doAction(action);
}

void SoFCSwitch::getBoundingBox(SoGetBoundingBoxAction* action)
{
    this->doAction(action);
}
