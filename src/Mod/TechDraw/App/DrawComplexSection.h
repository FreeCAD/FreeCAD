/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DrawComplexSection_h_
#define DrawComplexSection_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>

#include "DrawViewSection.h"

namespace TechDraw
{

//changes in direction of complex section line
class ChangePoint
{
public:
    ChangePoint(QPointF location, QPointF preDirection, QPointF postDirection);
    ChangePoint(gp_Pnt location, gp_Dir preDirection, gp_Dir postDirection);
    ~ChangePoint() = default;

    QPointF getLocation() const { return m_location; }
    QPointF getPreDirection() const { return m_preDirection; }
    QPointF getPostDirection() const { return m_postDirection; }
    void scale(double scaleFactor);

private:
    QPointF m_location;
    QPointF m_preDirection;
    QPointF m_postDirection;
};

using ChangePointVector = std::vector<ChangePoint>;

class TechDrawExport DrawComplexSection: public DrawViewSection
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawComplexSection);

public:
    DrawComplexSection();
    ~DrawComplexSection() = default;

    App::PropertyLink CuttingToolWireObject;
    App::PropertyEnumeration ProjectionStrategy;//Offset or Aligned

    TopoDS_Shape getShapeToCut() override;
    TopoDS_Shape makeCuttingTool(double dMax) override;
    gp_Ax2 getCSFromBase(const std::string sectionName) const override;
    bool isBaseValid() const override;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape &cutShape) override;
    TopoDS_Shape prepareShape(const TopoDS_Shape &cutShape, double shapeSize) override;
    TopoDS_Shape getShapeToPrepare() const override;
    void makeSectionCut(TopoDS_Shape &baseShape) override;
    TopoDS_Shape getShapeToIntersect() override;
    gp_Pln getSectionPlane() const override;
    TopoDS_Compound alignSectionFaces(TopoDS_Shape faceIntersections) override;
    std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds() override;

    bool boxesIntersect(TopoDS_Face &face, TopoDS_Shape &shape);
    TopoDS_Shape shapeShapeIntersect(const TopoDS_Shape &shape0, const TopoDS_Shape &shape1);
    std::vector<TopoDS_Face> faceShapeIntersect(const TopoDS_Face &face, const TopoDS_Shape &shape);
    TopoDS_Shape extrudeWireToFace(TopoDS_Wire &wire, gp_Dir extrudeDir, double extrudeDist);
    TopoDS_Shape makeAlignedPieces(const TopoDS_Shape &rawShape, const TopoDS_Shape &toolFaceShape,
                                   double extrudeDistance);
    TopoDS_Shape distributeAlignedPieces(std::vector<TopoDS_Shape> pieces);
    TopoDS_Compound singleToolIntersections(const TopoDS_Shape &cutShape);
    TopoDS_Compound alignedToolIntersections(const TopoDS_Shape &cutShape);

    BaseGeomPtrVector makeSectionLineGeometry();
    std::pair<Base::Vector3d, Base::Vector3d> sectionArrowDirs();
    TopoDS_Wire makeSectionLineWire();

    TopoDS_Wire makeProfileWire(App::DocumentObject *toolObj);
    TopoDS_Wire makeNoseToTailWire(TopoDS_Wire inWire);
    gp_Dir projectProfileWire(TopoDS_Wire profileWire, gp_Ax3 paperCS);
    ChangePointVector getChangePointsFromSectionLine();

    bool validateProfilePosition(TopoDS_Wire profileWire, gp_Ax2 sectionCS,
                                 gp_Dir &gClosestBasis) const;
    bool showSegment(gp_Dir segmentNormal) const;

    static bool isProfileObject(App::DocumentObject *obj);
    static bool isMultiSegmentProfile(App::DocumentObject *obj);
    static bool isLinearProfile(App::DocumentObject *obj);
    static bool isTrulyEmpty(TopoDS_Shape inShape);

private:
    gp_Dir getFaceNormal(TopoDS_Face &face);

    TopoDS_Shape m_toolFaceShape;
    TopoDS_Wire m_profileWire;

    static const char *ProjectionStrategyEnums[];
};

using DrawComplexSectionPython = App::FeaturePythonT<DrawComplexSection>;

}//namespace TechDraw

#endif
