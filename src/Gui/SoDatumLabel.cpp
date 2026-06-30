// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011-2012 Luke Parry <l.parry@warwick.ac.uk>
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

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <numbers>
#include <QFontMetrics>
#include <QPainter>

#include <Inventor/SbRotation.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoVertexProperty.h>

#include <Base/Tools.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Tools.h>

#include "SoDatumLabel.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
constexpr const float ZCONSTR {0.006F};
// Z offset for arrowheads and text to render them ON TOP of geometry lines.
// Geometry lines are at Z ~0.005-0.008, so this offset ensures arrowheads
// and text selection primitives are above them and remain selectable.
constexpr const float ZARROW_TEXT_OFFSET {0.010F};

using namespace Gui;

// ------------------------------------------------------


namespace
{

SbVec3f withZ(const SbVec3f& point, float z)
{
    SbVec3f out = point;
    out[2] = z;
    return out;
}

void appendLine(
    std::vector<SbVec3f>& vertices,
    std::vector<int32_t>& counts,
    const SbVec3f& a,
    const SbVec3f& b
)
{
    vertices.push_back(a);
    vertices.push_back(b);
    counts.push_back(2);
}

void appendArc(
    std::vector<SbVec3f>& vertices,
    std::vector<int32_t>& counts,
    const SbVec3f& center,
    float radius,
    float startAngle,
    float endAngle,
    int countSegments = 0
)
{
    const float range = endAngle - startAngle;

    if (countSegments == 0) {
        countSegments = std::max(6, abs(int(25.0 * range / std::numbers::pi)));
    }

    const float segment = range / (countSegments - 1);

    for (int i = 0; i < countSegments; i++) {
        const float theta = startAngle + segment * i;
        const SbVec3f v = center + radius * SbVec3f(cos(theta), sin(theta), 0);
        vertices.push_back(v);
    }
    counts.push_back(countSegments);
}

void appendArrowTriangle(
    std::vector<SbVec3f>& vertices,
    std::vector<int32_t>& counts,
    const SbVec3f& base,
    const SbVec3f& dir,
    float width,
    float length
)
{
    SbVec3f unitDir = dir;
    unitDir.normalize();

    const SbVec3f normal(unitDir[1], -unitDir[0], 0);
    const SbVec3f arrowLeft = base - length * unitDir + width * normal;
    const SbVec3f arrowRight = base - length * unitDir - width * normal;

    vertices.push_back(withZ(base, ZARROW_TEXT_OFFSET));
    vertices.push_back(withZ(arrowLeft, ZARROW_TEXT_OFFSET));
    vertices.push_back(withZ(arrowRight, ZARROW_TEXT_OFFSET));
    counts.push_back(3);
}


float normalizeArcSweepEnd(float startAngle, float endAngle)
{
    constexpr float tau = 2.0F * std::numbers::pi_v<float>;
    const float delta = endAngle - startAngle;

    if (delta >= 0.0F) {
        return endAngle;
    }

    return endAngle + tau * std::ceil(-delta / tau);
}

SbVec3f getArcMidDirection(float startAngle, float endAngle)
{
    endAngle = normalizeArcSweepEnd(startAngle, endAngle);
    const float midAngle = startAngle + 0.5F * (endAngle - startAngle);
    return SbVec3f(cos(midAngle), sin(midAngle), 0);
}

SbVec3f getArcTextCenter(const SbVec3f& center, float startAngle, float endAngle, float distanceFromCenter)
{
    return center + getArcMidDirection(startAngle, endAngle) * distanceFromCenter;
}

float getSketchRotationAngle(SoState* state, const SbViewVolume& viewVolume, bool flip)
{
    SbVec3f camRight = viewVolume.lrf - viewVolume.llf;
    SbVec3f camUp = viewVolume.ulf - viewVolume.llf;

    camRight.normalize();
    camUp.normalize();

    const SbMatrix& matrix = SoModelMatrixElement::get(state);
    SbVec3f xWorld;
    matrix.multDirMatrix(SbVec3f(1.0f, 0.0f, 0.0f), xWorld);

    const float cosAngle = xWorld.dot(camRight);
    const float sinAngle = xWorld.dot(camUp);
    const float angle = std::atan2(sinAngle, cosAngle);

    return flip ? angle : -angle;
}

SbVec3f getAngleMidDirection(float startAngle, float range)
{
    const float midAngle = startAngle + 0.5F * range;
    return SbVec3f(cos(midAngle), sin(midAngle), 0);
}

SbVec3f getAngleTextCenter(const SbVec3f& center, float startAngle, float range, float distanceFromCenter)
{
    return center + getAngleMidDirection(startAngle, range) * distanceFromCenter;
}
}  // namespace


SO_NODE_SOURCE(SoDatumLabel)

void SoDatumLabel::initClass()
{
    SO_NODE_INIT_CLASS(SoDatumLabel, SoShape, "Shape");
}

// NOLINTNEXTLINE
SoDatumLabel::SoDatumLabel()
{
    SO_NODE_CONSTRUCTOR(SoDatumLabel);
    SO_NODE_ADD_FIELD(string, (""));
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0F, 1.0F, 1.0F)));
    SO_NODE_ADD_FIELD(pnts, (SbVec3f(.0F, .0F, .0F)));
    SO_NODE_ADD_FIELD(norm, (SbVec3f(.0F, .0F, 1.F)));
    SO_NODE_ADD_FIELD(strikethrough, (false));

    SO_NODE_ADD_FIELD(name, ("osifont"));
    SO_NODE_ADD_FIELD(size, (10.F));
    SO_NODE_ADD_FIELD(lineWidth, (2.F));
    SO_NODE_ADD_FIELD(linePattern, (0b1111111111111111));
    SO_NODE_ADD_FIELD(sampling, (2.F));

    SO_NODE_ADD_FIELD(datumtype, (SoDatumLabel::DISTANCE));

    SO_NODE_DEFINE_ENUM_VALUE(Type, DISTANCE);
    SO_NODE_DEFINE_ENUM_VALUE(Type, DISTANCEX);
    SO_NODE_DEFINE_ENUM_VALUE(Type, DISTANCEY);
    SO_NODE_DEFINE_ENUM_VALUE(Type, ANGLE);
    SO_NODE_DEFINE_ENUM_VALUE(Type, RADIUS);
    SO_NODE_DEFINE_ENUM_VALUE(Type, DIAMETER);
    SO_NODE_DEFINE_ENUM_VALUE(Type, ARCLENGTH);
    SO_NODE_SET_SF_ENUM_TYPE(datumtype, Type);

    SO_NODE_ADD_FIELD(param1, (0.F));
    SO_NODE_ADD_FIELD(param2, (0.F));
    SO_NODE_ADD_FIELD(param4, (0.F));
    SO_NODE_ADD_FIELD(param5, (0.F));
    SO_NODE_ADD_FIELD(param6, (0.F));
    SO_NODE_ADD_FIELD(param7, (0.F));
    SO_NODE_ADD_FIELD(param8, (0.F));

    useAntialiasing = true;

    this->imgWidth = 0;
    this->imgHeight = 0;
    this->glimagevalid = false;

    m_Root = new SoSeparator;
    m_Root->ref();

    m_GeometryDepth = new SoDepthBuffer;
    m_GeometryDepth->test.setValue(true);
    m_GeometryDepth->write.setValue(false);
    m_Root->addChild(m_GeometryDepth);

    m_LightModel = new SoLightModel;
    m_LightModel->model.setValue(SoLightModel::BASE_COLOR);
    m_Root->addChild(m_LightModel);

    m_GeometryColor = new SoBaseColor;
    m_GeometryColor->rgb.connectFrom(&this->textColor);
    m_Root->addChild(m_GeometryColor);

    m_DrawStyle = new SoDrawStyle;
    m_DrawStyle->linePattern.connectFrom(&this->linePattern);
    m_DrawStyle->lineWidth.connectFrom(&this->lineWidth);
    m_Root->addChild(m_DrawStyle);

    // Keep the label two-sided even when inherited rendering state enables face culling.
    auto* hints = new SoShapeHints;
    hints->vertexOrdering.setValue(SoShapeHints::UNKNOWN_ORDERING);
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->faceType.setValue(SoShapeHints::UNKNOWN_FACE_TYPE);
    m_Root->addChild(hints);

    m_LineVertexProperty = new SoVertexProperty;
    m_LineSet = new SoLineSet;
    m_LineSet->vertexProperty.setValue(m_LineVertexProperty);
    m_Root->addChild(m_LineSet);

    m_TriangleVertexProperty = new SoVertexProperty;
    m_TriangleFaceSet = new SoFaceSet;
    m_TriangleFaceSet->vertexProperty.setValue(m_TriangleVertexProperty);
    m_Root->addChild(m_TriangleFaceSet);

    m_TextSwitch = new SoSwitch;
    m_TextSwitch->whichChild.setValue(SO_SWITCH_NONE);
    m_Root->addChild(m_TextSwitch);

    m_TextSeparator = new SoSeparator;
    m_TextSwitch->addChild(m_TextSeparator);

    m_TextDepth = new SoDepthBuffer;
    m_TextDepth->test.setValue(false);
    m_TextDepth->write.setValue(false);
    m_TextDepth->function.setValue(SoDepthBuffer::ALWAYS);
    m_TextSeparator->addChild(m_TextDepth);

    auto* textLightModel = new SoLightModel;
    textLightModel->model.setValue(SoLightModel::BASE_COLOR);
    m_TextSeparator->addChild(textLightModel);

    m_TextBaseColor = new SoBaseColor;
    m_TextBaseColor->rgb.setValue(1.0f, 1.0f, 1.0f);
    m_TextSeparator->addChild(m_TextBaseColor);

    m_TextTexture = new SoTexture2;
    m_TextTexture->wrapS = SoTexture2::CLAMP;
    m_TextTexture->wrapT = SoTexture2::CLAMP;
    m_TextTexture->model = SoTexture2::MODULATE;
    m_TextTexture->image.connectFrom(&this->image);
    m_TextSeparator->addChild(m_TextTexture);

    m_TextTransform = new SoTransform;
    m_TextSeparator->addChild(m_TextTransform);

    m_TextVertexProperty = new SoVertexProperty;
    m_TextFaceSet = new SoFaceSet;
    m_TextFaceSet->vertexProperty.setValue(m_TextVertexProperty);
    m_TextFaceSet->numVertices.set1Value(0, 4);
    m_TextSeparator->addChild(m_TextFaceSet);
}

SoDatumLabel::~SoDatumLabel()
{
    if (m_Root) {
        m_Root->unref();
        m_Root = nullptr;
    }
    m_GeometryDepth = nullptr;
    m_LightModel = nullptr;
    m_GeometryColor = nullptr;
    m_DrawStyle = nullptr;
    m_LineVertexProperty = nullptr;
    m_LineSet = nullptr;
    m_TriangleVertexProperty = nullptr;
    m_TriangleFaceSet = nullptr;
    m_TextSwitch = nullptr;
    m_TextSeparator = nullptr;
    m_TextDepth = nullptr;
    m_TextBaseColor = nullptr;
    m_TextTexture = nullptr;
    m_TextTransform = nullptr;
    m_TextVertexProperty = nullptr;
    m_TextFaceSet = nullptr;
}

void SoDatumLabel::drawImage()
{
    const SbString* s = string.getValues(0);
    int num = string.getNum();
    if (num == 0) {
        this->image = SoSFImage();
        return;
    }

    QFont font(QString::fromLatin1(name.getValue(), -1), size.getValue());
    QFontMetrics fm(font);
    QString str = QString::fromUtf8(s[0].getString());
    QRect rect = fm.boundingRect(str);

    int w = Gui::QtTools::horizontalAdvance(fm, str);
    int h = fm.height();

    // No Valid text
    if (!w) {
        this->image = SoSFImage();
        return;
    }

    const SbColor& t = textColor.getValue();
    QColor front;
    front.setRgbF(t[0], t[1], t[2]);

    QImage image(w * sampling.getValue(), h * sampling.getValue(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(sampling.getValue());
    image.fill(0x00000000);

    QPainter painter(&image);
    if (useAntialiasing) {
        painter.setRenderHint(QPainter::Antialiasing);
    }

    painter.setPen(QPen(front, 2));
    painter.setFont(font);
    painter.drawText(0, fm.ascent() + rect.y(), w, rect.height(), Qt::AlignLeft, str);
    if (strikethrough.getValue()) {
        int strikepos = fm.ascent() - fm.strikeOutPos();
        painter.drawLine(0, strikepos, w, strikepos);
    }
    painter.end();

    Gui::BitmapFactory().convert(image, this->image);
}

namespace Gui
{
// helper class to determine the bounding box of a datum label
class DatumLabelBox
{
public:
    DatumLabelBox(float scale, SoDatumLabel* label)
        : scale {scale}
        , label {label}
    {}
    void computeBBox(SbBox3f& box, SbVec3f& center) const
    {
        std::vector<SbVec3f> corners;
        if (label->datumtype.getValue() == SoDatumLabel::DISTANCE
            || label->datumtype.getValue() == SoDatumLabel::DISTANCEX
            || label->datumtype.getValue() == SoDatumLabel::DISTANCEY) {
            corners = computeDistanceBBox();
        }
        else if (
            label->datumtype.getValue() == SoDatumLabel::RADIUS
            || label->datumtype.getValue() == SoDatumLabel::DIAMETER
        ) {
            corners = computeRadiusDiameterBBox();
        }
        else if (label->datumtype.getValue() == SoDatumLabel::ANGLE) {
            corners = computeAngleBBox();
        }
        else if (label->datumtype.getValue() == SoDatumLabel::SYMMETRIC) {
            corners = computeSymmetricBBox();
        }
        else if (label->datumtype.getValue() == SoDatumLabel::ARCLENGTH) {
            corners = computeArcLengthBBox();
        }

        getBBox(corners, box, center);
    }

private:
    void getBBox(const std::vector<SbVec3f>& corners, SbBox3f& box, SbVec3f& center) const
    {
        constexpr float floatMax = std::numeric_limits<float>::max();
        if (corners.size() > 1) {
            float minX = floatMax;
            float minY = floatMax;
            float maxX = -floatMax;
            float maxY = -floatMax;
            for (SbVec3f it : corners) {
                minX = (it[0] < minX) ? it[0] : minX;
                minY = (it[1] < minY) ? it[1] : minY;
                maxX = (it[0] > maxX) ? it[0] : maxX;
                maxY = (it[1] > maxY) ? it[1] : maxY;
            }

            // Store the bounding box
            box.setBounds(SbVec3f(minX, minY, 0.0F), SbVec3f(maxX, maxY, 0.0F));
            center = box.getCenter();
        }
    }
    std::vector<SbVec3f> computeDistanceBBox() const
    {
        SbVec2s imgsize;
        int nc {};
        int srcw = 1;
        int srch = 1;

        const unsigned char* dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio = (float)srcw / (float)srch;
        float imgHeight = scale * (float)(srch);
        float imgWidth = aspectRatio * imgHeight;

        // get the points stored in the pnt field
        const SbVec3f* points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 2) {
            return {};
        }

        // use the shared geometry calculation for consistency
        SoDatumLabel::DistanceGeometry geom = label->calculateDistanceGeometry(points);

        std::vector<SbVec3f> corners;
        float margin = imgHeight / 4.0F;

        // include main points and extension line endpoints
        corners.push_back(geom.p1);
        corners.push_back(geom.p2);
        corners.push_back(geom.perp1);
        corners.push_back(geom.perp2);

        // include text label area
        corners.push_back(
            geom.textOffset + geom.dir * (imgWidth / 2.0F + margin) + geom.normal * (srch + margin)
        );
        corners.push_back(
            geom.textOffset - geom.dir * (imgWidth / 2.0F + margin) + geom.normal * (srch + margin)
        );
        corners.push_back(
            geom.textOffset + geom.dir * (imgWidth / 2.0F + margin) - geom.normal * margin
        );
        corners.push_back(
            geom.textOffset - geom.dir * (imgWidth / 2.0F + margin) - geom.normal * margin
        );

        // include arrow head positions for better selection
        // arrows are positioned at dimension line endpoints (par1, par4)
        corners.push_back(geom.par1);
        corners.push_back(geom.par4);
        corners.push_back(geom.ar1);
        corners.push_back(geom.ar2);
        corners.push_back(geom.ar3);
        corners.push_back(geom.ar4);

        return corners;
    }

    std::vector<SbVec3f> computeRadiusDiameterBBox() const
    {
        // get the points stored in the pnt field
        const SbVec3f* points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 2) {
            return {};
        }

        // use the shared geometry calculation for consistency
        SoDatumLabel::DiameterGeometry geom = label->calculateDiameterGeometry(points);

        std::vector<SbVec3f> corners;

        // include main points and line segment points around text
        corners.push_back(geom.p1);
        corners.push_back(geom.p2);
        corners.push_back(geom.pnt1);
        corners.push_back(geom.pnt2);

        // include arrow head positions for better selection
        // first arrow head at p2
        corners.push_back(geom.ar0);
        corners.push_back(geom.ar1);
        corners.push_back(geom.ar2);

        // second arrow head for diameter (if applicable)
        if (geom.isDiameter) {
            corners.push_back(geom.ar0_1);
            corners.push_back(geom.ar1_1);
            corners.push_back(geom.ar2_1);
        }

        // sample points along the arc helper
        constexpr int numArcSamples = 6;

        const auto includeArcHelper = [&corners, &geom](float startAngle, float range) {
            if (range != 0.0) {
                for (int i = 0; i <= numArcSamples; i++) {
                    float t = static_cast<float>(i) / static_cast<float>(numArcSamples);
                    float angle = startAngle + t * range;
                    SbVec3f arcPoint = geom.center
                        + SbVec3f(geom.radius * cos(angle), geom.radius * sin(angle), 0);
                    corners.push_back(arcPoint);
                }
            }
        };

        includeArcHelper(geom.startAngle, geom.startRange);
        includeArcHelper(geom.endAngle, geom.endRange);

        return corners;
    }

    std::vector<SbVec3f> computeAngleBBox() const
    {
        SbVec2s imgsize;
        int nc {};
        int srcw = 1;
        int srch = 1;

        const unsigned char* dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio = (float)srcw / (float)srch;
        float imgHeight = scale * (float)(srch);
        float imgWidth = aspectRatio * imgHeight;

        // get the points stored in the pnt field
        const SbVec3f* points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 1) {
            return {};
        }

        // use the shared geometry calculation for consistency
        SoDatumLabel::AngleGeometry geom = label->calculateAngleGeometry(points);

        std::vector<SbVec3f> corners;

        // include extension line endpoints
        corners.push_back(geom.pnt1);
        corners.push_back(geom.pnt2);
        corners.push_back(geom.pnt3);
        corners.push_back(geom.pnt4);

        // include text label area
        SbVec3f img1 = SbVec3f(-imgWidth / 2.0F, -imgHeight / 2, 0.0F);
        SbVec3f img2 = SbVec3f(-imgWidth / 2.0F, imgHeight / 2, 0.0F);
        SbVec3f img3 = SbVec3f(imgWidth / 2.0F, -imgHeight / 2, 0.0F);
        SbVec3f img4 = SbVec3f(imgWidth / 2.0F, imgHeight / 2, 0.0F);

        img1 += geom.textOffset;
        img2 += geom.textOffset;
        img3 += geom.textOffset;
        img4 += geom.textOffset;

        corners.push_back(img1);
        corners.push_back(img2);
        corners.push_back(img3);
        corners.push_back(img4);

        // include arrow head positions for better selection
        corners.push_back(geom.startArrowBase);
        corners.push_back(geom.endArrowBase);

        // include arrow tips (base + direction * length)
        corners.push_back(geom.startArrowBase + geom.dirStart * geom.arrowLength);
        corners.push_back(geom.endArrowBase + geom.dirEnd * geom.arrowLength);

        return corners;
    }

    std::vector<SbVec3f> computeSymmetricBBox() const
    {
        // get the points stored in the pnt field
        const SbVec3f* points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 2) {
            return {};
        }

        // use shared geometry calculation
        SoDatumLabel::SymmetricGeometry geom = label->calculateSymmetricGeometry(points);

        // include all visual elements in bounding box
        std::vector<SbVec3f> corners;

        // main points (existing)
        corners.push_back(geom.p1);
        corners.push_back(geom.p2);

        // first arrow triangle points
        corners.push_back(geom.ar0);  // arrow tip
        corners.push_back(geom.ar1);  // arrow base point 1
        corners.push_back(geom.ar2);  // arrow base point 2

        // second arrow triangle points
        corners.push_back(geom.ar3);  // arrow tip
        corners.push_back(geom.ar4);  // arrow base point 1
        corners.push_back(geom.ar5);  // arrow base point 2

        return corners;
    }

    std::vector<SbVec3f> computeArcLengthBBox() const
    {
        // get the points stored in the pnt field
        const SbVec3f* points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 3) {
            return {};
        }

        // use shared geometry calculation
        SoDatumLabel::ArcLengthGeometry geom = label->calculateArcLengthGeometry(points);

        // get text area for existing text coverage
        SbVec2s imgsize;
        int nc {};
        int srcw = 1;
        int srch = 1;

        const unsigned char* dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio = (float)srcw / (float)srch;
        float imgHeight = scale * (float)(srch);
        float imgWidth = aspectRatio * imgHeight;

        // text orientation
        SbVec3f dir = (geom.p2 - geom.p1);
        dir.normalize();
        SbVec3f normal = SbVec3f(-dir[1], dir[0], 0);

        // include all visual elements in bounding box
        std::vector<SbVec3f> corners;

        // text area (existing coverage)
        float margin = imgHeight / 4.0F;
        corners.push_back(
            geom.textOffset + dir * (imgWidth / 2.0F + margin) - normal * (imgHeight / 2.0F + margin)
        );
        corners.push_back(
            geom.textOffset - dir * (imgWidth / 2.0F + margin) - normal * (imgHeight / 2.0F + margin)
        );
        corners.push_back(
            geom.textOffset + dir * (imgWidth / 2.0F + margin) + normal * (imgHeight / 2.0F + margin)
        );
        corners.push_back(
            geom.textOffset - dir * (imgWidth / 2.0F + margin) + normal * (imgHeight / 2.0F + margin)
        );

        // extension line endpoints
        corners.push_back(geom.pnt1);  // start point
        corners.push_back(geom.pnt2);  // extension end 1
        corners.push_back(geom.pnt3);  // end point
        corners.push_back(geom.pnt4);  // extension end 2

        // arc sample points (8 points along the curve for better coverage)
        int numSamples = 8;
        for (int i = 0; i < numSamples; i++) {
            float t = (float)i / (numSamples - 1);
            float angle = geom.startangle + t * (geom.endangle - geom.startangle);
            SbVec3f arcPoint = geom.arcCenter
                + SbVec3f(geom.arcRadius * cos(angle), geom.arcRadius * sin(angle), 0);
            corners.push_back(arcPoint);
        }

        // arrow head tips (base + direction * length)
        float arrowLength = geom.margin * 2;
        SbVec3f startArrowTip = geom.pnt2 + geom.dirStart * arrowLength;
        SbVec3f endArrowTip = geom.pnt4 + geom.dirEnd * arrowLength;
        corners.push_back(startArrowTip);
        corners.push_back(endArrowTip);

        return corners;
    }

private:
    float scale;
    SoDatumLabel* label;
};
}  // namespace Gui

void SoDatumLabel::computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center)
{
    SoState* state = action->getState();
    float scale = getScaleFactor(state);

    Gui::DatumLabelBox datumBox(scale, this);
    datumBox.computeBBox(box, center);
}

SbVec3f SoDatumLabel::getLabelTextCenter()
{
    // Get the points stored
    int numPts = this->pnts.getNum();
    if (numPts < 2) {
        return {};
    }

    const SbVec3f* points = this->pnts.getValues(0);
    SbVec3f p1 = points[0];
    SbVec3f p2 = points[1];

    if (datumtype.getValue() == SoDatumLabel::DISTANCE
        || datumtype.getValue() == SoDatumLabel::DISTANCEX
        || datumtype.getValue() == SoDatumLabel::DISTANCEY) {
        return getLabelTextCenterDistance(p1, p2);
    }
    if (datumtype.getValue() == SoDatumLabel::RADIUS
        || datumtype.getValue() == SoDatumLabel::DIAMETER) {
        return getLabelTextCenterDiameter(p1, p2);
    }
    if (datumtype.getValue() == SoDatumLabel::ANGLE) {
        return getLabelTextCenterAngle(p1);
    }
    if (datumtype.getValue() == SoDatumLabel::ARCLENGTH) {
        if (numPts >= 3) {
            SbVec3f p3 = points[2];
            return getLabelTextCenterArcLength(p1, p2, p3);
        }
    }

    return p1;
}

SbVec3f SoDatumLabel::getLabelTextCenterDistance(const SbVec3f& p1, const SbVec3f& p2)
{
    float length = param1.getValue();
    float length2 = param2.getValue();

    SbVec3f dir;
    SbVec3f normal;

    constexpr float floatEpsilon = std::numeric_limits<float>::epsilon();
    if (datumtype.getValue() == SoDatumLabel::DISTANCE) {
        dir = (p2 - p1);
    }
    else if (datumtype.getValue() == SoDatumLabel::DISTANCEX) {
        dir = SbVec3f((p2[0] - p1[0] >= floatEpsilon) ? 1 : -1, 0, 0);
    }
    else if (datumtype.getValue() == SoDatumLabel::DISTANCEY) {
        dir = SbVec3f(0, (p2[1] - p1[1] >= floatEpsilon) ? 1 : -1, 0);
    }

    dir.normalize();
    normal = SbVec3f(-dir[1], dir[0], 0);

    float normproj12 = (p2 - p1).dot(normal);
    SbVec3f p1_ = p1 + normproj12 * normal;

    SbVec3f midpos = (p1_ + p2) / 2;

    SbVec3f textCenter = midpos + normal * length + dir * length2;
    return textCenter;
}

SbVec3f SoDatumLabel::getLabelTextCenterDiameter(const SbVec3f& p1, const SbVec3f& p2)
{
    SbVec3f dir = (p2 - p1);
    dir.normalize();

    float length = this->param1.getValue();
    SbVec3f textCenter = p2 + length * dir;
    return textCenter;
}

SbVec3f SoDatumLabel::getLabelTextCenterAngle(const SbVec3f& p0)
{
    // Load the Parameters
    float length = param1.getValue();
    float startangle = param2.getValue();
    float range = param3.getValue();
    float len2 = 2.0F * length;

    return getAngleTextCenter(p0, startangle, range, len2);
}

SbVec3f SoDatumLabel::getLabelTextCenterArcLength(
    const SbVec3f& ctr,
    const SbVec3f& p1,
    const SbVec3f& p2
) const
{
    SbVec3f points[3] = {ctr, p1, p2};
    return calculateArcLengthGeometry(points).textOffset;
}


void SoDatumLabel::generateDistancePrimitives(SoAction* action, const SbVec3f& p1, const SbVec3f& p2)
{
    SbVec3f points[2] = {p1, p2};

    DistanceGeometry geom = calculateDistanceGeometry(points);

    // generate selectable primitive for txt label at elevated Z for selection above geometry
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img3 = SbVec3f(this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img4 = SbVec3f(this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), ZARROW_TEXT_OFFSET);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), ZARROW_TEXT_OFFSET);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), ZARROW_TEXT_OFFSET);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), ZARROW_TEXT_OFFSET);

    img1 += geom.textOffset;
    img2 += geom.textOffset;
    img3 += geom.textOffset;
    img4 += geom.textOffset;

    // text label selection primitive
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLE_STRIP);
    pv.setPoint(img1);
    shapeVertex(&pv);
    pv.setPoint(img2);
    shapeVertex(&pv);
    pv.setPoint(img3);
    shapeVertex(&pv);
    pv.setPoint(img4);
    shapeVertex(&pv);
    this->endShape();

    // beginning of generation of selectable primitives for lines
    float lineWidth = geom.margin * 0.8f;  // adjust the width for selection

    // ext lines
    generateLineSelectionPrimitive(action, geom.p1, geom.perp1, lineWidth);
    generateLineSelectionPrimitive(action, geom.p2, geom.perp2, lineWidth);

    // dim lines
    generateLineSelectionPrimitive(action, geom.par1, geom.par2, lineWidth);
    generateLineSelectionPrimitive(action, geom.par3, geom.par4, lineWidth);

    // begin generation of selectable primitives for arrow-heads at elevated Z
    this->beginShape(action, TRIANGLES);
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    // 1st arrow-head
    pv.setPoint(SbVec3f(geom.par1[0], geom.par1[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar1[0], geom.ar1[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar2[0], geom.ar2[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);

    // 2nd arrow-head
    pv.setPoint(SbVec3f(geom.par4[0], geom.par4[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar3[0], geom.ar3[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar4[0], geom.ar4[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);

    this->endShape();
}

void SoDatumLabel::generateDiameterPrimitives(SoAction* action, const SbVec3f& p1, const SbVec3f& p2)
{
    SbVec3f points[2] = {p1, p2};
    DiameterGeometry geom = calculateDiameterGeometry(points);

    // generate selectable primitive for text label at elevated Z for selection above geometry
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img3 = SbVec3f(this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img4 = SbVec3f(this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), ZARROW_TEXT_OFFSET);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), ZARROW_TEXT_OFFSET);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), ZARROW_TEXT_OFFSET);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), ZARROW_TEXT_OFFSET);

    img1 += geom.textOffset;
    img2 += geom.textOffset;
    img3 += geom.textOffset;
    img4 += geom.textOffset;

    // txt label selection primitive
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLE_STRIP);
    pv.setPoint(img1);
    shapeVertex(&pv);
    pv.setPoint(img2);
    shapeVertex(&pv);
    pv.setPoint(img3);
    shapeVertex(&pv);
    pv.setPoint(img4);
    shapeVertex(&pv);
    this->endShape();

    // generate selectable primitives for dimension lines
    float lineWidth = geom.margin * 0.8f;

    // main dimension lines (from center/start to text area and from text area to end)
    generateLineSelectionPrimitive(action, geom.p1, geom.pnt1, lineWidth);
    generateLineSelectionPrimitive(action, geom.pnt2, geom.p2, lineWidth);

    // Generate selectable primitives for arrow heads at elevated Z
    this->beginShape(action, TRIANGLES);
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    // first arrow-head
    pv.setPoint(SbVec3f(geom.ar0[0], geom.ar0[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar1[0], geom.ar1[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar2[0], geom.ar2[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);

    // second arrow-head but only for diameter
    if (geom.isDiameter) {
        pv.setPoint(SbVec3f(geom.ar0_1[0], geom.ar0_1[1], ZARROW_TEXT_OFFSET));
        shapeVertex(&pv);
        pv.setPoint(SbVec3f(geom.ar1_1[0], geom.ar1_1[1], ZARROW_TEXT_OFFSET));
        shapeVertex(&pv);
        pv.setPoint(SbVec3f(geom.ar2_1[0], geom.ar2_1[1], ZARROW_TEXT_OFFSET));
        shapeVertex(&pv);
    }

    this->endShape();

    const auto generateSelectablePrimitiveForArcHelper = [&, this](float startAngle, float range) {
        if (range != 0.0) {
            int countSegments = std::max(6, abs(int(50.0 * range / (2 * std::numbers::pi))));
            double segment = range / (countSegments - 1);

            // create selectable line segments for the arc
            for (int i = 0; i < countSegments - 1; i++) {
                double theta1 = startAngle + segment * i;
                double theta2 = startAngle + segment * (i + 1);
                SbVec3f v1 = geom.center
                    + SbVec3f(geom.radius * cos(theta1), geom.radius * sin(theta1), 0);
                SbVec3f v2 = geom.center
                    + SbVec3f(geom.radius * cos(theta2), geom.radius * sin(theta2), 0);
                generateLineSelectionPrimitive(action, v1, v2, lineWidth * 0.5f);
            }
        }
    };

    generateSelectablePrimitiveForArcHelper(geom.startAngle, geom.startRange);
    generateSelectablePrimitiveForArcHelper(geom.endAngle, geom.endRange);
}

void SoDatumLabel::generateAnglePrimitives(SoAction* action, const SbVec3f& p0)
{
    // use shared geometry calculation
    SbVec3f points[1] = {p0};
    AngleGeometry geom = calculateAngleGeometry(points);

    // generate selectable primitive for text label at elevated Z for selection above geometry
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img3 = SbVec3f(this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img4 = SbVec3f(this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);

    img1 += geom.textOffset;
    img2 += geom.textOffset;
    img3 += geom.textOffset;
    img4 += geom.textOffset;

    // text label selection primitive
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLE_STRIP);
    pv.setPoint(img1);
    shapeVertex(&pv);
    pv.setPoint(img2);
    shapeVertex(&pv);
    pv.setPoint(img3);
    shapeVertex(&pv);
    pv.setPoint(img4);
    shapeVertex(&pv);
    this->endShape();

    // generate selectable primitives for dimension lines
    float lineWidth = geom.margin * 0.8f;

    // extension lines
    generateLineSelectionPrimitive(action, geom.pnt1, geom.pnt2, lineWidth);
    generateLineSelectionPrimitive(action, geom.pnt3, geom.pnt4, lineWidth);

    // generate selectable primitives for arc segments
    float arcWidth = geom.margin * 0.6f;

    // arc before text
    generateArcSelectionPrimitive(
        action,
        geom.p0,
        geom.r,
        geom.startangle,
        geom.startangle + geom.range / 2.0 - geom.textMargin,
        arcWidth
    );

    // arc after text
    generateArcSelectionPrimitive(
        action,
        geom.p0,
        geom.r,
        geom.startangle + geom.range / 2.0 + geom.textMargin,
        geom.endangle,
        arcWidth
    );

    // generate selectable primitives for arrow heads
    generateArrowSelectionPrimitive(
        action,
        geom.startArrowBase,
        geom.dirStart,
        geom.arrowWidth,
        geom.arrowLength
    );
    generateArrowSelectionPrimitive(
        action,
        geom.endArrowBase,
        geom.dirEnd,
        geom.arrowWidth,
        geom.arrowLength
    );
}

void SoDatumLabel::generateSymmetricPrimitives(SoAction* action, const SbVec3f& p1, const SbVec3f& p2)
{
    // use shared geometry calculation
    SbVec3f points[2] = {p1, p2};
    SymmetricGeometry geom = calculateSymmetricGeometry(points);

    // generate selectable primitives for lines
    float lineWidth = geom.margin * 0.8f;

    // lines from endpoints to arrow tips
    generateLineSelectionPrimitive(action, geom.p1, geom.ar0, lineWidth);
    generateLineSelectionPrimitive(action, geom.p2, geom.ar3, lineWidth);

    // generate selectable primitives for arrow heads as triangles at elevated Z
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLES);

    // first arrow
    pv.setPoint(SbVec3f(geom.ar0[0], geom.ar0[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar1[0], geom.ar1[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar2[0], geom.ar2[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);

    // second arrow
    pv.setPoint(SbVec3f(geom.ar3[0], geom.ar3[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar4[0], geom.ar4[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(geom.ar5[0], geom.ar5[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);

    this->endShape();
}

void SoDatumLabel::generateArcLengthPrimitives(
    SoAction* action,
    const SbVec3f& ctr,
    const SbVec3f& p1,
    const SbVec3f& p2
)
{
    // use shared geometry calculation
    SbVec3f points[3] = {ctr, p1, p2};
    ArcLengthGeometry geom = calculateArcLengthGeometry(points);

    // generate selectable primitive for text label at elevated Z for selection above geometry
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img3 = SbVec3f(this->imgWidth / 2, -this->imgHeight / 2, ZARROW_TEXT_OFFSET);
    SbVec3f img4 = SbVec3f(this->imgWidth / 2, this->imgHeight / 2, ZARROW_TEXT_OFFSET);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), ZARROW_TEXT_OFFSET);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), ZARROW_TEXT_OFFSET);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), ZARROW_TEXT_OFFSET);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), ZARROW_TEXT_OFFSET);

    img1 += geom.textOffset;
    img2 += geom.textOffset;
    img3 += geom.textOffset;
    img4 += geom.textOffset;

    // text label selection primitive
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLE_STRIP);
    pv.setPoint(img1);
    shapeVertex(&pv);
    pv.setPoint(img2);
    shapeVertex(&pv);
    pv.setPoint(img3);
    shapeVertex(&pv);
    pv.setPoint(img4);
    shapeVertex(&pv);
    this->endShape();

    // generate selectable primitives for lines
    float lineWidth = geom.margin * 0.8f;

    // extension lines
    generateLineSelectionPrimitive(action, geom.pnt1, geom.pnt2, lineWidth);
    generateLineSelectionPrimitive(action, geom.pnt3, geom.pnt4, lineWidth);

    // generate selectable primitive for arc
    generateArcSelectionPrimitive(
        action,
        geom.arcCenter,
        geom.arcRadius,
        geom.startangle,
        geom.endangle,
        lineWidth
    );

    // generate selectable primitives for arrow heads
    float arrowLength = geom.margin * 2;
    float arrowWidth = geom.margin * 0.5F;

    generateArrowSelectionPrimitive(action, geom.pnt2, geom.dirStart, arrowWidth, arrowLength);
    generateArrowSelectionPrimitive(action, geom.pnt4, geom.dirEnd, arrowWidth, arrowLength);
}

void SoDatumLabel::generatePrimitives(SoAction* action)
{
    // Initialisation check (needs something more sensible) prevents an infinite loop bug
    constexpr float floatEpsilon = std::numeric_limits<float>::epsilon();
    if (this->imgHeight <= floatEpsilon || this->imgWidth <= floatEpsilon) {
        return;
    }

    int numPts = this->pnts.getNum();
    if (numPts < 2) {
        return;
    }

    // Get the points stored
    const SbVec3f* points = this->pnts.getValues(0);
    SbVec3f p1 = points[0];
    SbVec3f p2 = points[1];

    // Change the offset and bounding box parameters depending on Datum Type
    if (this->datumtype.getValue() == DISTANCE || this->datumtype.getValue() == DISTANCEX
        || this->datumtype.getValue() == DISTANCEY) {

        generateDistancePrimitives(action, p1, p2);
    }
    else if (this->datumtype.getValue() == RADIUS || this->datumtype.getValue() == DIAMETER) {

        generateDiameterPrimitives(action, p1, p2);
    }
    else if (this->datumtype.getValue() == ANGLE) {

        generateAnglePrimitives(action, p1);
    }
    else if (this->datumtype.getValue() == SYMMETRIC) {

        generateSymmetricPrimitives(action, p1, p2);
    }
    else if (this->datumtype.getValue() == ARCLENGTH) {

        if (numPts >= 3) {
            SbVec3f p3 = points[2];
            generateArcLengthPrimitives(action, p1, p2, p3);
        }
    }
}

void SoDatumLabel::notify(SoNotList* l)
{
    SoField* f = l->getLastField();
    if (f == &this->string) {
        this->glimagevalid = false;
    }
    else if (f == &this->textColor) {
        this->glimagevalid = false;
    }
    else if (f == &this->name) {
        this->glimagevalid = false;
    }
    else if (f == &this->size) {
        this->glimagevalid = false;
    }
    else if (f == &this->image) {
        this->glimagevalid = false;
    }
    inherited::notify(l);
}

float SoDatumLabel::getScaleFactor(SoState* state) const
{
    /**Remark from Stefan Tröger:
     * The scale calculation is based on knowledge of SbViewVolume::getWorldToScreenScale
     * implementation internals. The factor returned from this function is calculated from the view
     * frustums nearplane width, height is not taken into account, and hence we divide it with the
     * viewport width to get the exact pixel scale factor. This is not documented and therefore may
     * change on later coin versions!
     */
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    // As reference use the center point the camera is looking at on the focal plane
    // because then independent of the camera we get a constant scale factor when panning.
    // If we used (0,0,0) instead then the scale factor would change heavily in perspective
    // rendering mode. See #0002921 and #0002922.
    // It's important to use the distance to the focal plane an not near or far plane because
    // depending on additionally displayed objects they may change heavily and thus impact the
    // scale factor. See #7082 and #7860.
    float focal = SoFocalDistanceElement::get(state);
    SbVec3f center = vv.getSightPoint(focal);
    float scale = vv.getWorldToScreenScale(center, 1.F);
    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    SbVec2s vp_size = vp.getViewportSizePixels();
    scale /= float(vp_size[0]);

    return scale;
}

void SoDatumLabel::setVertexZ(SbVec3f& point, float z) const
{
    point[2] = z;
}

void SoDatumLabel::ensureCoinGeometry(const SbVec3f* points, int numPoints)
{
    if (!points || numPoints <= 0 || !m_LineVertexProperty || !m_LineSet
        || !m_TriangleVertexProperty || !m_TriangleFaceSet) {
        return;
    }

    std::vector<SbVec3f> lineVertices;
    std::vector<int32_t> lineCounts;
    std::vector<SbVec3f> triangleVertices;
    std::vector<int32_t> triangleCounts;

    const auto addTriangle = [&](SbVec3f a, SbVec3f b, SbVec3f c) {
        setVertexZ(a, ZARROW_TEXT_OFFSET);
        setVertexZ(b, ZARROW_TEXT_OFFSET);
        setVertexZ(c, ZARROW_TEXT_OFFSET);
        triangleVertices.push_back(a);
        triangleVertices.push_back(b);
        triangleVertices.push_back(c);
        triangleCounts.push_back(3);
    };

    const auto type = static_cast<Type>(datumtype.getValue());

    if (type == DISTANCE || type == DISTANCEX || type == DISTANCEY) {
        if (numPoints >= 2) {
            const DistanceGeometry geom = calculateDistanceGeometry(points);

            if (param1.getValue() != 0.0F) {
                appendLine(lineVertices, lineCounts, geom.p1, geom.perp1);
                appendLine(lineVertices, lineCounts, geom.p2, geom.perp2);
            }
            appendLine(lineVertices, lineCounts, geom.par1, geom.par2);
            appendLine(lineVertices, lineCounts, geom.par3, geom.par4);

            addTriangle(geom.par1, geom.ar1, geom.ar2);
            addTriangle(geom.par4, geom.ar3, geom.ar4);

            if (type == DISTANCE && numPoints >= 4) {
                const float range1 = param4.getValue();
                if (range1 != 0.0F) {
                    const float startAngle1 = param3.getValue();
                    const float radius1 = param5.getValue();
                    const SbVec3f center1 = points[2];
                    appendArc(lineVertices, lineCounts, center1, radius1, startAngle1, startAngle1 + range1);
                }

                const float range2 = param7.getValue();
                if (range2 != 0.0F) {
                    const float startAngle2 = param6.getValue();
                    const float radius2 = param8.getValue();
                    const SbVec3f center2 = points[3];
                    appendArc(lineVertices, lineCounts, center2, radius2, startAngle2, startAngle2 + range2);
                }
            }
        }
    }
    else if (type == RADIUS || type == DIAMETER) {
        if (numPoints >= 2) {
            const DiameterGeometry geom = calculateDiameterGeometry(points);

            appendLine(lineVertices, lineCounts, geom.p1, geom.pnt1);
            appendLine(lineVertices, lineCounts, geom.pnt2, geom.p2);

            addTriangle(geom.ar0, geom.ar1, geom.ar2);
            if (geom.isDiameter) {
                addTriangle(geom.ar0_1, geom.ar1_1, geom.ar2_1);
            }

            if (geom.startRange != 0.0F) {
                appendArc(
                    lineVertices,
                    lineCounts,
                    geom.center,
                    geom.radius,
                    geom.startAngle,
                    geom.startAngle + geom.startRange
                );
            }
            if (geom.endRange != 0.0F) {
                appendArc(
                    lineVertices,
                    lineCounts,
                    geom.center,
                    geom.radius,
                    geom.endAngle,
                    geom.endAngle + geom.endRange
                );
            }
        }
    }
    else if (type == ANGLE) {
        if (numPoints >= 1) {
            const AngleGeometry geom = calculateAngleGeometry(points);

            appendArc(
                lineVertices,
                lineCounts,
                geom.p0,
                geom.r,
                geom.startangle,
                geom.startangle + geom.range / 2.0F - static_cast<float>(geom.textMargin)
            );
            appendArc(
                lineVertices,
                lineCounts,
                geom.p0,
                geom.r,
                geom.startangle + geom.range / 2.0F + static_cast<float>(geom.textMargin),
                geom.endangle
            );

            appendLine(lineVertices, lineCounts, geom.pnt1, geom.pnt2);
            appendLine(lineVertices, lineCounts, geom.pnt3, geom.pnt4);

            appendArrowTriangle(
                triangleVertices,
                triangleCounts,
                geom.startArrowBase,
                geom.dirStart,
                geom.arrowWidth,
                geom.arrowLength
            );
            appendArrowTriangle(
                triangleVertices,
                triangleCounts,
                geom.endArrowBase,
                geom.dirEnd,
                geom.arrowWidth,
                geom.arrowLength
            );
        }
    }
    else if (type == SYMMETRIC) {
        if (numPoints >= 2) {
            const SymmetricGeometry geom = calculateSymmetricGeometry(points);

            SbVec3f p1 = geom.p1;
            SbVec3f ar0 = geom.ar0;
            SbVec3f p2 = geom.p2;
            SbVec3f ar3 = geom.ar3;
            setVertexZ(p1, ZCONSTR);
            setVertexZ(ar0, ZCONSTR);
            setVertexZ(p2, ZCONSTR);
            setVertexZ(ar3, ZCONSTR);
            appendLine(lineVertices, lineCounts, p1, ar0);
            appendLine(lineVertices, lineCounts, p2, ar3);

            appendLine(
                lineVertices,
                lineCounts,
                withZ(geom.ar0, ZARROW_TEXT_OFFSET),
                withZ(geom.ar1, ZARROW_TEXT_OFFSET)
            );
            appendLine(
                lineVertices,
                lineCounts,
                withZ(geom.ar0, ZARROW_TEXT_OFFSET),
                withZ(geom.ar2, ZARROW_TEXT_OFFSET)
            );
            appendLine(
                lineVertices,
                lineCounts,
                withZ(geom.ar3, ZARROW_TEXT_OFFSET),
                withZ(geom.ar4, ZARROW_TEXT_OFFSET)
            );
            appendLine(
                lineVertices,
                lineCounts,
                withZ(geom.ar3, ZARROW_TEXT_OFFSET),
                withZ(geom.ar5, ZARROW_TEXT_OFFSET)
            );
        }
    }
    else if (type == ARCLENGTH) {
        if (numPoints >= 3) {
            const ArcLengthGeometry geom = calculateArcLengthGeometry(points);

            appendArc(
                lineVertices,
                lineCounts,
                geom.arcCenter,
                geom.arcRadius,
                geom.startangle,
                geom.endangle
            );
            appendLine(lineVertices, lineCounts, geom.pnt1, geom.pnt2);
            appendLine(lineVertices, lineCounts, geom.pnt3, geom.pnt4);

            const float arrowLength = geom.margin * 2.0F;
            const float arrowWidth = geom.margin * 0.5F;
            appendArrowTriangle(
                triangleVertices,
                triangleCounts,
                geom.pnt2,
                geom.dirStart,
                arrowWidth,
                arrowLength
            );
            appendArrowTriangle(
                triangleVertices,
                triangleCounts,
                geom.pnt4,
                geom.dirEnd,
                arrowWidth,
                arrowLength
            );
        }
    }

    if (!lineVertices.empty()) {
        m_LineVertexProperty->vertex
            .setValues(0, static_cast<int>(lineVertices.size()), lineVertices.data());
        m_LineSet->numVertices.setValues(0, static_cast<int>(lineCounts.size()), lineCounts.data());
    }
    else {
        m_LineVertexProperty->vertex.setNum(0);
        m_LineSet->numVertices.setNum(0);
    }

    if (!triangleVertices.empty()) {
        m_TriangleVertexProperty->vertex
            .setValues(0, static_cast<int>(triangleVertices.size()), triangleVertices.data());
        m_TriangleFaceSet->numVertices
            .setValues(0, static_cast<int>(triangleCounts.size()), triangleCounts.data());
    }
    else {
        m_TriangleVertexProperty->vertex.setNum(0);
        m_TriangleFaceSet->numVertices.setNum(0);
    }
}

void SoDatumLabel::ensureCoinText(SoState* state, int srcw, int srch, float angle, const SbVec3f& textOffset)
{
    if (!state || !m_TextSwitch || !m_TextVertexProperty || !m_TextFaceSet || !m_TextTransform
        || !m_TextTexture) {
        return;
    }

    SbVec2s imgsize;
    int nc {};
    const unsigned char* dataptr = this->image.getValue(imgsize, nc);
    if (!dataptr || srcw <= 0 || srch <= 0 || imgWidth <= 0.0f || imgHeight <= 0.0f) {
        m_TextSwitch->whichChild.setValue(SO_SWITCH_NONE);
        m_TextVertexProperty->vertex.setNum(0);
        m_TextVertexProperty->texCoord.setNum(0);
        m_TextFaceSet->numVertices.setNum(0);
        return;
    }

    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbVec3f z = vv.zVector();
    const bool flip = norm.getValue().dot(z) > std::numeric_limits<float>::epsilon();
    const float sketchAngle = getSketchRotationAngle(state, vv, flip);
    const float absLabelAngle = std::abs(sketchAngle + angle);

    constexpr float quarter = 90.0F;
    constexpr float hysteresis = 15.0F;
    constexpr float threshold = Base::toRadians(quarter + hysteresis);

    if ((flip && absLabelAngle > threshold) || (!flip && absLabelAngle < threshold)) {
        angle += std::numbers::pi;
    }

    m_TextTransform->translation.setValue(textOffset);
    m_TextTransform->rotation.setValue(SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), angle));

    const float left = -imgWidth / 2.0f;
    const float right = imgWidth / 2.0f;
    const float bottom = -imgHeight / 2.0f;
    const float top = imgHeight / 2.0f;

    m_TextVertexProperty->vertex.setNum(4);
    m_TextVertexProperty->vertex.set1Value(0, SbVec3f(left, top, 0.0f));
    m_TextVertexProperty->vertex.set1Value(1, SbVec3f(left, bottom, 0.0f));
    m_TextVertexProperty->vertex.set1Value(2, SbVec3f(right, bottom, 0.0f));
    m_TextVertexProperty->vertex.set1Value(3, SbVec3f(right, top, 0.0f));

    m_TextVertexProperty->texCoord.setNum(4);
    if (flip) {
        m_TextVertexProperty->texCoord.set1Value(0, SbVec2f(0.0f, 1.0f));
        m_TextVertexProperty->texCoord.set1Value(1, SbVec2f(0.0f, 0.0f));
        m_TextVertexProperty->texCoord.set1Value(2, SbVec2f(1.0f, 0.0f));
        m_TextVertexProperty->texCoord.set1Value(3, SbVec2f(1.0f, 1.0f));
    }
    else {
        m_TextVertexProperty->texCoord.set1Value(0, SbVec2f(1.0f, 1.0f));
        m_TextVertexProperty->texCoord.set1Value(1, SbVec2f(1.0f, 0.0f));
        m_TextVertexProperty->texCoord.set1Value(2, SbVec2f(0.0f, 0.0f));
        m_TextVertexProperty->texCoord.set1Value(3, SbVec2f(0.0f, 1.0f));
    }

    m_TextFaceSet->numVertices.set1Value(0, 4);
    m_TextSwitch->whichChild.setValue(0);
}

void SoDatumLabel::GLRender(SoGLRenderAction* action)
{
    SoState* state = action->getState();

    if (!shouldGLRender(action)) {
        return;
    }
    if (action->handleTransparency(true)) {
        return;
    }

    const float scale = getScaleFactor(state);
    bool hasText = hasDatumText();

    int srcw = 1;
    int srch = 1;

    if (hasText) {
        getDimension(scale, srcw, srch);
    }

    if (this->datumtype.getValue() == SYMMETRIC) {
        this->imgHeight = scale * 25.0F;
        this->imgWidth = scale * 25.0F;
    }

    // Get the points stored in the pnt field
    const SbVec3f* points = this->pnts.getValues(0);

    state->push();

    // Annotation faces should stay visible even when an ancestor enables back-face culling.
    SoLazyElement::setBackfaceCulling(state, FALSE);

    const auto type = static_cast<Type>(datumtype.getValue());
    const int numPoints = this->pnts.getNum();
    const bool isDistance = type == DISTANCE || type == DISTANCEX || type == DISTANCEY;
    if (isDistance && numPoints < 2) {
        SoDebugError::postWarning("SoDatumLabel::GLRender", "Too few points to render distance label");
    }

    if (hasText) {
        // Text labels are rendered as SoTexture2 on a quad. Coin's default texture quality
        // (0.5) enables mipmaps, which can blur small UI text. Keep linear filtering but
        // avoid mipmaps for crisper results.
        SoTextureQualityElement::set(state, this, 0.49F);
        SoLazyElement::setTransparencyType(state, static_cast<int32_t>(SoGLRenderAction::BLEND));
    }

    ensureCoinGeometry(points, numPoints);

    float angle = 0.0F;
    SbVec3f textOffset;
    if (hasText) {
        if (isDistance && numPoints >= 2) {
            const DistanceGeometry geom = calculateDistanceGeometry(points);
            angle = geom.angle;
            textOffset = geom.textOffset;
        }
        else if (type == RADIUS || type == DIAMETER) {
            const DiameterGeometry geom = calculateDiameterGeometry(points);
            angle = geom.angle;
            textOffset = geom.textOffset;
        }
        else if (type == ANGLE) {
            const AngleGeometry geom = calculateAngleGeometry(points);
            angle = geom.angle;
            textOffset = geom.textOffset;
        }
        else if (type == ARCLENGTH && this->pnts.getNum() >= 3) {
            const ArcLengthGeometry geom = calculateArcLengthGeometry(points);
            angle = geom.angle;
            textOffset = geom.textOffset;
        }

        ensureCoinText(state, srcw, srch, angle, textOffset);
    }
    else if (m_TextSwitch) {
        m_TextSwitch->whichChild.setValue(SO_SWITCH_NONE);
    }

    if (m_Root) {
        m_Root->GLRender(action);
    }

    state->pop();
}

bool SoDatumLabel::hasDatumText() const
{
    const SbString* s = string.getValues(0);
    return (s->getLength() > 0);
}

void SoDatumLabel::getDimension(float scale, int& srcw, int& srch)
{
    SbVec2s imgsize;
    int nc {};

    if (!this->glimagevalid) {
        drawImage();
        this->glimagevalid = true;
    }

    const unsigned char* dataptr = this->image.getValue(imgsize, nc);
    if (!dataptr) {  // no image
        return;
    }

    srcw = imgsize[0];
    srch = imgsize[1];

    float aspectRatio = (float)srcw / (float)srch;
    this->imgHeight = scale * (float)(srch) / sampling.getValue();
    this->imgWidth = aspectRatio * (float)this->imgHeight;
}

void SoDatumLabel::setPoints(SbVec3f p1, SbVec3f p2)
{
    pnts.setNum(2);
    SbVec3f* verts = pnts.startEditing();
    verts[0] = p1;
    verts[1] = p2;
    pnts.finishEditing();
}
// NOLINTEND(readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)

SoDatumLabel::DistanceGeometry SoDatumLabel::calculateDistanceGeometry(const SbVec3f* points) const
{
    using std::numbers::pi;

    SoDatumLabel::DistanceGeometry geom;

    float length = this->param1.getValue();
    float length2 = this->param2.getValue();

    geom.p1 = points[0];
    geom.p2 = points[1];

    constexpr float floatEpsilon = std::numeric_limits<float>::epsilon();
    if (this->datumtype.getValue() == DISTANCE) {
        geom.dir = (geom.p2 - geom.p1);
    }
    else if (this->datumtype.getValue() == DISTANCEX) {
        geom.dir = SbVec3f((geom.p2[0] - geom.p1[0] >= floatEpsilon) ? 1 : -1, 0, 0);
    }
    else if (this->datumtype.getValue() == DISTANCEY) {
        geom.dir = SbVec3f(0, (geom.p2[1] - geom.p1[1] >= floatEpsilon) ? 1 : -1, 0);
    }

    geom.dir.normalize();
    geom.normal = SbVec3f(-geom.dir[1], geom.dir[0], 0);

    // when the datum line is not parallel to p1-p2 the projection of
    // p1-p2 on normal is not zero, p2 is considered as reference and p1
    // is replaced by its projection p1_
    float normproj12 = (geom.p2 - geom.p1).dot(geom.normal);
    SbVec3f p1_ = geom.p1 + normproj12 * geom.normal;

    geom.midpos = (p1_ + geom.p2) / 2;

    // Get magnitude of angle between horizontal
    geom.angle = atan2f(geom.dir[1], geom.dir[0]);
    if (geom.angle > pi / 2 + pi / 12) {
        geom.angle -= (float)pi;
    }
    else if (geom.angle <= -pi / 2 + pi / 12) {
        geom.angle += (float)pi;
    }

    geom.textOffset = geom.midpos + geom.normal * length + geom.dir * length2;

    geom.margin = this->imgHeight / 3.0F;

    float offset1 = ((length + normproj12 < 0) ? -1.F : 1.F) * geom.margin;
    float offset2 = ((length < 0) ? -1 : 1) * geom.margin;

    geom.perp1 = p1_ + geom.normal * (length + offset1);
    geom.perp2 = geom.p2 + geom.normal * (length + offset2);

    // Calculate the coordinates for the parallel datum lines
    geom.par1 = p1_ + geom.normal * length;
    geom.par2 = geom.midpos + geom.normal * length
        + geom.dir * (length2 - this->imgWidth / 2 - geom.margin);
    geom.par3 = geom.midpos + geom.normal * length
        + geom.dir * (length2 + this->imgWidth / 2 + geom.margin);
    geom.par4 = geom.p2 + geom.normal * length;

    geom.flipTriang = false;

    if ((geom.par3 - geom.par1).dot(geom.dir) > (geom.par4 - geom.par1).length()) {
        // Increase Margin to improve visibility
        float tmpMargin = this->imgHeight / 0.75F;
        geom.par3 = geom.par4;
        if ((geom.par2 - geom.par1).dot(geom.dir) > (geom.par4 - geom.par1).length()) {
            geom.par3 = geom.par2;
            geom.par2 = geom.par1 - geom.dir * tmpMargin;
            geom.flipTriang = true;
        }
    }
    else if ((geom.par2 - geom.par1).dot(geom.dir) < 0.F) {
        float tmpMargin = this->imgHeight / 0.75F;
        geom.par2 = geom.par1;
        if ((geom.par3 - geom.par1).dot(geom.dir) < 0.F) {
            geom.par2 = geom.par3;
            geom.par3 = geom.par4 + geom.dir * tmpMargin;
            geom.flipTriang = true;
        }
    }

    geom.arrowWidth = geom.margin * 0.5F;

    geom.ar1 = geom.par1 + ((geom.flipTriang) ? -1 : 1) * geom.dir * 0.866F * 2 * geom.margin;
    geom.ar2 = geom.ar1 + geom.normal * geom.arrowWidth;
    geom.ar1 -= geom.normal * geom.arrowWidth;

    geom.ar3 = geom.par4 - ((geom.flipTriang) ? -1 : 1) * geom.dir * 0.866F * 2 * geom.margin;
    geom.ar4 = geom.ar3 + geom.normal * geom.arrowWidth;
    geom.ar3 -= geom.normal * geom.arrowWidth;

    return geom;
}

SoDatumLabel::DiameterGeometry SoDatumLabel::calculateDiameterGeometry(const SbVec3f* points) const
{
    DiameterGeometry geom;

    // Get the Points
    geom.p1 = points[0];
    geom.p2 = points[1];

    geom.dir = (geom.p2 - geom.p1);
    geom.center = geom.p1;
    geom.radius = (geom.p2 - geom.p1).length();
    geom.isDiameter = (this->datumtype.getValue() == DIAMETER);

    if (geom.isDiameter) {
        geom.center = (geom.p1 + geom.p2) / 2;
        geom.radius = geom.radius / 2;
    }

    geom.dir.normalize();
    geom.normal = SbVec3f(-geom.dir[1], geom.dir[0], 0);

    float length = this->param1.getValue();
    geom.pos = geom.p2 + length * geom.dir;

    // Get magnitude of angle between horizontal
    geom.angle = atan2f(geom.dir[1], geom.dir[0]);
    if (geom.angle > std::numbers::pi / 2 + std::numbers::pi / 12) {
        geom.angle -= (float)std::numbers::pi;
    }
    else if (geom.angle <= -std::numbers::pi / 2 + std::numbers::pi / 12) {
        geom.angle += (float)std::numbers::pi;
    }

    geom.textOffset = geom.pos;

    geom.margin = this->imgHeight / 3.0F;

    // Create the first arrowhead
    geom.arrowWidth = geom.margin * 0.5F;
    geom.ar0 = geom.p2;
    geom.ar1 = geom.p2 - geom.dir * 0.866F * 2 * geom.margin;
    geom.ar2 = geom.ar1 + geom.normal * geom.arrowWidth;
    geom.ar1 -= geom.normal * geom.arrowWidth;

    SbVec3f p3 = geom.pos + geom.dir * (this->imgWidth / 2 + geom.margin);
    if ((p3 - geom.p1).length() > (geom.p2 - geom.p1).length()) {
        geom.p2 = p3;
    }

    // Calculate the line segment points around text
    geom.pnt1 = geom.pos - geom.dir * (geom.margin + this->imgWidth / 2);
    geom.pnt2 = geom.pos + geom.dir * (geom.margin + this->imgWidth / 2);

    if (geom.isDiameter) {
        // Create second arrowhead for diameter
        geom.ar0_1 = geom.p1;
        geom.ar1_1 = geom.p1 + geom.dir * 0.866F * 2 * geom.margin;
        geom.ar2_1 = geom.ar1_1 + geom.normal * geom.arrowWidth;
        geom.ar1_1 -= geom.normal * geom.arrowWidth;
    }

    // Arc helper parameters
    geom.startAngle = this->param3.getValue();
    geom.startRange = this->param4.getValue();

    geom.endAngle = this->param5.getValue();
    geom.endRange = this->param6.getValue();

    return geom;
}

SoDatumLabel::AngleGeometry SoDatumLabel::calculateAngleGeometry(const SbVec3f* points) const
{
    AngleGeometry geom;

    // only the angle intersection point is needed
    geom.p0 = points[0];

    geom.margin = this->imgHeight / 3.0F;

    // load the parameters
    geom.length = this->param1.getValue();
    geom.startangle = this->param2.getValue();
    geom.range = this->param3.getValue();
    geom.endangle = geom.startangle + geom.range;
    geom.endLineLength1 = std::max(this->param4.getValue(), geom.margin);
    geom.endLineLength2 = std::max(this->param5.getValue(), geom.margin);
    geom.endLineLength12 = std::max(-this->param4.getValue(), geom.margin);
    geom.endLineLength22 = std::max(-this->param5.getValue(), geom.margin);

    geom.r = 2 * geom.length;

    // set the text label angle to zero
    geom.angle = 0.F;

    geom.v0 = getAngleMidDirection(geom.startangle, geom.range);

    // leave some space for the text
    geom.textMargin = std::min(0.2F * abs(geom.range), this->imgWidth / (2 * geom.r));

    geom.textOffset = getAngleTextCenter(geom.p0, geom.startangle, geom.range, geom.r);

    // direction vectors for start and end lines
    geom.v1 = SbVec3f(cos(geom.startangle), sin(geom.startangle), 0);
    geom.v2 = SbVec3f(cos(geom.endangle), sin(geom.endangle), 0);

    if (geom.range < 0 || geom.length < 0) {
        std::swap(geom.v1, geom.v2);
        geom.textMargin = -geom.textMargin;
    }

    geom.pnt1 = geom.p0 + (geom.r - geom.endLineLength1) * geom.v1;
    geom.pnt2 = geom.p0 + (geom.r + geom.endLineLength12) * geom.v1;
    geom.pnt3 = geom.p0 + (geom.r - geom.endLineLength2) * geom.v2;
    geom.pnt4 = geom.p0 + (geom.r + geom.endLineLength22) * geom.v2;

    // create the arrowheads
    geom.arrowLength = geom.margin * 2;
    geom.arrowWidth = geom.margin * 0.5F;

    geom.dirStart = SbVec3f(geom.v1[1], -geom.v1[0], 0);
    geom.startArrowBase = geom.p0 + geom.r * geom.v1;

    geom.dirEnd = SbVec3f(-geom.v2[1], geom.v2[0], 0);
    geom.endArrowBase = geom.p0 + geom.r * geom.v2;

    return geom;
}

SoDatumLabel::SymmetricGeometry SoDatumLabel::calculateSymmetricGeometry(const SbVec3f* points) const
{
    SymmetricGeometry geom;

    geom.p1 = points[0];
    geom.p2 = points[1];

    geom.dir = (geom.p2 - geom.p1);
    geom.dir.normalize();
    geom.normal = SbVec3f(-geom.dir[1], geom.dir[0], 0);

    geom.margin = this->imgHeight / 4.0F;

    // calculate coordinates for the first arrow
    geom.ar0 = geom.p1 + geom.dir * 4 * geom.margin;  // tip of arrow
    geom.ar1 = geom.ar0 - geom.dir * 0.866F * 2 * geom.margin;
    geom.ar2 = geom.ar1 + geom.normal * geom.margin;
    geom.ar1 -= geom.normal * geom.margin;

    // calculate coordinates for the second arrow
    geom.ar3 = geom.p2 - geom.dir * 4 * geom.margin;  // tip of 2nd arrow
    geom.ar4 = geom.ar3 + geom.dir * 0.866F * 2 * geom.margin;
    geom.ar5 = geom.ar4 + geom.normal * geom.margin;
    geom.ar4 -= geom.normal * geom.margin;

    return geom;
}

void SoDatumLabel::generateLineSelectionPrimitive(
    SoAction* action,
    const SbVec3f& start,
    const SbVec3f& end,
    float width
)
{
    // create a thicker line used for selection
    SbVec3f dir = end - start;
    dir.normalize();
    SbVec3f perp = SbVec3f(-dir[1], dir[0], 0) * (width / 2.0f);

    SbVec3f p1 = start + perp;
    SbVec3f p2 = start - perp;
    SbVec3f p3 = end + perp;
    SbVec3f p4 = end - perp;

    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLE_STRIP);
    pv.setPoint(p1);
    shapeVertex(&pv);
    pv.setPoint(p2);
    shapeVertex(&pv);
    pv.setPoint(p3);
    shapeVertex(&pv);
    pv.setPoint(p4);
    shapeVertex(&pv);
    this->endShape();
}

void SoDatumLabel::generateArcSelectionPrimitive(
    SoAction* action,
    const SbVec3f& center,
    float radius,
    float startAngle,
    float endAngle,
    float width
)
{
    // create selectable arc by generating line segments
    int countSegments = std::max(6, abs(int(50.0 * (endAngle - startAngle) / (2 * std::numbers::pi))));
    double segment = (endAngle - startAngle) / (countSegments - 1);

    for (int i = 0; i < countSegments - 1; i++) {
        double theta1 = startAngle + segment * i;
        double theta2 = startAngle + segment * (i + 1);
        SbVec3f v1 = center + SbVec3f(radius * cos(theta1), radius * sin(theta1), 0);
        SbVec3f v2 = center + SbVec3f(radius * cos(theta2), radius * sin(theta2), 0);
        generateLineSelectionPrimitive(action, v1, v2, width);
    }
}

void SoDatumLabel::generateArrowSelectionPrimitive(
    SoAction* action,
    const SbVec3f& base,
    const SbVec3f& dir,
    float width,
    float length
)
{
    // create selectable arrow as a triangle at elevated Z for selection above geometry
    SbVec3f tip = base + dir * length;
    SbVec3f perp = SbVec3f(-dir[1], dir[0], 0) * (width / 2.0f);

    SbVec3f p1 = base + perp;
    SbVec3f p2 = base - perp;

    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLES);
    pv.setPoint(SbVec3f(tip[0], tip[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(p1[0], p1[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    pv.setPoint(SbVec3f(p2[0], p2[1], ZARROW_TEXT_OFFSET));
    shapeVertex(&pv);
    this->endShape();
}

SoDatumLabel::ArcLengthGeometry SoDatumLabel::calculateArcLengthGeometry(const SbVec3f* points) const
{
    using std::numbers::pi;

    ArcLengthGeometry geom;

    geom.ctr = points[0];
    geom.p1 = points[1];
    geom.p2 = points[2];
    geom.length = this->param1.getValue();

    geom.margin = this->imgHeight / 3.0F;

    // angles calculations
    SbVec3f vc1 = (geom.p1 - geom.ctr);
    SbVec3f vc2 = (geom.p2 - geom.ctr);

    geom.startangle = atan2f(vc1[1], vc1[0]);
    geom.endangle = atan2f(vc2[1], vc2[0]);
    geom.endangle = normalizeArcSweepEnd(geom.startangle, geom.endangle);

    geom.range = geom.endangle - geom.startangle;
    geom.radius = vc1.length();

    // text orientation
    SbVec3f dir = (geom.p2 - geom.p1);
    dir.normalize();
    // get magnitude of angle between horizontal
    geom.angle = atan2f(dir[1], dir[0]);
    if (geom.angle > pi / 2 + pi / 12) {
        geom.angle -= (float)pi;
    }
    else if (geom.angle <= -pi / 2 + pi / 12) {
        geom.angle += (float)pi;
    }

    // lines direction
    geom.vm = (geom.p1 + geom.p2) / 2 - geom.ctr;
    geom.vm.normalize();

    // determine if this is a large arc (> pi)
    geom.isLargeArc = (geom.range > pi);

    // lines points
    geom.pnt1 = geom.p1;
    geom.pnt3 = geom.p2;

    if (geom.isLargeArc) {
        const float desiredRadius = std::max(geom.length, geom.radius);
        const float averageMidDirectionProjection
            = std::clamp(0.5F * ((vc1.dot(geom.vm) + vc2.dot(geom.vm)) / geom.radius), -1.0F, 1.0F);
        const float offset = -geom.radius * averageMidDirectionProjection
            + std::sqrt(
                std::max(
                    0.0F,
                    desiredRadius * desiredRadius
                        - geom.radius * geom.radius
                            * (1.0F - averageMidDirectionProjection * averageMidDirectionProjection)
                )
            );

        // recalculate angles for the outer arc
        SbVec3f vc1_outer = geom.p1 + offset * geom.vm - geom.ctr;
        SbVec3f vc2_outer = geom.p2 + offset * geom.vm - geom.ctr;
        vc1_outer.normalize();
        vc2_outer.normalize();

        geom.arcCenter = geom.ctr;
        geom.arcRadius = desiredRadius;
        geom.pnt2 = geom.arcCenter + geom.arcRadius * vc1_outer;
        geom.pnt4 = geom.arcCenter + geom.arcRadius * vc2_outer;

        // update angles for outer arc
        geom.startangle = atan2f(vc1_outer[1], vc1_outer[0]);
        geom.endangle = normalizeArcSweepEnd(geom.startangle, atan2f(vc2_outer[1], vc2_outer[0]));
        geom.range = geom.endangle - geom.startangle;
    }
    else {
        const float offset = geom.length - geom.radius;
        geom.pnt2 = geom.p1 + offset * geom.vm;
        geom.pnt4 = geom.p2 + offset * geom.vm;

        // arc center and radius for inner arc
        geom.arcCenter = geom.ctr + offset * geom.vm;
        geom.arcRadius = geom.radius;
    }

    geom.textOffset = getArcTextCenter(
        geom.arcCenter,
        geom.startangle,
        geom.endangle,
        geom.arcRadius + this->imgHeight
    );

    // normals for the arrowheads at arc start and end
    geom.dirStart = SbVec3f(sin(geom.startangle), -cos(geom.startangle), 0);
    geom.dirEnd = SbVec3f(-sin(geom.endangle), cos(geom.endangle), 0);

    return geom;
}
