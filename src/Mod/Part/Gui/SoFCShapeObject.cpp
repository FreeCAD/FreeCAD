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
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/misc/SoState.h>

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

    meshLineSet = new SoIndexedLineSet;
    meshLineSet->ref();
    polesPointSet = new SoPointSet;
    polesPointSet->ref();
    knotsPointSet = new SoPointSet;
    knotsPointSet->ref();
}

/**
 * Renders the control points.
 */
void SoFCControlPoints::GLRender(SoGLRenderAction* action)
{
    if (shouldGLRender(action)) {
        SoState* state = action->getState();
        const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
        if (!coords) {
            return;
        }

        SoMaterialBundle mb(action);
        SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
        mb.sendFirst();  // make sure we have the correct material

        const int32_t len = coords->getNum();

        const uint32_t nCtU = numPolesU.getValue();
        const uint32_t nCtV = numPolesV.getValue();
        const uint32_t poles = nCtU * nCtV;
        if (poles > static_cast<uint32_t>(len)) {
            return;  // wrong setup, too few points
        }

        const uint32_t knots = numKnotsU.getValue() * numKnotsV.getValue();
        const uint32_t total = poles + knots;
        if (total > static_cast<uint32_t>(len)) {
            return;  // wrong setup, too few points
        }

        updateMeshCoordIndex(nCtU, nCtV);

        const SbColor poleColor = lineColor.getValue();
        const SbColor knotColor(1.0f, 1.0f, 0.0f);

        // Draw control mesh.
        state->push();
        SoLineWidthElement::set(state, 1.0f);
        SoLazyElement::setEmissive(state, &poleColor);
        uint32_t packed = poleColor.getPackedValue(0.0f);
        SoLazyElement::setPacked(state, meshLineSet, 1, &packed, false);
        meshLineSet->coordIndex
            .setValues(0, static_cast<int32_t>(meshCoordIndex.size()), meshCoordIndex.data());
        meshLineSet->GLRender(action);
        state->pop();

        // Draw poles.
        state->push();
        SoPointSizeElement::set(state, 5.0f);
        SoLazyElement::setEmissive(state, &poleColor);
        packed = poleColor.getPackedValue(0.0f);
        SoLazyElement::setPacked(state, polesPointSet, 1, &packed, false);
        polesPointSet->startIndex.setValue(0);
        polesPointSet->numPoints.setValue(static_cast<int32_t>(poles));
        polesPointSet->GLRender(action);
        state->pop();

        // Draw knots, if available.
        if (knots > 0) {
            state->push();
            SoPointSizeElement::set(state, 6.0f);
            SoLazyElement::setEmissive(state, &knotColor);
            packed = knotColor.getPackedValue(0.0f);
            SoLazyElement::setPacked(state, knotsPointSet, 1, &packed, false);
            knotsPointSet->startIndex.setValue(static_cast<int32_t>(poles));
            knotsPointSet->numPoints.setValue(static_cast<int32_t>(knots));
            knotsPointSet->GLRender(action);
            state->pop();
        }
    }
}

SoFCControlPoints::~SoFCControlPoints()
{
    if (meshLineSet) {
        meshLineSet->unref();
        meshLineSet = nullptr;
    }
    if (polesPointSet) {
        polesPointSet->unref();
        polesPointSet = nullptr;
    }
    if (knotsPointSet) {
        knotsPointSet->unref();
        knotsPointSet = nullptr;
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

void SoFCControlPoints::generatePrimitives(SoAction* /*action*/)
{}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoFCControlPoints::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    SoState* state = action->getState();
    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
    if (!coords) {
        return;
    }
    const SbVec3f* points = coords->getArrayPtr3();
    if (!points) {
        return;
    }
    constexpr float floatMax = std::numeric_limits<float>::max();
    float maxX = -floatMax, minX = floatMax, maxY = -floatMax, minY = floatMax, maxZ = -floatMax,
          minZ = floatMax;
    int32_t len = coords->getNum();
    if (len > 0) {
        for (int32_t i = 0; i < len; i++) {
            maxX = std::max<float>(maxX, points[i][0]);
            minX = std::min<float>(minX, points[i][0]);
            maxY = std::max<float>(maxY, points[i][1]);
            minY = std::min<float>(minY, points[i][1]);
            maxZ = std::max<float>(maxZ, points[i][2]);
            minZ = std::min<float>(minZ, points[i][2]);
        }

        box.setBounds(minX, minY, minZ, maxX, maxY, maxZ);
        center.setValue(0.5f * (minX + maxX), 0.5f * (minY + maxY), 0.5f * (minZ + maxZ));
    }
    else {
        box.setBounds(SbVec3f(0, 0, 0), SbVec3f(0, 0, 0));
        center.setValue(0.0f, 0.0f, 0.0f);
    }
}
