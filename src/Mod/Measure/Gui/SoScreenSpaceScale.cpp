// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983[at]gmail.com>          *
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

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include "SoScreenSpaceScale.h"

using namespace MeasureGui;

SO_NODE_SOURCE(SoScreenSpaceScale)

void SoScreenSpaceScale::initClass()
{
    SO_NODE_INIT_CLASS(SoScreenSpaceScale, SoTransformation, "Transformation");
}

float SoScreenSpaceScale::calculateScaleFactor(SoAction* action)
{
    // reference: SoDatumLabel::getScaleFactor
    // fallback to origin if focal distance element is not available

    SoState* state = action->getState();
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    SbVec2s vp_size = vp.getViewportSizePixels();

    SbVec3f referencePoint(0.f, 0.f, 0.f);

    if (state->isElementEnabled(SoFocalDistanceElement::getClassStackIndex())) {
        float focal = SoFocalDistanceElement::get(state);
        referencePoint = vv.getSightPoint(focal);
    }

    float scale = vv.getWorldToScreenScale(referencePoint, 1.0f);
    scale /= float(vp_size[0]);

    float newScale = scale * referenceSize.getValue();
    if (finalScale.getValue() != newScale) {
        finalScale.setValue(newScale);
    }
    return newScale;
}

SoScreenSpaceScale::SoScreenSpaceScale()
{
    SO_NODE_CONSTRUCTOR(SoScreenSpaceScale);
    SO_NODE_ADD_FIELD(referenceSize, (1.0f));
    SO_NODE_ADD_FIELD(finalScale, (1.0f));
}

void SoScreenSpaceScale::GLRender(SoGLRenderAction* action)
{
    SoScreenSpaceScale::doAction((SoAction*)action);
}

void SoScreenSpaceScale::doAction(SoAction* action)
{
    float sf = 1.0f;
    if (referenceSize.getValue() != 0.0f) {
        sf = this->calculateScaleFactor(action);
    }

    if (sf != 1.0f) {
        SoModelMatrixElement::scaleBy(action->getState(), this, SbVec3f(sf, sf, sf));
    }
}

void SoScreenSpaceScale::callback(SoCallbackAction* action)
{
    SoScreenSpaceScale::doAction((SoAction*)action);
}

void SoScreenSpaceScale::getBoundingBox(SoGetBoundingBoxAction* action)
{
    SoScreenSpaceScale::doAction((SoAction*)action);
}

void SoScreenSpaceScale::pick(SoPickAction* action)
{
    SoScreenSpaceScale::doAction((SoAction*)action);
}

void SoScreenSpaceScale::getPrimitiveCount(SoGetPrimitiveCountAction* action)
{
    SoScreenSpaceScale::doAction((SoAction*)action);
}
