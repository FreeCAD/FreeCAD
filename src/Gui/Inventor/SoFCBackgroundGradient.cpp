// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2005 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <array>
#include <cmath>
#include <numbers>

#include <Inventor/SbVec3f.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include "SoFCBackgroundGradient.h"

using namespace Gui;

SO_NODE_SOURCE(SoFCBackgroundGradient)

void SoFCBackgroundGradient::finish()
{
    atexit_cleanup();
}

SoFCBackgroundGradient::SoFCBackgroundGradient()
{
    SO_NODE_CONSTRUCTOR(SoFCBackgroundGradient);
    SO_NODE_ADD_FIELD(gradientMode, (LINEAR));
    SO_NODE_ADD_FIELD(fromColor, (SbColor(0.5f, 0.5f, 0.8f)));
    SO_NODE_ADD_FIELD(toColor, (SbColor(0.7f, 0.7f, 0.9f)));
    SO_NODE_ADD_FIELD(midColor, (SbColor(1.0f, 1.0f, 1.0f)));
    SO_NODE_ADD_FIELD(useMidColor, (TRUE));
    SO_NODE_DEFINE_ENUM_VALUE(Gradient, LINEAR);
    SO_NODE_DEFINE_ENUM_VALUE(Gradient, RADIAL);
    SO_NODE_SET_SF_ENUM_TYPE(gradientMode, Gradient);

    setCoordinateSpace(CoordinateSpace::Normalized);
    setBaseColorLightModel(true);
    setTexturesEnabled(false);
    setMultiTexturesEnabled(false);
    setDepthBuffer(false, false, SoDepthBufferElement::ALWAYS);

    gradientSwitch = new SoSwitch;
    gradientSwitch->whichChild = 0;
    addChild(gradientSwitch);

    // Linear gradient geometry
    linearSeparator = new SoSeparator;
    auto* linearHints = new SoShapeHints;
    linearHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    linearHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    linearSeparator->addChild(linearHints);

    linearVertexProperty = new SoVertexProperty;
    linearVertexProperty->materialBinding = SoVertexProperty::PER_VERTEX;
    linearFaces = new SoFaceSet;
    linearFaces->vertexProperty.setValue(linearVertexProperty);
    linearSeparator->addChild(linearFaces);

    // Radial gradient geometry
    radialSeparator = new SoSeparator;
    auto* radialHints = new SoShapeHints;
    radialHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    radialHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    radialSeparator->addChild(radialHints);

    radialFanVertexProperty = new SoVertexProperty;
    radialFanVertexProperty->materialBinding = SoVertexProperty::PER_VERTEX;
    radialFan = new SoFaceSet;
    radialFan->vertexProperty.setValue(radialFanVertexProperty);
    radialRingVertexProperty = new SoVertexProperty;
    radialRingVertexProperty->materialBinding = SoVertexProperty::PER_VERTEX;
    radialRing = new SoFaceSet;
    radialRing->vertexProperty.setValue(radialRingVertexProperty);
    radialSeparator->addChild(radialFan);
    radialSeparator->addChild(radialRing);

    gradientSwitch->addChild(linearSeparator);
    gradientSwitch->addChild(radialSeparator);
    ensureGeometry();
}

SoFCBackgroundGradient::~SoFCBackgroundGradient()
{
    gradientSwitch = nullptr;
    linearSeparator = nullptr;
    linearFaces = nullptr;
    linearVertexProperty = nullptr;
    radialSeparator = nullptr;
    radialFan = nullptr;
    radialFanVertexProperty = nullptr;
    radialRing = nullptr;
    radialRingVertexProperty = nullptr;
}

void SoFCBackgroundGradient::initClass()
{
    SO_NODE_INIT_CLASS(SoFCBackgroundGradient, SoFCScreenSpaceGroup, "SoFCScreenSpaceGroup");
}

void SoFCBackgroundGradient::doAction(SoAction* action)
{
    ensureGeometry();
    inherited::doAction(action);
}

void SoFCBackgroundGradient::GLRenderBelowPath(SoGLRenderAction* action)
{
    ensureGeometry();
    inherited::GLRenderBelowPath(action);
}

void SoFCBackgroundGradient::GLRenderInPath(SoGLRenderAction* action)
{
    ensureGeometry();
    inherited::GLRenderInPath(action);
}

void SoFCBackgroundGradient::setGradient(SoFCBackgroundGradient::Gradient grad)
{
    gradientMode.setValue(static_cast<int>(grad));
}

SoFCBackgroundGradient::Gradient SoFCBackgroundGradient::getGradient() const
{
    return static_cast<Gradient>(gradientMode.getValue());
}

void SoFCBackgroundGradient::setColorGradient(const SbColor& fromColorValue, const SbColor& toColorValue)
{
    fromColor.setValue(fromColorValue);
    toColor.setValue(toColorValue);
    useMidColor.setValue(FALSE);
}

void SoFCBackgroundGradient::setColorGradient(
    const SbColor& fromColorValue,
    const SbColor& toColorValue,
    const SbColor& midColorValue
)
{
    fromColor.setValue(fromColorValue);
    toColor.setValue(toColorValue);
    midColor.setValue(midColorValue);
    useMidColor.setValue(TRUE);
}

void SoFCBackgroundGradient::ensureGeometry()
{
    const GeometryState state = currentGeometryState();
    if (geometryInitialized && state.gradient == appliedGeometryState.gradient
        && state.fromColor == appliedGeometryState.fromColor
        && state.toColor == appliedGeometryState.toColor
        && state.midColor == appliedGeometryState.midColor
        && state.useMidColor == appliedGeometryState.useMidColor) {
        return;
    }

    geometryInitialized = true;
    appliedGeometryState = state;
    if (gradientSwitch) {
        gradientSwitch->whichChild = (state.gradient == Gradient::LINEAR) ? 0 : 1;
    }

    if (state.gradient == Gradient::LINEAR) {
        updateLinearGeometry(state);
    }
    else {
        updateRadialGeometry(state);
    }
}

SoFCBackgroundGradient::GeometryState SoFCBackgroundGradient::currentGeometryState() const
{
    GeometryState state;
    state.gradient = static_cast<Gradient>(gradientMode.getValue());
    state.fromColor = fromColor.getValue().getPackedValue();
    state.toColor = toColor.getValue().getPackedValue();
    state.midColor = midColor.getValue().getPackedValue();
    state.useMidColor = (useMidColor.getValue() != FALSE);
    return state;
}

void SoFCBackgroundGradient::updateLinearGeometry(const GeometryState& state)
{
    if (!linearVertexProperty || !linearFaces) {
        return;
    }

    if (!state.useMidColor) {
        static const SbVec3f quadVertices[4] = {
            SbVec3f(-1.0f, 1.0f, 0.0f),
            SbVec3f(1.0f, 1.0f, 0.0f),
            SbVec3f(1.0f, -1.0f, 0.0f),
            SbVec3f(-1.0f, -1.0f, 0.0f)
        };
        linearVertexProperty->vertex.setValues(0, 4, quadVertices);
        const uint32_t colors[4] = {state.fromColor, state.fromColor, state.toColor, state.toColor};
        linearVertexProperty->orderedRGBA.setValues(0, 4, colors);
        linearFaces->numVertices.setNum(1);
        linearFaces->numVertices.set1Value(0, 4);
    }
    else {
        static const SbVec3f vertices[8] = {
            SbVec3f(-1.0f, 1.0f, 0.0f),
            SbVec3f(1.0f, 1.0f, 0.0f),
            SbVec3f(1.0f, 0.0f, 0.0f),
            SbVec3f(-1.0f, 0.0f, 0.0f),
            SbVec3f(-1.0f, 0.0f, 0.0f),
            SbVec3f(1.0f, 0.0f, 0.0f),
            SbVec3f(1.0f, -1.0f, 0.0f),
            SbVec3f(-1.0f, -1.0f, 0.0f)
        };
        linearVertexProperty->vertex.setValues(0, 8, vertices);
        const uint32_t colors[8] = {
            state.fromColor,
            state.fromColor,
            state.midColor,
            state.midColor,
            state.midColor,
            state.midColor,
            state.toColor,
            state.toColor
        };
        linearVertexProperty->orderedRGBA.setValues(0, 8, colors);
        linearFaces->numVertices.setNum(2);
        linearFaces->numVertices.set1Value(0, 4);
        linearFaces->numVertices.set1Value(1, 4);
    }
}

void SoFCBackgroundGradient::updateRadialGeometry(const GeometryState& state)
{
    if (!radialFan || !radialFanVertexProperty || !radialRing || !radialRingVertexProperty) {
        return;
    }

    constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
    constexpr float sqrt2 = std::numbers::sqrt2_v<float>;
    const float angleStep = twoPi / CircleSegments;

    std::array<SbVec3f, CircleSegments> outerPoints;
    for (int i = 0; i < CircleSegments; ++i) {
        const float angle = angleStep * i;
        outerPoints[i].setValue(sqrt2 * std::cos(angle), sqrt2 * std::sin(angle), 0.0f);
    }

    std::array<SbVec3f, CircleSegments> innerPoints;
    if (state.useMidColor) {
        const float sqrtHalf = std::sqrt(0.5f);
        for (int i = 0; i < CircleSegments; ++i) {
            const float angle = angleStep * i;
            innerPoints[i].setValue(0.3f * sqrt2 * std::cos(angle), sqrtHalf * std::sin(angle), 0.0f);
        }
    }

    // Build triangle fan (center to inner or outer circle)
    radialFan->numVertices.setNum(CircleSegments);
    const int fanVertexCount = CircleSegments * 3;
    radialFanVertexProperty->vertex.setNum(fanVertexCount);
    radialFanVertexProperty->orderedRGBA.setNum(fanVertexCount);
    for (int i = 0; i < CircleSegments; ++i) {
        const int next = (i + 1) % CircleSegments;
        const int base = i * 3;

        radialFanVertexProperty->vertex.set1Value(base + 0, SbVec3f(0.0f, 0.0f, 0.0f));
        radialFanVertexProperty->orderedRGBA.set1Value(base + 0, state.fromColor);

        const SbVec3f& first = state.useMidColor ? innerPoints[i] : outerPoints[i];
        const SbVec3f& second = state.useMidColor ? innerPoints[next] : outerPoints[next];
        const uint32_t color = state.useMidColor ? state.midColor : state.toColor;

        radialFanVertexProperty->vertex.set1Value(base + 1, first);
        radialFanVertexProperty->orderedRGBA.set1Value(base + 1, color);
        radialFanVertexProperty->vertex.set1Value(base + 2, second);
        radialFanVertexProperty->orderedRGBA.set1Value(base + 2, color);

        radialFan->numVertices.set1Value(i, 3);
    }

    if (state.useMidColor) {
        const int ringTriangles = CircleSegments * 2;
        const int ringVertexCount = ringTriangles * 3;
        radialRing->numVertices.setNum(ringTriangles);
        radialRingVertexProperty->vertex.setNum(ringVertexCount);
        radialRingVertexProperty->orderedRGBA.setNum(ringVertexCount);

        for (int i = 0; i < CircleSegments; ++i) {
            const int next = (i + 1) % CircleSegments;
            const int base = i * 6;

            // Triangle: inner_i, outer_i, outer_next
            radialRingVertexProperty->vertex.set1Value(base + 0, innerPoints[i]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 0, state.midColor);
            radialRingVertexProperty->vertex.set1Value(base + 1, outerPoints[i]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 1, state.toColor);
            radialRingVertexProperty->vertex.set1Value(base + 2, outerPoints[next]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 2, state.toColor);
            radialRing->numVertices.set1Value(i * 2, 3);

            // Triangle: inner_i, outer_next, inner_next
            radialRingVertexProperty->vertex.set1Value(base + 3, innerPoints[i]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 3, state.midColor);
            radialRingVertexProperty->vertex.set1Value(base + 4, outerPoints[next]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 4, state.toColor);
            radialRingVertexProperty->vertex.set1Value(base + 5, innerPoints[next]);
            radialRingVertexProperty->orderedRGBA.set1Value(base + 5, state.midColor);
            radialRing->numVertices.set1Value(i * 2 + 1, 3);
        }
    }
    else {
        radialRing->numVertices.setNum(0);
        radialRingVertexProperty->vertex.setNum(0);
        radialRingVertexProperty->orderedRGBA.setNum(0);
    }
}
