// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                    *
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

#include <FCConfig.h>

#ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
#else
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# include <GL/gl.h>
#endif

#include <Inventor/elements/SoCacheElement.h>
#include <algorithm>

#include "So3DAnnotation.h"
#include <Gui/Selection/Selection.h>

using namespace Gui;

SO_ELEMENT_SOURCE(SoDelayedAnnotationsElement);

bool SoDelayedAnnotationsElement::isProcessingDelayedPaths = false;

void SoDelayedAnnotationsElement::init(SoState* state)
{
    SoElement::init(state);
    paths.clear();
}

void SoDelayedAnnotationsElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(SoDelayedAnnotationsElement, inherited);

    SO_ENABLE(SoGLRenderAction, SoDelayedAnnotationsElement);
}

void SoDelayedAnnotationsElement::addDelayedPath(SoState* state, SoPath* path, int priority)
{
    auto elt = static_cast<SoDelayedAnnotationsElement*>(state->getElementNoPush(classStackIndex));

    // add to unified storage with specified priority (default = 0)
    elt->paths.emplace_back(path, priority);
}

SoPathList SoDelayedAnnotationsElement::getDelayedPaths(SoState* state)
{
    auto elt = static_cast<SoDelayedAnnotationsElement*>(state->getElementNoPush(classStackIndex));

    if (elt->paths.empty()) {
        return {};
    }

    // sort by priority (lower numbers render first)
    std::stable_sort(
        elt->paths.begin(),
        elt->paths.end(),
        [](const PriorityPath& a, const PriorityPath& b) { return a.priority < b.priority; }
    );

    SoPathList sortedPaths;
    for (const auto& priorityPath : elt->paths) {
        sortedPaths.append(priorityPath.path);
    }

    // Clear storage
    elt->paths.clear();

    return sortedPaths;
}

void SoDelayedAnnotationsElement::processDelayedPathsWithPriority(SoState* state, SoGLRenderAction* action)
{
    auto elt = static_cast<SoDelayedAnnotationsElement*>(state->getElementNoPush(classStackIndex));

    if (elt->paths.empty()) {
        return;
    }

    std::stable_sort(
        elt->paths.begin(),
        elt->paths.end(),
        [](const PriorityPath& a, const PriorityPath& b) { return a.priority < b.priority; }
    );

    isProcessingDelayedPaths = true;

    for (const auto& priorityPath : elt->paths) {
        SoPathList singlePath;
        singlePath.append(priorityPath.path);

        action->apply(singlePath, TRUE);
    }

    isProcessingDelayedPaths = false;

    elt->paths.clear();
}

SO_NODE_SOURCE(So3DAnnotation);

bool So3DAnnotation::render = false;

So3DAnnotation::So3DAnnotation()
{
    SO_NODE_CONSTRUCTOR(So3DAnnotation);
}

void So3DAnnotation::initClass()
{
    SO_NODE_INIT_CLASS(So3DAnnotation, SoSeparator, "3DAnnotation");
}

void So3DAnnotation::GLRender(SoGLRenderAction* action)
{
    switch (action->getCurPathCode()) {
        case SoAction::NO_PATH:
        case SoAction::BELOW_PATH:
            this->GLRenderBelowPath(action);
            break;
        case SoAction::OFF_PATH:
            // do nothing. Separator will reset state.
            break;
        case SoAction::IN_PATH:
            this->GLRenderInPath(action);
            break;
    }
}

void So3DAnnotation::GLRenderBelowPath(SoGLRenderAction* action)
{
    if (render) {
        inherited::GLRenderBelowPath(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        SoDelayedAnnotationsElement::addDelayedPath(action->getState(), action->getCurPath()->copy());
    }
}

void So3DAnnotation::GLRenderInPath(SoGLRenderAction* action)
{
    if (render) {
        inherited::GLRenderInPath(action);
    }
    else {
        SoCacheElement::invalidate(action->getState());
        SoDelayedAnnotationsElement::addDelayedPath(action->getState(), action->getCurPath()->copy());
    }
}

void So3DAnnotation::GLRenderOffPath(SoGLRenderAction* /* action */)
{
    // should never render, this is a separator node
}
