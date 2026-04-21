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

#pragma once

#include <Inventor/SbBox3f.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/nodes/SoShape.h>

#include <FCGlobal.h>


namespace Gui
{

class GuiExport SoDatumLabel: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoDatumLabel);

    friend class DatumLabelBox;

public:
    enum Type
    {
        ANGLE,
        DISTANCE,
        DISTANCEX,
        DISTANCEY,
        RADIUS,
        DIAMETER,
        SYMMETRIC,
        ARCLENGTH
    };

    static void initClass();
    SoDatumLabel();

    /*The points have to be on XY plane, ie they need to be 2D points.
    To draw on other planes, you need to attach a SoTransform to the SoDatumLabel (or parent).*/
    void setPoints(SbVec3f p1, SbVec3f p2);

    /* returns the center point of the text of the label */
    SbVec3f getLabelTextCenter();

    SoMFString string;
    SoSFColor textColor;
    SoSFEnum datumtype;
    SoSFName name;
    SoSFInt32 size;
    SoSFFloat param1;
    SoSFFloat param2;
    SoSFFloat param3;
    SoSFFloat param4;
    SoSFFloat param5;
    SoSFFloat param6;
    SoSFFloat param7;
    SoSFFloat param8;
    SoMFVec3f pnts;
    SoSFVec3f norm;
    SoSFBool strikethrough;
    SoSFImage image;
    SoSFFloat lineWidth;
    SoSFFloat sampling;
    bool useAntialiasing;

protected:
    ~SoDatumLabel() override = default;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction*, SbBox3f& box, SbVec3f& center) override;
    void generatePrimitives(SoAction* action) override;
    void notify(SoNotList* l) override;

private:
    struct DistanceGeometry
    {
        SbVec3f p1, p2;                  // main points used for measurement
        SbVec3f dir, normal;             // dir and normal vecs
        SbVec3f midpos;                  // mid pt
        SbVec3f perp1, perp2;            // ext line endpts
        SbVec3f par1, par2, par3, par4;  // dim line pts
        SbVec3f ar1, ar2;                // 1st arrow head triang pts
        SbVec3f ar3, ar4;                // 2nd arrow head triang pts
        float angle;                     // text angle
        SbVec3f textOffset;              // text pos
        bool flipTriang;                 // check for arrow flipping
        float margin;                    // margin for arrow calcs
        float arrowWidth;                // width of the arrow
    };

    struct DiameterGeometry
    {
        SbVec3f p1, p2;         // main points (center and edge for radius, endpoints for diameter)
        SbVec3f center;         // center point
        SbVec3f dir, normal;    // direction and normal vectors
        SbVec3f pos;            // text position base
        SbVec3f pnt1, pnt2;     // line segment points around text
        SbVec3f ar0, ar1, ar2;  // first arrow head triangle points
        SbVec3f ar0_1, ar1_1, ar2_1;  // second arrow head triangle points (diameter only)
        float angle;                  // text angle
        SbVec3f textOffset;           // text position
        float radius;                 // radius value
        float margin;                 // margin for calculations
        float arrowWidth;             // arrow width
        bool isDiameter;              // true for diameter, false for radius
        // Arc helper parameters
        float startAngle, startRange;  // start parameters
        float endAngle, endRange;      // start parameters
    };

    struct AngleGeometry
    {
        SbVec3f p0;                              // angle intersection point
        SbVec3f v0;                              // vector for text position
        SbVec3f v1, v2;                          // direction vectors for start and end lines
        SbVec3f pnt1, pnt2, pnt3, pnt4;          // line endpoints
        SbVec3f startArrowBase, endArrowBase;    // arrow base points
        SbVec3f dirStart, dirEnd;                // arrow directions
        float angle;                             // text angle (always 0 for angles)
        SbVec3f textOffset;                      // text position
        float length;                            // length parameter
        float startangle, endangle, range;       // angle parameters
        float r;                                 // arc radius
        float margin;                            // margin for calculations
        float arrowLength, arrowWidth;           // arrow dimensions
        double textMargin;                       // margin around text in arc
        float endLineLength1, endLineLength2;    // extension line lengths
        float endLineLength12, endLineLength22;  // extension line lengths (other side)
    };

    struct SymmetricGeometry
    {
        SbVec3f p1, p2;         // main points
        SbVec3f dir, normal;    // direction and normal vectors
        SbVec3f ar0, ar1, ar2;  // first arrow triangle points (tip, base1, base2)
        SbVec3f ar3, ar4, ar5;  // second arrow triangle points (tip, base1, base2)
        float margin;           // margin for calculations
    };

    struct ArcLengthGeometry
    {
        SbVec3f ctr, p1, p2;
        float length;
        float margin;
        float startangle, endangle;
        float range;
        float radius;
        SbVec3f vm;                      // middle direction vector
        SbVec3f pnt1, pnt2, pnt3, pnt4;  // line endpoints
        SbVec3f dirStart, dirEnd;        // arrow directions
        SbVec3f textOffset;
        float angle;
        bool isLargeArc;    // whether range > pi
        SbVec3f arcCenter;  // center for arc drawing
        float arcRadius;    // radius for arc drawing
    };

    float getScaleFactor(SoState*) const;
    void generateDistancePrimitives(SoAction* action, const SbVec3f&, const SbVec3f&);
    void generateDiameterPrimitives(SoAction* action, const SbVec3f&, const SbVec3f&);
    void generateAnglePrimitives(SoAction* action, const SbVec3f&);
    void generateSymmetricPrimitives(SoAction* action, const SbVec3f&, const SbVec3f&);
    void generateArcLengthPrimitives(SoAction* action, const SbVec3f&, const SbVec3f&, const SbVec3f&);
    SbVec3f getLabelTextCenterDistance(const SbVec3f&, const SbVec3f&);
    SbVec3f getLabelTextCenterDiameter(const SbVec3f&, const SbVec3f&);
    SbVec3f getLabelTextCenterAngle(const SbVec3f&);
    SbVec3f getLabelTextCenterArcLength(const SbVec3f&, const SbVec3f&, const SbVec3f&) const;
    bool hasDatumText() const;
    void getDimension(float scale, int& srcw, int& srch);
    DistanceGeometry calculateDistanceGeometry(const SbVec3f* points) const;
    DiameterGeometry calculateDiameterGeometry(const SbVec3f* points) const;
    AngleGeometry calculateAngleGeometry(const SbVec3f* points) const;
    SymmetricGeometry calculateSymmetricGeometry(const SbVec3f* points) const;
    ArcLengthGeometry calculateArcLengthGeometry(const SbVec3f* points) const;
    void generateLineSelectionPrimitive(
        SoAction* action,
        const SbVec3f& start,
        const SbVec3f& end,
        float width
    );
    void generateArcSelectionPrimitive(
        SoAction* action,
        const SbVec3f& center,
        float radius,
        float startAngle,
        float endAngle,
        float width
    );
    void generateArrowSelectionPrimitive(
        SoAction* action,
        const SbVec3f& base,
        const SbVec3f& dir,
        float width,
        float length
    );
    void drawDistance(const SbVec3f* points, float& angle, SbVec3f& textOffset);
    void drawDistance(const SbVec3f* points);
    void drawRadiusOrDiameter(const SbVec3f* points, float& angle, SbVec3f& textOffset);
    void drawAngle(const SbVec3f* points, float& angle, SbVec3f& textOffset);
    void drawSymmetric(const SbVec3f* points);
    void drawArcLength(const SbVec3f* points, float& angle, SbVec3f& textOffset);
    void drawText(SoState* state, int srcw, int srch, float angle, const SbVec3f& textOffset);

private:
    void drawImage();
    float imgWidth;
    float imgHeight;
    bool glimagevalid;
};

}  // namespace Gui
