// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCConfig.h>

#include <algorithm>
#include <limits>
#include <vector>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoIRRenderAction.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>

#include "SoPolygon.h"


using namespace MeshGui;

SO_NODE_SOURCE(SoPolygon)

namespace
{

bool getPolygonRange(
    const SoCoordinateElement* coords,
    int32_t startIndex,
    int32_t numVertices,
    int32_t& begin,
    int32_t& end
)
{
    if (!coords || startIndex < 0 || numVertices <= 0) {
        return false;
    }

    const int32_t coordinateCount = coords->getNum();
    if (startIndex > coordinateCount || numVertices > coordinateCount - startIndex) {
        return false;
    }

    begin = startIndex;
    end = startIndex + numVertices;
    return true;
}

}  // namespace

void SoPolygon::initClass()
{
    SO_NODE_INIT_CLASS(SoPolygon, SoShape, "Shape");
    SoIRRenderAction::addMethod(SoPolygon::getClassTypeId(), SoPolygon::IRRender);
}

SoPolygon::SoPolygon()
{
    SO_NODE_CONSTRUCTOR(SoPolygon);

    SO_NODE_ADD_FIELD(startIndex, (0));
    SO_NODE_ADD_FIELD(numVertices, (0));
    SO_NODE_ADD_FIELD(highlight, (false));
    SO_NODE_ADD_FIELD(render, (true));

    renderRoot = new SoSeparator;
    renderRoot->ref();

    renderCoordinates = new SoCoordinate3;
    renderRoot->addChild(renderCoordinates);

    auto* renderLightModel = new SoLightModel;
    renderLightModel->model = SoLightModel::BASE_COLOR;
    renderRoot->addChild(renderLightModel);

    renderMaterial = new SoMaterial;
    renderRoot->addChild(renderMaterial);

    auto* renderDrawStyle = new SoDrawStyle;
    renderDrawStyle->lineWidth = 3.0f;
    renderRoot->addChild(renderDrawStyle);

    renderLineSet = new SoIndexedLineSet;
    renderRoot->addChild(renderLineSet);
}

SoPolygon::~SoPolygon()
{
    if (renderRoot) {
        renderRoot->unref();
        renderRoot = nullptr;
    }
}

void SoPolygon::clearRenderGeometry()
{
    if (renderCoordinates) {
        if (renderCoordinates->point.getNum() != 0) {
            renderCoordinates->point.setNum(0);
        }
    }
    if (renderLineSet) {
        if (renderLineSet->coordIndex.getNum() != 0) {
            renderLineSet->coordIndex.setNum(0);
        }
    }
}

bool SoPolygon::syncRenderGeometry(SoState* state)
{
    if (!state || !renderCoordinates || !renderMaterial || !renderLineSet) {
        clearRenderGeometry();
        return false;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
    if (!coords) {
        clearRenderGeometry();
        return false;
    }

    int32_t begin = 0;
    int32_t end = 0;
    if (!getPolygonRange(coords, startIndex.getValue(), numVertices.getValue(), begin, end)) {
        clearRenderGeometry();
        return false;
    }

    const int32_t count = end - begin;
    if (count < 2) {
        clearRenderGeometry();
        return false;
    }

    bool coordinatesChanged = renderCoordinates->point.getNum() != count;
    if (!coordinatesChanged) {
        const SbVec3f* retainedPoints = renderCoordinates->point.getValues(0);
        for (int32_t i = 0; i < count; ++i) {
            if (retainedPoints[i] != coords->get3(begin + i)) {
                coordinatesChanged = true;
                break;
            }
        }
    }
    if (coordinatesChanged) {
        std::vector<SbVec3f> polygonPoints;
        polygonPoints.reserve(static_cast<size_t>(count));
        for (int32_t i = begin; i < end; ++i) {
            polygonPoints.push_back(coords->get3(i));
        }
        renderCoordinates->point.setNum(count);
        renderCoordinates->point.setValues(0, count, polygonPoints.data());
    }

    updateLineIndices(count);
    const int indexCount = static_cast<int>(lineIndices.size());
    bool indicesChanged = renderLineSet->coordIndex.getNum() != indexCount;
    if (!indicesChanged) {
        const int32_t* retainedIndices = renderLineSet->coordIndex.getValues(0);
        indicesChanged = !std::equal(lineIndices.begin(), lineIndices.end(), retainedIndices);
    }
    if (indicesChanged) {
        renderLineSet->coordIndex.setNum(indexCount);
        renderLineSet->coordIndex.setValues(0, indexCount, lineIndices.data());
    }

    const SbColor diffuse = SoLazyElement::getDiffuse(state, 0);
    if (renderMaterial->diffuseColor.getNum() != 1
        || renderMaterial->diffuseColor.getValues(0)[0] != diffuse) {
        renderMaterial->diffuseColor.setValue(diffuse);
    }
    const float transparency = SoLazyElement::getTransparency(state, 0);
    if (renderMaterial->transparency.getNum() != 1
        || renderMaterial->transparency.getValues(0)[0] != transparency) {
        renderMaterial->transparency.setValue(transparency);
    }

    return true;
}

/**
 * Renders the polygon.
 */
void SoPolygon::GLRender(SoGLRenderAction* action)
{
    if (!action || !render.getValue() || !shouldGLRender(action)) {
        return;
    }

    if (!syncRenderGeometry(action->getState())) {
        return;
    }

    renderRoot->GLRender(action);
}

void SoPolygon::updateLineIndices(int32_t vertexCount)
{
    if (vertexCount == cachedVertexCount) {
        return;
    }

    cachedVertexCount = vertexCount;
    lineIndices.clear();
    lineIndices.reserve(static_cast<size_t>(vertexCount) * 3U);
    for (int32_t i = 0; i < vertexCount; ++i) {
        lineIndices.push_back(i);
        lineIndices.push_back((i + 1) % vertexCount);
        lineIndices.push_back(-1);
    }
}

void SoPolygon::IRRender(SoAction* action, SoNode* node)
{
    auto* polygon = static_cast<SoPolygon*>(node);
    if (!polygon) {
        return;
    }

    polygon->renderAction(static_cast<SoIRRenderAction*>(action));
}

void SoPolygon::renderAction(SoIRRenderAction* action)
{
    if (!action || !render.getValue()) {
        return;
    }

    if (!syncRenderGeometry(action->getState())) {
        return;
    }

    renderRoot->doAction(action);
}

/**
 * Calculates picked point based on primitives generated by subclasses.
 */
void SoPolygon::rayPick(SoRayPickAction* action)
{
    // if (this->shouldRayPick(action)) {
    //     this->computeObjectSpaceRay(action);

    //    const SoBoundingBoxCache* bboxcache = getBoundingBoxCache();
    //    if (!bboxcache || !bboxcache->isValid(action->getState()) ||
    //        SoFCMeshObjectShape_ray_intersect(action, bboxcache->getProjectedBox())) {
    //        this->generatePrimitives(action);
    //    }
    //}
    inherited::rayPick(action);
}

void SoPolygon::generatePrimitives(SoAction* /*action*/)
{}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoPolygon::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    SoState* state = action->getState();
    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
    if (!coords) {
        return;
    }
    constexpr float floatMax = std::numeric_limits<float>::max();
    float maxX = -floatMax, minX = floatMax, maxY = -floatMax, minY = floatMax, maxZ = -floatMax,
          minZ = floatMax;
    int32_t begin = 0;
    int32_t end = 0;
    if (getPolygonRange(coords, startIndex.getValue(), numVertices.getValue(), begin, end)) {
        for (int32_t i = begin; i < end; i++) {
            const SbVec3f& point = coords->get3(i);
            maxX = std::max<float>(maxX, point[0]);
            minX = std::min<float>(minX, point[0]);
            maxY = std::max<float>(maxY, point[1]);
            minY = std::min<float>(minY, point[1]);
            maxZ = std::max<float>(maxZ, point[2]);
            minZ = std::min<float>(minZ, point[2]);
        }

        box.setBounds(minX, minY, minZ, maxX, maxY, maxZ);
        center.setValue(0.5F * (minX + maxX), 0.5F * (minY + maxY), 0.5F * (minZ + maxZ));
    }
    else {
        box.setBounds(SbVec3f(0, 0, 0), SbVec3f(0, 0, 0));
        center.setValue(0.0F, 0.0F, 0.0F);
    }
}
