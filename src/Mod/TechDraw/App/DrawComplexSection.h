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

class TechDrawExport AlignedSizeResponse
{
public:
    AlignedSizeResponse(const TopoDS_Shape& piece, Base::Vector3d size, double zvalue) :
        alignedPiece(piece),
        pieceSize(size),
        zMax(zvalue)    {}

    TopoDS_Shape alignedPiece;
    Base::Vector3d pieceSize;
    double zMax;
};

//NOLINTBEGIN
class TechDrawExport DrawComplexSection: public DrawViewSection
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawComplexSection);
//NOLINTEND

public:
    DrawComplexSection();
    ~DrawComplexSection() override = default;

//NOLINTBEGIN
    App::PropertyLink CuttingToolWireObject;
    App::PropertyEnumeration ProjectionStrategy;//Offset or Aligned
//NOLINTEND

    TopoDS_Shape makeCuttingTool(double dMax) override;
    gp_Ax2 getCSFromBase(const std::string& sectionName) const override;
    bool isBaseValid() const override;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& cutShape) override;
    TopoDS_Shape prepareShape(const TopoDS_Shape& cutShape, double shapeSize) override;
    TopoDS_Shape getShapeToPrepare() const override;
    TopoDS_Shape getShapeToIntersect() override;
    gp_Pln getSectionPlane() const override;
    TopoDS_Compound alignSectionFaces(const TopoDS_Shape& faceIntersections) override;
    std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds() override;

    void makeSectionCut(const TopoDS_Shape& baseShape) override;

    void waitingForAlign(bool s) { m_waitingForAlign = s; }
    bool waitingForAlign() const { return m_waitingForAlign; }

    TopoDS_Shape getShapeForDetail() const override;

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

    bool validateProfilePosition(const TopoDS_Wire& profileWire, const gp_Ax2& sectionCS) const;
    bool showSegment(gp_Dir segmentNormal) const;
    gp_Vec projectVector(const gp_Vec& vec) const;

    TopoDS_Wire makeProfileWire() const;
    static TopoDS_Wire makeProfileWire(App::DocumentObject* toolObj);
    static TopoDS_Wire makeNoseToTailWire(const TopoDS_Wire& inWire);
    static gp_Vec makeProfileVector(const TopoDS_Wire& profileWire);
    static bool isProfileObject(App::DocumentObject* obj);
    static bool isMultiSegmentProfile(App::DocumentObject* obj);
    static bool isLinearProfile(App::DocumentObject* obj);
    static bool isTrulyEmpty(const TopoDS_Shape& inShape);
    static bool canBuild(gp_Ax2 sectionCS, App::DocumentObject* profileObject);
    static gp_Vec projectVector(const gp_Vec& vec, gp_Ax2 sectionCS);
    static std::pair<Base::Vector3d, Base::Vector3d> getSegmentEnds(const TopoDS_Edge& segment);
    static std::pair<Base::Vector3d, Base::Vector3d> getWireEnds(const TopoDS_Wire& wire);

public Q_SLOTS:
    void onSectionCutFinished() override;

private:
    bool validateOffsetProfile(TopoDS_Wire profile, Base::Vector3d direction, double angleThresholdDeg) const;
    Base::Vector3d getReferenceAxis();

    // methods refactored out of makeAlignedPieces
    bool getReversers(const gp_Vec& gProfileVector, double& horizReverser, double& verticalReverser);
    AlignedSizeResponse getAlignedSize(const TopoDS_Shape& pieceRotated, int iPiece) const;
    TopoDS_Shape cutAndRotatePiece(const TopoDS_Shape& rawShape,
                                      const TopoDS_Face& segmentFace,
                                      int iPiece,
                                      Base::Vector3d uOrientedSegmentNormal,
                                      Base::Vector3d uRotateAxis,
                                      double& pieceVertical);
    TopoDS_Shape movePieceToPaperPlane(const TopoDS_Shape& piece, double sizeMax);
    TopoDS_Shape distributePiece(const TopoDS_Shape& piece,
                           double pieceSize,
                           double verticalDisplace,
                           const gp_Vec& alignmentAxis,
                           const gp_Vec& gMovementVector,
                           double baseDistance);


    std::vector<std::pair<int, Base::Vector3d> >
                    getSegmentViewDirections(const TopoDS_Wire& profileWire,
                                           Base::Vector3d sectionNormal,
                                           Base::Vector3d referenceAxis) const;

    std::vector<Base::Vector3d> getPointsForClosingProfile(const TopoDS_Wire& profileWire,
                                                           double dMax);

    static std::vector<TopoDS_Edge> getUniqueEdges(const TopoDS_Wire& wireIn);
    static TopoDS_Shape removeEmptyShapes(const TopoDS_Shape& roughTool);
    static gp_Dir getFaceNormal(TopoDS_Face& face);
    static bool faceContainsEndpoints(const TopoDS_Edge& edgeToMatch,
                                      const TopoDS_Face& faceToSearch);
    static bool normalLess(const std::pair<int, Base::Vector3d>& n1,
                           const std::pair<int, Base::Vector3d>& n2);
    static TopoDS_Wire closeProfile(const TopoDS_Wire& profileWire, Base::Vector3d referenceAxis, double dMax);
    static TopoDS_Shape profileToSolid(const TopoDS_Wire& closedProfileWire, Base::Vector3d referenceAxis, double dMax);

    TopoDS_Shape m_toolFaceShape;
    TopoDS_Shape m_alignResult;
    TopoDS_Shape m_preparedShape;//saved for detail views

    QMetaObject::Connection connectAlignWatcher;
    QFutureWatcher<void> m_alignWatcher;
    QFuture<void> m_alignFuture;
    bool m_waitingForAlign;

    static const char* ProjectionStrategyEnums[];       //NOLINT
};

using DrawComplexSectionPython = App::FeaturePythonT<DrawComplexSection>;

}//namespace TechDraw

#endif
