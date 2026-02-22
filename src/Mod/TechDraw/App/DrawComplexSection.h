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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>

#include "DrawViewSection.h"
#include "GeometryObject.h"

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
    void makeSectionCut(const TopoDS_Shape& baseShape) override;
    TopoDS_Wire closeProfileForCut(const TopoDS_Wire& profileWire,
                                   double dMax) const;
    TopoDS_Shape makeCuttingToolFromClosedProfile(const TopoDS_Wire& profileWire, double dMax);
    TopoDS_Shape cuttingToolFromProfile(const TopoDS_Wire& inProfileWire,
                                        double dMax) const;
    TopoDS_Wire closeSingleEdgeProfile(const TopoDS_Edge& singleEdge,
                                       double dMax) const;


    void makeAlignedPieces(const TopoDS_Shape& rawShape);
    TopoDS_Compound alignSectionFaces(const TopoDS_Shape& faceIntersections) override;


    static gp_Vec makeProfileVector(const TopoDS_Wire& profileWire);
    TopoDS_Wire makeProfileWire() const;
    static TopoDS_Wire makeProfileWire(App::DocumentObject* toolObj);
    static TopoDS_Wire makeNoseToTailWire(const TopoDS_Shape& inShape);


    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& cutShape) override;
    TopoDS_Shape getShapeToIntersect() override;
    bool boxesIntersect(TopoDS_Face& face, TopoDS_Shape& shape);
    TopoDS_Shape shapeShapeIntersect(const TopoDS_Shape& shape0, const TopoDS_Shape& shape1);
    std::vector<TopoDS_Face> faceShapeIntersect(const TopoDS_Face& face, const TopoDS_Shape& shape);
    TopoDS_Compound singleToolIntersections(const TopoDS_Shape& cutShape);
    TopoDS_Compound alignedToolIntersections(const TopoDS_Shape& cutShape);

    TopoDS_Shape prepareShape(const TopoDS_Shape& cutShape, double shapeSize) override;
    TopoDS_Shape getShapeToPrepare() const override;

    gp_Pln getSectionPlane() const override;
    gp_Ax2 getCSFromBase(const std::string& sectionName) const override;

    std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds() override;
    BaseGeomPtrVector makeSectionLineGeometry();
    std::pair<Base::Vector3d, Base::Vector3d> sectionLineArrowDirs();
    std::pair<Base::Vector3d, Base::Vector3d> sectionLineArrowDirsMapped();
    TopoDS_Wire makeSectionLineWire();
    ChangePointVector getChangePointsFromSectionLine() override;

    bool isBaseValid() const override;
    bool validateProfilePosition(const TopoDS_Wire& profileWire, const gp_Ax2& sectionCS) const;
    bool validateSketchNormal(App::DocumentObject* sketchObject) const;
    bool validateProfileAlignment(const TopoDS_Wire& profileWire) const;
    static bool isProfileObject(App::DocumentObject* obj);
    static bool isMultiSegmentProfile(App::DocumentObject* obj);
    static bool isLinearProfile(App::DocumentObject* obj);
    static bool isTrulyEmpty(const TopoDS_Shape& inShape);
    static bool canBuild(gp_Ax2 sectionCS, App::DocumentObject* profileObject);

    bool showSegment(gp_Dir segmentNormal) const;
    bool showSegment(const Base::Vector3d& segmentNormal) const;
    void waitingForAlign(bool s) { m_waitingForAlign = s; }
    bool waitingForAlign() const { return m_waitingForAlign; }

    TopoDS_Shape getShapeForDetail() const override;

    static std::pair<Base::Vector3d, Base::Vector3d> getSegmentEnds(const TopoDS_Edge& segment);
    static std::pair<Base::Vector3d, Base::Vector3d> getWireEnds(const TopoDS_Wire& wire);
    static int getSegmentIndex(const TopoDS_Face& face, const std::vector<TopoDS_Edge>& edgesAll);

    static std::pair<Base::Vector3d, Base::Vector3d> sketchNormalAndX(App::DocumentObject* sketchObj);
    static std::pair<int, Base::Vector3d> findNormalForFace(const TopoDS_Face& face,
                                           const std::vector<std::pair<int, Base::Vector3d>>& normalKV,
                                           const std::vector<TopoDS_Edge>& segmentEdges);

public Q_SLOTS:
    void onSectionCutFinished() override;

private:
    bool validateOffsetProfile(const TopoDS_Wire& profile,
                               Base::Vector3d direction,
                               double angleThresholdDeg) const;
    TopoDS_Wire closeProfile(const TopoDS_Wire& profileWire,
                             double dMax) const;
    TopoDS_Shape profileToSolid(const TopoDS_Wire& closedProfileWire, double dMax) const;


    Base::Vector3d getReferenceAxis() const;

    // methods refactored out of makeAlignedPieces
    bool getReversers(const gp_Vec& gProfileVector, double& horizReverser, double& verticalReverser);
    AlignedSizeResponse getAlignedSize(const TopoDS_Shape& pieceRotated, int iPiece) const;
    TopoDS_Shape cutAndRotatePiece(const TopoDS_Shape& rawShape,
                                      const TopoDS_Face& segmentFace,
                                      int iPiece,
                                      Base::Vector3d uOrientedSegmentNormal,
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
                                           Base::Vector3d sectionNormal) const;
    static gp_Dir getFaceNormal(TopoDS_Face& face);
    static bool faceContainsEndpoints(const TopoDS_Edge& edgeToMatch,
                                      const TopoDS_Face& faceToSearch);
    static bool normalLess(const std::pair<int, Base::Vector3d>& n1,
                           const std::pair<int, Base::Vector3d>& n2);
    static bool pointOnFace(Base::Vector3d point, const TopoDS_Face& face);


    TopoDS_Edge mapEdgeToBase(const TopoDS_Edge& inEdge);
    TopoDS_Edge mapEdgeToBase(const Base::Vector3d& inVector);
    static std::vector<TopoDS_Edge> getUniqueEdges(const TopoDS_Wire& wireIn);

    static TopoDS_Shape removeEmptyShapes(const TopoDS_Shape& roughTool);

    static bool isFacePlanar(const TopoDS_Face& face);


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