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

class TechDrawExport DrawComplexSection: public DrawViewSection
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawComplexSection);

public:
    DrawComplexSection();
    ~DrawComplexSection() override = default;

    App::PropertyLink CuttingToolWireObject;
    App::PropertyEnumeration ProjectionStrategy;//Offset or Aligned

    TopoDS_Shape makeCuttingTool(double dMax) override;
    gp_Ax2 getCSFromBase(const std::string sectionName) const override;
    bool isBaseValid() const override;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& cutShape) override;
    TopoDS_Shape prepareShape(const TopoDS_Shape& cutShape, double shapeSize) override;
    TopoDS_Shape getShapeToPrepare() const override;
    TopoDS_Shape getShapeToIntersect() override;
    gp_Pln getSectionPlane() const override;
    TopoDS_Compound alignSectionFaces(TopoDS_Shape faceIntersections) override;
    std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds() override;

    void makeSectionCut(const TopoDS_Shape& baseShape) override;

    void waitingForAlign(bool s) { m_waitingForAlign = s; }
    bool waitingForAlign(void) const { return m_waitingForAlign; }

    TopoDS_Shape getShapeForDetail() const override;

public Q_SLOTS:
    void onSectionCutFinished(void) override;

    bool boxesIntersect(TopoDS_Face& face, TopoDS_Shape& shape);
    TopoDS_Shape shapeShapeIntersect(const TopoDS_Shape& shape0, const TopoDS_Shape& shape1);
    std::vector<TopoDS_Face> faceShapeIntersect(const TopoDS_Face& face, const TopoDS_Shape& shape);
    TopoDS_Shape extrudeWireToFace(TopoDS_Wire& wire, gp_Dir extrudeDir, double extrudeDist);
    void makeAlignedPieces(const TopoDS_Shape& rawShape);
    TopoDS_Compound singleToolIntersections(const TopoDS_Shape& cutShape);
    TopoDS_Compound alignedToolIntersections(const TopoDS_Shape& cutShape);

    BaseGeomPtrVector makeSectionLineGeometry();
    std::pair<Base::Vector3d, Base::Vector3d> sectionArrowDirs();
    TopoDS_Wire makeSectionLineWire();

    ChangePointVector getChangePointsFromSectionLine() override;

    bool validateProfilePosition(TopoDS_Wire profileWire, gp_Ax2 sectionCS,
                                 gp_Dir& gClosestBasis) const;
    bool showSegment(gp_Dir segmentNormal) const;
    gp_Vec projectVector(const gp_Vec& vec) const;

    TopoDS_Wire makeProfileWire() const;
    static TopoDS_Wire makeProfileWire(App::DocumentObject* toolObj);
    static TopoDS_Wire makeNoseToTailWire(TopoDS_Wire inWire);
    static gp_Vec makeProfileVector(TopoDS_Wire profileWire);
    static bool isProfileObject(App::DocumentObject* obj);
    static bool isMultiSegmentProfile(App::DocumentObject* obj);
    static bool isLinearProfile(App::DocumentObject* obj);
    static bool isTrulyEmpty(TopoDS_Shape inShape);
    static bool canBuild(gp_Ax2 sectionCS, App::DocumentObject* profileObject);
    static gp_Vec projectVector(const gp_Vec& vec, gp_Ax2 sectionCS);

private:
    gp_Dir getFaceNormal(TopoDS_Face& face);
    bool validateOffsetProfile(TopoDS_Wire profile, Base::Vector3d direction, double angleThresholdDeg) const;
    std::pair<Base::Vector3d, Base::Vector3d> getSegmentEnds(TopoDS_Edge segment) const;

    TopoDS_Shape m_toolFaceShape;
    TopoDS_Shape m_alignResult;
    TopoDS_Shape m_preparedShape;//saved for detail views

    QMetaObject::Connection connectAlignWatcher;
    QFutureWatcher<void> m_alignWatcher;
    QFuture<void> m_alignFuture;
    bool m_waitingForAlign;

    static const char* ProjectionStrategyEnums[];
};

using DrawComplexSectionPython = App::FeaturePythonT<DrawComplexSection>;

}//namespace TechDraw

#endif
