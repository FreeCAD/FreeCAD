/***************************************************************************
 *   Copyright (c) 2011-2012 Luke Parry <l.parry@warwick.ac.uk>            *
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
# ifdef FC_OS_WIN32
# include <windows.h>
# undef min
# undef max
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif

# include <algorithm>
# include <cmath>
# include <limits>
# include <numbers>
# include <QFontMetrics>
# include <QPainter>

# include <Inventor/SoPrimitiveVertex.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/elements/SoFocalDistanceElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/elements/SoViewVolumeElement.h>
# include <Inventor/misc/SoState.h>
#endif // _PreComp_

#include <Base/Tools.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Tools.h>

#include "SoDatumLabel.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
constexpr const float ZCONSTR {0.006F};

using namespace Gui;

// ------------------------------------------------------


namespace {

void glVertex(const SbVec3f& pt){
    glVertex3f(pt[0], pt[1], pt[2]);
}

void glVertexes(const std::vector<SbVec3f>& pts){
    for (auto pt: pts){
        glVertex3f(pt[0], pt[1], pt[2]);
    }
}

void glDrawLine(const SbVec3f& p1, const SbVec3f& p2){
    glBegin(GL_LINES);
        glVertexes({p1, p2});
    glEnd();
}

void glDrawArc(const SbVec3f& center, float radius, float startAngle=0.,
               float endAngle=2.0*std::numbers::pi, int countSegments=0){
    float range = endAngle - startAngle;

    if (countSegments == 0){
        countSegments = std::max(6, abs(int(25.0 * range / std::numbers::pi)));
    }

    float segment = range / (countSegments-1);

    glBegin(GL_LINE_STRIP);
    for (int i=0; i < countSegments; i++) {
        float theta = startAngle + segment*i;
        SbVec3f v1 = center + radius * SbVec3f(cos(theta),sin(theta),0);
        glVertex(v1);
    }
    glEnd();
}

void glDrawArrow(const SbVec3f& base, const SbVec3f& dir, float width, float length){
    // Calculate arrowhead points
    SbVec3f normal(dir[1], -dir[0], 0);
    SbVec3f arrowLeft = base - length * dir + width * normal;
    SbVec3f arrowRight = base - length * dir - width * normal;

    // Draw arrowheads
    glBegin(GL_TRIANGLES);
    glVertexes({base, arrowLeft, arrowRight});
    glEnd();
}

} // namespace


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
    SO_NODE_ADD_FIELD(textColor, (SbVec3f(1.0F,1.0F,1.0F)));
    SO_NODE_ADD_FIELD(pnts, (SbVec3f(.0F,.0F,.0F)));
    SO_NODE_ADD_FIELD(norm, (SbVec3f(.0F,.0F,1.F)));

    SO_NODE_ADD_FIELD(name, ("Helvetica"));
    SO_NODE_ADD_FIELD(size, (10.F));
    SO_NODE_ADD_FIELD(lineWidth, (2.F));
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

    int w = Gui::QtTools::horizontalAdvance(fm, str);
    int h = fm.height();

    // No Valid text
    if (!w) {
        this->image = SoSFImage();
        return;
    }

    const SbColor& t = textColor.getValue();
    QColor front;
    front.setRgbF(t[0],t[1], t[2]);

    QImage image(w * sampling.getValue(), h * sampling.getValue(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(sampling.getValue());
    image.fill(0x00000000);

    QPainter painter(&image);
    if (useAntialiasing) {
        painter.setRenderHint(QPainter::Antialiasing);
    }

    painter.setPen(front);
    painter.setFont(font);
    painter.drawText(0, 0, w, h, Qt::AlignLeft, str);
    painter.end();

    Gui::BitmapFactory().convert(image, this->image);
}

namespace Gui {
// helper class to determine the bounding box of a datum label
class DatumLabelBox
{
public:
    DatumLabelBox(float scale, SoDatumLabel* label)
        : scale{scale}
        , label{label}
    {

    }
    void computeBBox(SbBox3f& box, SbVec3f& center) const
    {
        std::vector<SbVec3f> corners;
        if (label->datumtype.getValue() == SoDatumLabel::DISTANCE ||
            label->datumtype.getValue() == SoDatumLabel::DISTANCEX ||
            label->datumtype.getValue() == SoDatumLabel::DISTANCEY ) {
            corners = computeDistanceBBox();
        }
        else if (label->datumtype.getValue() == SoDatumLabel::RADIUS ||
                 label->datumtype.getValue() == SoDatumLabel::DIAMETER) {
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
            box.setBounds(SbVec3f(minX, minY, 0.0F), SbVec3f (maxX, maxY, 0.0F));
            center = box.getCenter();
        }
    }
    std::vector<SbVec3f> computeDistanceBBox() const
    {
        SbVec2s imgsize;
        int nc {};
        int srcw = 1;
        int srch = 1;

        const unsigned char * dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio =  (float) srcw / (float) srch;
        float imgHeight = scale * (float) (srch);
        float imgWidth  = aspectRatio * imgHeight;

        // get the points stored in the pnt field
        const SbVec3f *points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 2) {
            return {};
        }

        // use the shared geometry calculation for consistency
        SoDatumLabel::DistanceGeometry geom = label->calculateDistanceGeometry(points, scale, srch);

        std::vector<SbVec3f> corners;
        float margin = imgHeight / 4.0F;

        // include main points and extension line endpoints
        corners.push_back(geom.p1);
        corners.push_back(geom.p2);
        corners.push_back(geom.perp1);
        corners.push_back(geom.perp2);

        // include text label area
        corners.push_back(geom.textOffset + geom.dir * (imgWidth / 2.0F + margin) + geom.normal * (srch + margin));
        corners.push_back(geom.textOffset - geom.dir * (imgWidth / 2.0F + margin) + geom.normal * (srch + margin));
        corners.push_back(geom.textOffset + geom.dir * (imgWidth / 2.0F + margin) - geom.normal * margin);
        corners.push_back(geom.textOffset - geom.dir * (imgWidth / 2.0F + margin) - geom.normal * margin);

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
        SbVec2s imgsize;
        int nc {};
        int srcw = 1;
        int srch = 1;

        const unsigned char * dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio =  (float) srcw / (float) srch;
        float imgHeight = scale * (float) (srch);
        float imgWidth  = aspectRatio * imgHeight;

        // get the points stored in the pnt field
        const SbVec3f *points = label->pnts.getValues(0);
        if (label->pnts.getNum() < 2) {
            return {};
        }

        // use the shared geometry calculation for consistency
        SoDatumLabel::DiameterGeometry geom = label->calculateDiameterGeometry(points);

        std::vector<SbVec3f> corners;
        float margin = imgHeight / 4.0F;

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
                    SbVec3f arcPoint = geom.center + SbVec3f(geom.radius * cos(angle), geom.radius * sin(angle), 0);
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

        const unsigned char * dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio =  (float) srcw / (float) srch;
        float imgHeight = scale * (float) (srch);
        float imgWidth  = aspectRatio * imgHeight;

        // get the points stored in the pnt field
        const SbVec3f *points = label->pnts.getValues(0);
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
        SbVec3f img2 = SbVec3f(-imgWidth / 2.0F,  imgHeight / 2, 0.0F);
        SbVec3f img3 = SbVec3f( imgWidth / 2.0F, -imgHeight / 2, 0.0F);
        SbVec3f img4 = SbVec3f( imgWidth / 2.0F,  imgHeight / 2, 0.0F);

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
        const SbVec3f *points = label->pnts.getValues(0);
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
        const SbVec3f *points = label->pnts.getValues(0);
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

        const unsigned char * dataptr = label->image.getValue(imgsize, nc);
        if (dataptr) {
            srcw = imgsize[0];
            srch = imgsize[1];
        }

        float aspectRatio =  (float) srcw / (float) srch;
        float imgHeight = scale * (float) (srch);
        float imgWidth  = aspectRatio * imgHeight;

        // text orientation
        SbVec3f dir = (geom.p2 - geom.p1);
        dir.normalize();
        SbVec3f normal = SbVec3f (-dir[1], dir[0], 0);

        // include all visual elements in bounding box
        std::vector<SbVec3f> corners;

        // text area (existing coverage)
        float margin = imgHeight / 4.0F;
        corners.push_back(geom.textOffset + dir * (imgWidth / 2.0F + margin) - normal * (imgHeight / 2.0F + margin));
        corners.push_back(geom.textOffset - dir * (imgWidth / 2.0F + margin) - normal * (imgHeight / 2.0F + margin));
        corners.push_back(geom.textOffset + dir * (imgWidth / 2.0F + margin) + normal * (imgHeight / 2.0F + margin));
        corners.push_back(geom.textOffset - dir * (imgWidth / 2.0F + margin) + normal * (imgHeight / 2.0F + margin));

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
            SbVec3f arcPoint = geom.arcCenter + SbVec3f(geom.arcRadius * cos(angle), geom.arcRadius * sin(angle), 0);
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
} // namespace Gui

void SoDatumLabel::computeBBox(SoAction * action, SbBox3f &box, SbVec3f &center)
{
    SoState *state = action->getState();
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

    if (datumtype.getValue() == SoDatumLabel::DISTANCE ||
        datumtype.getValue() == SoDatumLabel::DISTANCEX ||
        datumtype.getValue() == SoDatumLabel::DISTANCEY) {
        return getLabelTextCenterDistance(p1, p2);
    }
    if (datumtype.getValue() == SoDatumLabel::RADIUS ||
        datumtype.getValue() == SoDatumLabel::DIAMETER) {
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

    // Useful Information
    // v0 - vector for text position
    // p0 - vector for angle intersect
    SbVec3f v0(cos(startangle + range / 2), sin(startangle + range / 2), 0);

    SbVec3f textCenter = p0 + v0 * len2;
    return textCenter;
}

SbVec3f SoDatumLabel::getLabelTextCenterArcLength(const SbVec3f& ctr, const SbVec3f& p1, const SbVec3f& p2) const
{
    float length = this->param1.getValue();

    // Angles calculations
    SbVec3f vc1 = (p1 - ctr);
    SbVec3f vc2 = (p2 - ctr);

    float startangle = atan2f(vc1[1], vc1[0]);
    float endangle = atan2f(vc2[1], vc2[0]);

    if (endangle < startangle) {
        endangle += 2.F * std::numbers::pi_v<float>;
    }

    // Text location
    SbVec3f vm = (p1+p2)/2 - ctr;
    vm.normalize();

    SbVec3f textCenter;
    if (endangle - startangle <= std::numbers::pi) {
        textCenter = ctr + vm * (length + this->imgHeight);
    } else {
        textCenter = ctr - vm * (length + 2. * this->imgHeight);
    }
    return textCenter;
}


void SoDatumLabel::generateDistancePrimitives(SoAction * action, const SbVec3f& p1, const SbVec3f& p2)
{
    SbVec3f points[2] = {p1, p2};
    float scale = 1.0f;
    int srch = 1;
    DistanceGeometry geom = calculateDistanceGeometry(points, scale, srch);

    // generate selectable primitive for txt label
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2,  this->imgHeight / 2, 0.F);
    SbVec3f img3 = SbVec3f( this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img4 = SbVec3f( this->imgWidth / 2,  this->imgHeight / 2, 0.F);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), 0.F);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), 0.F);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), 0.F);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), 0.F);

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
    float lineWidth = geom.margin * 0.8f; // adjust the width for selection

    // ext lines
    generateLineSelectionPrimitive(action, geom.p1, geom.perp1, lineWidth);
    generateLineSelectionPrimitive(action, geom.p2, geom.perp2, lineWidth);

    // dim lines
    generateLineSelectionPrimitive(action, geom.par1, geom.par2, lineWidth);
    generateLineSelectionPrimitive(action, geom.par3, geom.par4, lineWidth);

    // begin generation of selectable primitives for arrow-heads
    this->beginShape(action, TRIANGLES);
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    // 1st arrow-head
    pv.setPoint(geom.par1);
    shapeVertex(&pv);
    pv.setPoint(geom.ar1);
    shapeVertex(&pv);
    pv.setPoint(geom.ar2);
    shapeVertex(&pv);

    // 2nd arrow-head
    pv.setPoint(geom.par4);
    shapeVertex(&pv);
    pv.setPoint(geom.ar3);
    shapeVertex(&pv);
    pv.setPoint(geom.ar4);
    shapeVertex(&pv);

    this->endShape();
}

void SoDatumLabel::generateDiameterPrimitives(SoAction * action, const SbVec3f& p1, const SbVec3f& p2)
{
    SbVec3f points[2] = {p1, p2};
    DiameterGeometry geom = calculateDiameterGeometry(points);

    // generate selectable primitive for text label
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2,  this->imgHeight / 2, 0.F);
    SbVec3f img3 = SbVec3f( this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img4 = SbVec3f( this->imgWidth / 2,  this->imgHeight / 2, 0.F);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), 0.F);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), 0.F);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), 0.F);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), 0.F);

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

    // Generate selectable primitives for arrow heads
    this->beginShape(action, TRIANGLES);
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    // first arrow-head
    pv.setPoint(geom.ar0);
    shapeVertex(&pv);
    pv.setPoint(geom.ar1);
    shapeVertex(&pv);
    pv.setPoint(geom.ar2);
    shapeVertex(&pv);

    // second arrow-head but only for diameter
    if (geom.isDiameter) {
        pv.setPoint(geom.ar0_1);
        shapeVertex(&pv);
        pv.setPoint(geom.ar1_1);
        shapeVertex(&pv);
        pv.setPoint(geom.ar2_1);
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
                SbVec3f v1 = geom.center + SbVec3f(geom.radius * cos(theta1), geom.radius * sin(theta1), 0);
                SbVec3f v2 = geom.center + SbVec3f(geom.radius * cos(theta2), geom.radius * sin(theta2), 0);
                generateLineSelectionPrimitive(action, v1, v2, lineWidth * 0.5f);
            }
        }
    };

    generateSelectablePrimitiveForArcHelper(geom.startAngle, geom.startRange);
    generateSelectablePrimitiveForArcHelper(geom.endAngle, geom.endRange);
}

void SoDatumLabel::generateAnglePrimitives(SoAction * action, const SbVec3f& p0)
{
    // use shared geometry calculation
    SbVec3f points[1] = {p0};
    AngleGeometry geom = calculateAngleGeometry(points);

    // generate selectable primitive for text label
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2,  this->imgHeight / 2, 0.F);
    SbVec3f img3 = SbVec3f( this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img4 = SbVec3f( this->imgWidth / 2,  this->imgHeight / 2, 0.F);

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
    generateArcSelectionPrimitive(action, geom.p0, geom.r, geom.startangle, geom.startangle + geom.range / 2.0 - geom.textMargin, arcWidth);

    // arc after text
    generateArcSelectionPrimitive(action, geom.p0, geom.r, geom.startangle + geom.range / 2.0 + geom.textMargin, geom.endangle, arcWidth);

    // generate selectable primitives for arrow heads
    generateArrowSelectionPrimitive(action, geom.startArrowBase, geom.dirStart, geom.arrowWidth, geom.arrowLength);
    generateArrowSelectionPrimitive(action, geom.endArrowBase, geom.dirEnd, geom.arrowWidth, geom.arrowLength);
}

void SoDatumLabel::generateSymmetricPrimitives(SoAction * action, const SbVec3f& p1, const SbVec3f& p2)
{
    // use shared geometry calculation
    SbVec3f points[2] = {p1, p2};
    SymmetricGeometry geom = calculateSymmetricGeometry(points);

    // generate selectable primitives for lines
    float lineWidth = geom.margin * 0.8f;

    // lines from endpoints to arrow tips
    generateLineSelectionPrimitive(action, geom.p1, geom.ar0, lineWidth);
    generateLineSelectionPrimitive(action, geom.p2, geom.ar3, lineWidth);

    // generate selectable primitives for arrow heads as triangles
    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLES);

    // first arrow
    pv.setPoint(geom.ar0);
    shapeVertex(&pv);
    pv.setPoint(geom.ar1);
    shapeVertex(&pv);
    pv.setPoint(geom.ar2);
    shapeVertex(&pv);

    // second arrow
    pv.setPoint(geom.ar3);
    shapeVertex(&pv);
    pv.setPoint(geom.ar4);
    shapeVertex(&pv);
    pv.setPoint(geom.ar5);
    shapeVertex(&pv);

    this->endShape();
}

void SoDatumLabel::generateArcLengthPrimitives(SoAction * action, const SbVec3f& ctr, const SbVec3f& p1, const SbVec3f& p2)
{
    // use shared geometry calculation
    SbVec3f points[3] = {ctr, p1, p2};
    ArcLengthGeometry geom = calculateArcLengthGeometry(points);

    // generate selectable primitive for text label
    SbVec3f img1 = SbVec3f(-this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img2 = SbVec3f(-this->imgWidth / 2,  this->imgHeight / 2, 0.F);
    SbVec3f img3 = SbVec3f( this->imgWidth / 2, -this->imgHeight / 2, 0.F);
    SbVec3f img4 = SbVec3f( this->imgWidth / 2,  this->imgHeight / 2, 0.F);

    float s = sin(geom.angle);
    float c = cos(geom.angle);

    img1 = SbVec3f((img1[0] * c) - (img1[1] * s), (img1[0] * s) + (img1[1] * c), 0.F);
    img2 = SbVec3f((img2[0] * c) - (img2[1] * s), (img2[0] * s) + (img2[1] * c), 0.F);
    img3 = SbVec3f((img3[0] * c) - (img3[1] * s), (img3[0] * s) + (img3[1] * c), 0.F);
    img4 = SbVec3f((img4[0] * c) - (img4[1] * s), (img4[0] * s) + (img4[1] * c), 0.F);

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
    generateArcSelectionPrimitive(action, geom.arcCenter, geom.arcRadius, geom.startangle, geom.endangle, lineWidth);

    // generate selectable primitives for arrow heads
    float arrowLength = geom.margin * 2;
    float arrowWidth = geom.margin * 0.5F;

    generateArrowSelectionPrimitive(action, geom.pnt2, geom.dirStart, arrowWidth, arrowLength);
    generateArrowSelectionPrimitive(action, geom.pnt4, geom.dirEnd, arrowWidth, arrowLength);
}

void SoDatumLabel::generatePrimitives(SoAction * action)
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
    const SbVec3f *points = this->pnts.getValues(0);
    SbVec3f p1 = points[0];
    SbVec3f p2 = points[1];

    // Change the offset and bounding box parameters depending on Datum Type
    if (this->datumtype.getValue() == DISTANCE ||
        this->datumtype.getValue() == DISTANCEX ||
        this->datumtype.getValue() == DISTANCEY) {

        generateDistancePrimitives(action, p1, p2);
    }
    else if (this->datumtype.getValue() == RADIUS ||
             this->datumtype.getValue() == DIAMETER) {

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

void SoDatumLabel::notify(SoNotList * l)
{
    SoField * f = l->getLastField();
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
    * implementation internals. The factor returned from this function is calculated from the view frustums
    * nearplane width, height is not taken into account, and hence we divide it with the viewport width
    * to get the exact pixel scale factor.
    * This is not documented and therefore may change on later coin versions!
    */
    const SbViewVolume & vv = SoViewVolumeElement::get(state);
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
    const SbViewportRegion & vp = SoViewportRegionElement::get(state);
    SbVec2s vp_size = vp.getViewportSizePixels();
    scale /= float(vp_size[0]);

    return scale;
}

void SoDatumLabel::GLRender(SoGLRenderAction * action)
{
    SoState *state = action->getState();

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
        this->imgHeight = scale*25.0F;
        this->imgWidth = scale*25.0F;
    }

    // Get the points stored in the pnt field
    const SbVec3f *points = this->pnts.getValues(0);

    state->push();

    //Set General OpenGL Properties
    glPushAttrib(GL_ENABLE_BIT | GL_PIXEL_MODE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    //Enable Anti-alias
    if (action->isSmoothing()) {
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    }

    // Position for Datum Text Label
    float angle = 0;

    // Get the colour
    const SbColor& t = textColor.getValue();

    // Set GL Properties
    glLineWidth(this->lineWidth.getValue());
    glColor3f(t[0], t[1], t[2]);

    SbVec3f textOffset;

    if (this->datumtype.getValue() == DISTANCE ||
        this->datumtype.getValue() == DISTANCEX ||
        this->datumtype.getValue() == DISTANCEY ) {
        drawDistance(points, scale, srch, angle, textOffset);
    }
    else if (this->datumtype.getValue() == RADIUS || this->datumtype.getValue() == DIAMETER) {
        drawRadiusOrDiameter(points, angle, textOffset);
    }
    else if (this->datumtype.getValue() == ANGLE) {
        drawAngle(points, angle, textOffset);
    }
    else if (this->datumtype.getValue() == SYMMETRIC) {
        drawSymmetric(points);
    }
    else if (this->datumtype.getValue() == ARCLENGTH) {
        drawArcLength(points, angle, textOffset);
    }

    if (hasText) {
        drawText(state, srcw, srch, angle, textOffset);
    }

    glPopAttrib();
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

    const unsigned char * dataptr = this->image.getValue(imgsize, nc);
    if (!dataptr) { // no image
        return;
    }

    srcw = imgsize[0];
    srch = imgsize[1];

    float aspectRatio =  (float) srcw / (float) srch;
    this->imgHeight = scale * (float) (srch) / sampling.getValue();
    this->imgWidth  = aspectRatio * (float) this->imgHeight;
}

void SoDatumLabel::drawDistance(const SbVec3f* points, float scale, int srch, float& angle, SbVec3f& textOffset)
{
    SoDatumLabel::DistanceGeometry geom = this->calculateDistanceGeometry(points, scale, srch);

    angle = geom.angle;
    textOffset = geom.textOffset;

    // Get the colour
    const SbColor& t = textColor.getValue();

    // Set GL Properties
    glLineWidth(this->lineWidth.getValue());
    glColor3f(t[0], t[1], t[2]);

    // Perp Lines
    glBegin(GL_LINES);
        if (this->param1.getValue() != 0.) {
            glVertex2f(geom.p1[0], geom.p1[1]);
            glVertex2f(geom.perp1[0], geom.perp1[1]);

            glVertex2f(geom.p2[0], geom.p2[1]);
            glVertex2f(geom.perp2[0], geom.perp2[1]);
        }

        glVertex2f(geom.par1[0], geom.par1[1]);
        glVertex2f(geom.par2[0], geom.par2[1]);

        glVertex2f(geom.par3[0], geom.par3[1]);
        glVertex2f(geom.par4[0], geom.par4[1]);
    glEnd();

    // Draw the arrowheads
    glBegin(GL_TRIANGLES);
        glVertex2f(geom.par1[0], geom.par1[1]);
        glVertex2f(geom.ar1[0], geom.ar1[1]);
        glVertex2f(geom.ar2[0], geom.ar2[1]);

        glVertex2f(geom.par4[0], geom.par4[1]);
        glVertex2f(geom.ar3[0], geom.ar3[1]);
        glVertex2f(geom.ar4[0], geom.ar4[1]);
    glEnd();

    if (this->datumtype.getValue() == DISTANCE) {
        drawDistance(points);
    }
}

void SoDatumLabel::drawDistance(const SbVec3f* points)
{
    // Draw arc helpers if needed
    float range1 = this->param4.getValue();
    if (range1 != 0.0) {
        float startAngle1 = this->param3.getValue();
        float radius1 = this->param5.getValue();
        SbVec3f center1 = points[2];
        glDrawArc(center1, radius1, startAngle1, startAngle1 + range1);
    }

    float range2 = this->param7.getValue();
    if (range2 != 0.0) {
        float startAngle2 = this->param6.getValue();
        float radius2 = this->param8.getValue();
        SbVec3f center2 = points[3];
        glDrawArc(center2, radius2, startAngle2, startAngle2 + range2);
    }
}

void SoDatumLabel::drawRadiusOrDiameter(const SbVec3f* points, float& angle, SbVec3f& textOffset)
{
    // Use shared geometry calculation
    DiameterGeometry geom = calculateDiameterGeometry(points);

    angle = geom.angle;
    textOffset = geom.textOffset;

    // Draw the Lines
    glBegin(GL_LINES);
        glVertex2f(geom.p1[0], geom.p1[1]);
        glVertex2f(geom.pnt1[0], geom.pnt1[1]);

        glVertex2f(geom.pnt2[0], geom.pnt2[1]);
        glVertex2f(geom.p2[0], geom.p2[1]);
    glEnd();

    glBegin(GL_TRIANGLES);
        glVertex2f(geom.ar0[0], geom.ar0[1]);
        glVertex2f(geom.ar1[0], geom.ar1[1]);
        glVertex2f(geom.ar2[0], geom.ar2[1]);
    glEnd();

    if (geom.isDiameter) {
        // Draw second arrowhead
        glBegin(GL_TRIANGLES);
            glVertex2f(geom.ar0_1[0], geom.ar0_1[1]);
            glVertex2f(geom.ar1_1[0], geom.ar1_1[1]);
            glVertex2f(geom.ar2_1[0], geom.ar2_1[1]);
        glEnd();
    }

    // Draw arc helpers if needed
    if (geom.startRange != 0.0) {
        glDrawArc(geom.center, geom.radius, geom.startAngle, geom.startAngle + geom.startRange);
    }

    if (geom.endRange != 0.0) {
        glDrawArc(geom.center, geom.radius, geom.endAngle, geom.endAngle + geom.endRange);
    }
}

void SoDatumLabel::drawAngle(const SbVec3f* points, float& angle, SbVec3f& textOffset)
{
    // use shared geometry calculation
    AngleGeometry geom = calculateAngleGeometry(points);

    angle = geom.angle;
    textOffset = geom.textOffset;

    // draw arc segments
    glDrawArc(geom.p0, geom.r, geom.startangle, geom.startangle + geom.range / 2.0 - geom.textMargin);
    glDrawArc(geom.p0, geom.r, geom.startangle + geom.range / 2.0 + geom.textMargin, geom.endangle);

    // draw extension lines
    glDrawLine(geom.pnt1, geom.pnt2);
    glDrawLine(geom.pnt3, geom.pnt4);

    // draw arrowheads
    glDrawArrow(geom.startArrowBase, geom.dirStart, geom.arrowWidth, geom.arrowLength);
    glDrawArrow(geom.endArrowBase, geom.dirEnd, geom.arrowWidth, geom.arrowLength);
}

void SoDatumLabel::drawSymmetric(const SbVec3f* points)
{
    // use shared geometry calculation
    SymmetricGeometry geom = calculateSymmetricGeometry(points);

    // draw first arrow
    glBegin(GL_LINES);
        glVertex3f(geom.p1[0], geom.p1[1], ZCONSTR);
        glVertex3f(geom.ar0[0], geom.ar0[1], ZCONSTR);
        glVertex3f(geom.ar0[0], geom.ar0[1], ZCONSTR);
        glVertex3f(geom.ar1[0], geom.ar1[1], ZCONSTR);
        glVertex3f(geom.ar0[0], geom.ar0[1], ZCONSTR);
        glVertex3f(geom.ar2[0], geom.ar2[1], ZCONSTR);
    glEnd();

    // draw second arrow
    glBegin(GL_LINES);
        glVertex3f(geom.p2[0], geom.p2[1], ZCONSTR);
        glVertex3f(geom.ar3[0], geom.ar3[1], ZCONSTR);
        glVertex3f(geom.ar3[0], geom.ar3[1], ZCONSTR);
        glVertex3f(geom.ar4[0], geom.ar4[1], ZCONSTR);
        glVertex3f(geom.ar3[0], geom.ar3[1], ZCONSTR);
        glVertex3f(geom.ar5[0], geom.ar5[1], ZCONSTR);
    glEnd();
}

void SoDatumLabel::drawArcLength(const SbVec3f* points, float& angle, SbVec3f& textOffset)
{
    // use shared geometry calculation
    ArcLengthGeometry geom = calculateArcLengthGeometry(points);

    // set output parameters
    angle = geom.angle;
    textOffset = geom.textOffset;

    // draw arc
    glDrawArc(geom.arcCenter, geom.arcRadius, geom.startangle, geom.endangle);

    // draw lines
    glDrawLine(geom.pnt1, geom.pnt2);
    glDrawLine(geom.pnt3, geom.pnt4);

    // create the arrowheads
    float arrowLength = geom.margin * 2;
    float arrowWidth = geom.margin * 0.5F;

    glDrawArrow(geom.pnt2, geom.dirStart, arrowWidth, arrowLength);
    glDrawArrow(geom.pnt4, geom.dirEnd, arrowWidth, arrowLength);
}

// NOLINTNEXTLINE
void SoDatumLabel::drawText(SoState *state, int srcw, int srch, float angle, const SbVec3f& textOffset)
{
    SbVec2s imgsize;
    int  nc {};
    const unsigned char * dataptr = this->image.getValue(imgsize, nc);

    //Get the camera z-direction
    const SbViewVolume & vv = SoViewVolumeElement::get(state);
    SbVec3f z = vv.zVector();

    bool flip = norm.getValue().dot(z) > std::numeric_limits<float>::epsilon();

    static bool init = false;
    static bool npot = false;
    if (!init) {
        init = true;
        std::string ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));  // NOLINT
        npot = (ext.find("GL_ARB_texture_non_power_of_two") != std::string::npos);
    }

    int w = srcw;
    int h = srch;
    if (!npot) {
        // make power of two
        if ((w & (w-1)) != 0) {
            int i=1;
            while (i < 8) {
                if ((w >> i) == 0) {
                    break;
                }
                i++;
            }
            w = (1 << i);
        }
        // make power of two
        if ((h & (h-1)) != 0) {
            int i=1;
            while (i < 8) {
                if ((h >> i) == 0) {
                    break;
                }
                i++;
            }
            h = (1 << i);
        }
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D); // Enable Textures
    glEnable(GL_BLEND);

    // glGenTextures/glBindTexture was commented out but it must be active, see:
    // #0000971: Tracing over a background image in Sketcher: image is overwritten by first dimensional constraint text
    // #0001185: Planer image changes to number graphic when a part design constraint is made after the planar image
    //
    // Copy the text bitmap into memory and bind
    GLuint myTexture {};
    // generate a texture
    glGenTextures(1, &myTexture);
    glBindTexture(GL_TEXTURE_2D, myTexture);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if (!npot) {
        QImage imagedata(w, h,QImage::Format_ARGB32_Premultiplied);
        imagedata.fill(0x00000000);
        int sx = (w - srcw)/2;
        int sy = (h - srch)/2;
        glTexImage2D(GL_TEXTURE_2D, 0, nc, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*)imagedata.bits());
        glTexSubImage2D(GL_TEXTURE_2D, 0, sx, sy, srcw, srch, GL_RGBA, GL_UNSIGNED_BYTE,(const GLvoid*)  dataptr);
    }
    else {
        glTexImage2D(GL_TEXTURE_2D, 0, nc, srcw, srch, 0, GL_RGBA, GL_UNSIGNED_BYTE,(const GLvoid*)  dataptr);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Apply a rotation and translation matrix
    glTranslatef(textOffset[0], textOffset[1], textOffset[2]);
    glRotatef(Base::toDegrees<GLfloat>(angle), 0,0,1);
    glBegin(GL_QUADS);

    glColor3f(1.F, 1.F, 1.F);

    glTexCoord2f(flip ? 0.F : 1.F, 1.F); glVertex2f( -this->imgWidth / 2,  this->imgHeight / 2);
    glTexCoord2f(flip ? 0.F : 1.F, 0.F); glVertex2f( -this->imgWidth / 2, -this->imgHeight / 2);
    glTexCoord2f(flip ? 1.F : 0.F, 0.F); glVertex2f( this->imgWidth / 2, -this->imgHeight / 2);
    glTexCoord2f(flip ? 1.F : 0.F, 1.F); glVertex2f( this->imgWidth / 2,  this->imgHeight / 2);

    glEnd();

    // Reset the Mode
    glPopMatrix();

    // wmayer: see bug report below which is caused by generating but not
    // deleting the texture.
    // #0000721: massive memory leak when dragging an unconstrained model
    glDeleteTextures(1, &myTexture);
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

SoDatumLabel::DistanceGeometry SoDatumLabel::calculateDistanceGeometry(const SbVec3f* points, float scale, int srch) const
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
    } else if (this->datumtype.getValue() == DISTANCEX) {
        geom.dir = SbVec3f((geom.p2[0] - geom.p1[0] >= floatEpsilon) ? 1 : -1, 0, 0);
    } else if (this->datumtype.getValue() == DISTANCEY) {
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

    float offset1 = ((length + normproj12 < 0) ? -1.F : 1.F) * srch;
    float offset2 = ((length < 0) ? -1 : 1) * srch;

    // Get magnitude of angle between horizontal
    geom.angle = atan2f(geom.dir[1], geom.dir[0]);
    if (geom.angle > pi/2 + pi/12) {
        geom.angle -= (float)pi;
    } else if (geom.angle <= -pi/2 + pi/12) {
        geom.angle += (float)pi;
    }

    geom.textOffset = geom.midpos + geom.normal * length + geom.dir * length2;

    geom.margin = this->imgHeight / 3.0F;

    geom.perp1 = p1_ + geom.normal * (length + offset1 * scale);
    geom.perp2 = geom.p2 + geom.normal * (length + offset2 * scale);

    // Calculate the coordinates for the parallel datum lines
    geom.par1 = p1_ + geom.normal * length;
    geom.par2 = geom.midpos + geom.normal * length + geom.dir * (length2 - this->imgWidth / 2 - geom.margin);
    geom.par3 = geom.midpos + geom.normal * length + geom.dir * (length2 + this->imgWidth / 2 + geom.margin);
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
        if((geom.par3 - geom.par1).dot(geom.dir) < 0.F) {
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
    if (geom.angle > std::numbers::pi/2 + std::numbers::pi/12) {
        geom.angle -= (float)std::numbers::pi;
    } else if (geom.angle <= -std::numbers::pi/2 + std::numbers::pi/12) {
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

    // useful information
    // v0 - vector for text position
    // p0 - vector for angle intersect
    geom.v0 = SbVec3f(cos(geom.startangle + geom.range / 2), sin(geom.startangle + geom.range / 2), 0);

    // leave some space for the text
    geom.textMargin = std::min(0.2F * abs(geom.range), this->imgWidth / (2 * geom.r));

    geom.textOffset = geom.p0 + geom.v0 * geom.r;

    // direction vectors for start and end lines
    geom.v1 = SbVec3f(cos(geom.startangle), sin(geom.startangle), 0);
    geom.v2 = SbVec3f(cos(geom.endangle), sin(geom.endangle), 0);

    if (geom.range < 0) {
        std::swap(geom.v1, geom.v2);
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
    geom.ar0 = geom.p1 + geom.dir * 4 * geom.margin; // tip of arrow
    geom.ar1 = geom.ar0 - geom.dir * 0.866F * 2 * geom.margin;
    geom.ar2 = geom.ar1 + geom.normal * geom.margin;
    geom.ar1 -= geom.normal * geom.margin;

    // calculate coordinates for the second arrow
    geom.ar3 = geom.p2 - geom.dir * 4 * geom.margin; // tip of 2nd arrow
    geom.ar4 = geom.ar3 + geom.dir * 0.866F * 2 * geom.margin;
    geom.ar5 = geom.ar4 + geom.normal * geom.margin;
    geom.ar4 -= geom.normal * geom.margin;

    return geom;
}

void SoDatumLabel::generateLineSelectionPrimitive(SoAction* action, const SbVec3f& start, const SbVec3f& end, float width)
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

void SoDatumLabel::generateArcSelectionPrimitive(SoAction* action, const SbVec3f& center, float radius, float startAngle, float endAngle, float width)
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

void SoDatumLabel::generateArrowSelectionPrimitive(SoAction* action, const SbVec3f& base, const SbVec3f& dir, float width, float length)
{
    // create selectable arrow as a triangle
    SbVec3f tip = base + dir * length;
    SbVec3f perp = SbVec3f(-dir[1], dir[0], 0) * (width / 2.0f);

    SbVec3f p1 = base + perp;
    SbVec3f p2 = base - perp;

    SoPrimitiveVertex pv;
    pv.setNormal(SbVec3f(0.F, 0.F, 1.F));

    this->beginShape(action, TRIANGLES);
    pv.setPoint(tip);
    shapeVertex(&pv);
    pv.setPoint(p1);
    shapeVertex(&pv);
    pv.setPoint(p2);
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
    if (geom.endangle < geom.startangle) {
        geom.endangle += 2.0F * (float)pi;
    }

    geom.range = geom.endangle - geom.startangle;
    geom.radius = vc1.length();

    // text orientation
    SbVec3f dir = (geom.p2 - geom.p1);
    dir.normalize();
    // get magnitude of angle between horizontal
    geom.angle = atan2f(dir[1],dir[0]);
    if (geom.angle > pi/2 + pi/12) {
        geom.angle -= (float)pi;
    } else if (geom.angle <= -pi/2 + pi/12) {
        geom.angle += (float)pi;
    }

    // text location
    geom.textOffset = getLabelTextCenterArcLength(geom.ctr, geom.p1, geom.p2);

    // lines direction
    geom.vm = (geom.p1+geom.p2)/2 - geom.ctr;
    geom.vm.normalize();

    // determine if this is a large arc (> pi)
    geom.isLargeArc = (geom.range > pi);

    // lines points
    geom.pnt1 = geom.p1;
    geom.pnt3 = geom.p2;

    if (geom.isLargeArc) {
        geom.pnt2 = geom.p1 + geom.length * geom.vm;
        geom.pnt4 = geom.p2 + geom.length * geom.vm;

        // recalculate angles for the outer arc
        SbVec3f vc1_outer = (geom.pnt2 - geom.ctr);
        SbVec3f vc2_outer = (geom.pnt4 - geom.ctr);
        geom.arcCenter = geom.ctr;
        geom.arcRadius = vc1_outer.length();
        // update angles for outer arc
        geom.startangle = atan2f(vc1_outer[1], vc1_outer[0]);
        geom.endangle = atan2f(vc2_outer[1], vc2_outer[0]);
    } else {
        geom.pnt2 = geom.p1 + (geom.length - geom.radius) * geom.vm;
        geom.pnt4 = geom.p2 + (geom.length - geom.radius) * geom.vm;

        // arc center and radius for inner arc
        geom.arcCenter = geom.ctr + (geom.length - geom.radius) * geom.vm;
        geom.arcRadius = geom.radius;
    }

    // normals for the arrowheads at arc start and end
    geom.dirStart = SbVec3f(sin(geom.startangle), -cos(geom.startangle), 0);
    geom.dirEnd = SbVec3f(-sin(geom.endangle), cos(geom.endangle), 0);

    return geom;
}
