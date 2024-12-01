// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#endif

#include "SoFCTransform.h"


using namespace Gui;

SO_NODE_SOURCE(SoFCTransform)

void SoFCTransform::initClass()
{
    SO_NODE_INIT_CLASS(SoFCTransform, SoTransform, "Transform");
}

SoFCTransform::SoFCTransform()
{
    SO_NODE_CONSTRUCTOR(SoFCTransform);
}

void SoFCTransform::GLRender(SoGLRenderAction * action)
{
    SoFCTransform::doAction(action);
}

void SoFCTransform::callback(SoCallbackAction * action)
{
    SoFCTransform::doAction(action);
}

void SoFCTransform::pick(SoPickAction * action)
{
    SoFCTransform::doAction(action);
}

void SoFCTransform::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
    SoFCTransform::doAction(action);
}

void SoFCTransform::getBoundingBox(SoGetBoundingBoxAction * action)
{
    SoFCTransform::doAction(action);
}

void SoFCTransform::doAction(SoAction * action)
{
    SbMatrix matrix;
    matrix.setTransform(this->translation.getValue(),
                        this->rotation.getValue(),
                        this->scaleFactor.getValue(),
                        this->scaleOrientation.getValue(),
                        this->center.getValue());

    // This is different to SoTransform::doAction() where model matrix
    // is always set
    if (matrix != SbMatrix::identity()) {
        SoModelMatrixElement::mult(action->getState(), this, matrix);
    }
}
