// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#include <algorithm>
#include <limits>
#include <vector>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoIRRenderAction.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>

#include "SoFCShapeObject.h"


using namespace PartGui;

SO_NODE_SOURCE(SoFCShape);

SoFCShape::SoFCShape()
    : coords(new SoCoordinate3)
    , norm(new SoNormal)
    , faceset(new SoBrepFaceSet)
    , lineset(new SoBrepEdgeSet)
    , nodeset(new SoBrepPointSet)
{
    SO_NODE_CONSTRUCTOR(SoFCShape);
}

void SoFCShape::initClass()
{
    SO_NODE_INIT_CLASS(SoFCShape, SoSeparator, "Separator");
}

SO_NODE_SOURCE(SoFCControlPoints)

void SoFCControlPoints::initClass()
{
    SO_NODE_INIT_CLASS(SoFCControlPoints, SoShape, "Shape");
}

SoFCControlPoints::SoFCControlPoints()
{
    SO_NODE_CONSTRUCTOR(SoFCControlPoints);

    SbVec3f c(1.0f, 0.447059f, 0.337255f);
    SO_NODE_ADD_FIELD(numPolesU, (0));
    SO_NODE_ADD_FIELD(numPolesV, (0));
    SO_NODE_ADD_FIELD(numKnotsU, (0));
    SO_NODE_ADD_FIELD(numKnotsV, (0));
    SO_NODE_ADD_FIELD(lineColor, (c));

    renderRoot = new SoSeparator;
    renderRoot->ref();

    auto* pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::UNPICKABLE;
    renderRoot->addChild(pickStyle);

    renderCoordinates = new SoCoordinate3;
    renderRoot->addChild(renderCoordinates);

    auto* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    renderRoot->addChild(lightModel);

    auto* meshRoot = new SoSeparator;
    meshBaseColor = new SoBaseColor;
    meshRoot->addChild(meshBaseColor);
    auto* meshDrawStyle = new SoDrawStyle;
    meshDrawStyle->lineWidth = 1.0f;
    meshRoot->addChild(meshDrawStyle);
    meshLineSet = new SoIndexedLineSet;
    meshRoot->addChild(meshLineSet);
    renderRoot->addChild(meshRoot);

    auto* polesRoot = new SoSeparator;
    polesBaseColor = new SoBaseColor;
    polesRoot->addChild(polesBaseColor);
    auto* polesDrawStyle = new SoDrawStyle;
    polesDrawStyle->pointSize = 5.0f;
    polesRoot->addChild(polesDrawStyle);
    polesPointSet = new SoPointSet;
    polesRoot->addChild(polesPointSet);
    renderRoot->addChild(polesRoot);

    auto* knotsRoot = new SoSeparator;
    auto* knotsBaseColor = new SoBaseColor;
    knotsBaseColor->rgb.setValue(1.0f, 1.0f, 0.0f);
    knotsRoot->addChild(knotsBaseColor);
    auto* knotsDrawStyle = new SoDrawStyle;
    knotsDrawStyle->pointSize = 6.0f;
    knotsRoot->addChild(knotsDrawStyle);
    knotsPointSet = new SoPointSet;
    knotsRoot->addChild(knotsPointSet);
    renderRoot->addChild(knotsRoot);
}

void SoFCControlPoints::clearRenderGeometry()
{
    if (renderCoordinates) {
        if (renderCoordinates->point.getNum() != 0) {
            renderCoordinates->point.setNum(0);
        }
    }
    if (meshLineSet) {
        if (meshLineSet->coordIndex.getNum() != 0) {
            meshLineSet->coordIndex.setNum(0);
        }
    }
    if (polesPointSet) {
        if (polesPointSet->startIndex.getValue() != 0) {
            polesPointSet->startIndex.setValue(0);
        }
        if (polesPointSet->numPoints.getValue() != 0) {
            polesPointSet->numPoints.setValue(0);
        }
    }
    if (knotsPointSet) {
        if (knotsPointSet->startIndex.getValue() != 0) {
            knotsPointSet->startIndex.setValue(0);
        }
        if (knotsPointSet->numPoints.getValue() != 0) {
            knotsPointSet->numPoints.setValue(0);
        }
    }
}

bool SoFCControlPoints::syncRenderGeometry(SoState* state)
{
    if (!state || !renderCoordinates || !meshBaseColor || !meshLineSet || !polesBaseColor
        || !polesPointSet || !knotsPointSet) {
        clearRenderGeometry();
        return false;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
    if (!coords) {
        clearRenderGeometry();
        return false;
    }

    const int32_t len = coords->getNum();
    const uint32_t nCtU = numPolesU.getValue();
    const uint32_t nCtV = numPolesV.getValue();
    const uint32_t poles = nCtU * nCtV;
    if (poles > static_cast<uint32_t>(len)) {
        clearRenderGeometry();
        return false;  // wrong setup, too few points
    }

    const uint32_t knots = numKnotsU.getValue() * numKnotsV.getValue();
    const uint32_t total = poles + knots;
    if (total > static_cast<uint32_t>(len)) {
        clearRenderGeometry();
        return false;  // wrong setup, too few points
    }

    // The control-point node is driven by a sibling coordinate node in the
    // parent scene. Copy only changed active coordinates into the private
    // retained graph so every renderer sees the same geometry.
    updateMeshCoordIndex(nCtU, nCtV);
    bool coordinatesChanged = renderCoordinates->point.getNum() != len;
    if (!coordinatesChanged) {
        const SbVec3f* retainedPoints = renderCoordinates->point.getValues(0);
        for (int32_t i = 0; i < len; ++i) {
            if (retainedPoints[i] != coords->get3(i)) {
                coordinatesChanged = true;
                break;
            }
        }
    }
    if (coordinatesChanged) {
        std::vector<SbVec3f> points;
        points.reserve(static_cast<size_t>(len));
        for (int32_t i = 0; i < len; ++i) {
            points.push_back(coords->get3(i));
        }
        renderCoordinates->point.setNum(len);
        renderCoordinates->point.setValues(0, len, points.data());
    }

    const SbColor poleColor = lineColor.getValue();
    if (meshBaseColor->rgb.getNum() != 1 || meshBaseColor->rgb.getValues(0)[0] != poleColor) {
        meshBaseColor->rgb.setValue(poleColor);
    }
    if (polesBaseColor->rgb.getNum() != 1 || polesBaseColor->rgb.getValues(0)[0] != poleColor) {
        polesBaseColor->rgb.setValue(poleColor);
    }

    const int meshIndexCount = static_cast<int>(meshCoordIndex.size());
    bool meshIndicesChanged = meshLineSet->coordIndex.getNum() != meshIndexCount;
    if (!meshIndicesChanged && meshIndexCount > 0) {
        const int32_t* retainedIndices = meshLineSet->coordIndex.getValues(0);
        meshIndicesChanged = !std::equal(meshCoordIndex.begin(), meshCoordIndex.end(), retainedIndices);
    }
    if (meshIndicesChanged && meshCoordIndex.empty()) {
        meshLineSet->coordIndex.setNum(0);
    }
    else if (meshIndicesChanged) {
        meshLineSet->coordIndex.setNum(meshIndexCount);
        meshLineSet->coordIndex.setValues(0, meshIndexCount, meshCoordIndex.data());
    }
    if (polesPointSet->startIndex.getValue() != 0) {
        polesPointSet->startIndex.setValue(0);
    }
    if (polesPointSet->numPoints.getValue() != static_cast<int32_t>(poles)) {
        polesPointSet->numPoints.setValue(static_cast<int32_t>(poles));
    }
    if (knotsPointSet->startIndex.getValue() != static_cast<int32_t>(poles)) {
        knotsPointSet->startIndex.setValue(static_cast<int32_t>(poles));
    }
    if (knotsPointSet->numPoints.getValue() != static_cast<int32_t>(knots)) {
        knotsPointSet->numPoints.setValue(static_cast<int32_t>(knots));
    }

    return true;
}

void SoFCControlPoints::GLRender(SoGLRenderAction* action)
{
    if (!action || !renderRoot || !shouldGLRender(action)) {
        return;
    }

    if (!syncRenderGeometry(action->getState())) {
        return;
    }

    renderRoot->GLRender(action);
}

void SoFCControlPoints::render(SoIRRenderAction* action)
{
    if (!action || !renderRoot || !syncRenderGeometry(action->getState())) {
        return;
    }

    renderRoot->doAction(action);
}

void SoFCControlPoints::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    if (!action) {
        return;
    }

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    if (!coords) {
        return;
    }

    const int32_t len = coords->getNum();
    if (len <= 0) {
        box.setBounds(SbVec3f(0.0f, 0.0f, 0.0f), SbVec3f(0.0f, 0.0f, 0.0f));
        center.setValue(0.0f, 0.0f, 0.0f);
        return;
    }

    constexpr float floatMax = std::numeric_limits<float>::max();
    SbVec3f minimum(floatMax, floatMax, floatMax);
    SbVec3f maximum(-floatMax, -floatMax, -floatMax);
    for (int32_t i = 0; i < len; ++i) {
        const SbVec3f& point = coords->get3(i);
        for (int axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::min(minimum[axis], point[axis]);
            maximum[axis] = std::max(maximum[axis], point[axis]);
        }
    }

    box.setBounds(minimum, maximum);
    center = (minimum + maximum) * 0.5f;
}

void SoFCControlPoints::generatePrimitives(SoAction*)
{}

void SoFCControlPoints::doAction(SoAction* action)
{
    if (action && action->getTypeId().isDerivedFrom(SoIRRenderAction::getClassTypeId())) {
        if (renderRoot && syncRenderGeometry(action->getState())) {
            renderRoot->doAction(action);
        }
        return;
    }

    inherited::doAction(action);
}

SoFCControlPoints::~SoFCControlPoints()
{
    if (renderRoot) {
        renderRoot->unref();
        renderRoot = nullptr;
    }
}

void SoFCControlPoints::updateMeshCoordIndex(uint32_t nCtU, uint32_t nCtV)
{
    if (nCtU == cachedNumPolesU && nCtV == cachedNumPolesV) {
        return;
    }

    cachedNumPolesU = nCtU;
    cachedNumPolesV = nCtV;
    meshCoordIndex.clear();

    if (nCtU < 2 || nCtV < 2) {
        return;
    }

    // Polylines along V for each U.
    for (uint32_t u = 0; u < nCtU; ++u) {
        for (uint32_t v = 0; v < nCtV; ++v) {
            meshCoordIndex.push_back(static_cast<int32_t>(u * nCtV + v));
        }
        meshCoordIndex.push_back(-1);
    }

    // Polylines along U for each V.
    for (uint32_t v = 0; v < nCtV; ++v) {
        for (uint32_t u = 0; u < nCtU; ++u) {
            meshCoordIndex.push_back(static_cast<int32_t>(u * nCtV + v));
        }
        meshCoordIndex.push_back(-1);
    }
}
