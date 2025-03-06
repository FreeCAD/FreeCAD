/***************************************************************************
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                    *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <Inventor/elements/SoCacheElement.h>
#endif

#include "So3DAnnotation.h"

using namespace Gui;

SO_ELEMENT_SOURCE(SoDelayedAnnotationsElement);

void SoDelayedAnnotationsElement::init(SoState* state)
{
    SoElement::init(state);
}

void SoDelayedAnnotationsElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(SoDelayedAnnotationsElement, inherited);

    SO_ENABLE(SoGLRenderAction, SoDelayedAnnotationsElement);
}

void SoDelayedAnnotationsElement::addDelayedPath(SoState* state, SoPath* path)
{
    auto elt = static_cast<SoDelayedAnnotationsElement*>(state->getElementNoPush(classStackIndex));

    elt->paths.append(path);
}

SoPathList SoDelayedAnnotationsElement::getDelayedPaths(SoState* state)
{
    auto elt = static_cast<SoDelayedAnnotationsElement*>(state->getElementNoPush(classStackIndex));
    auto copy = elt->paths;

    elt->paths.truncate(0);

    return copy;
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