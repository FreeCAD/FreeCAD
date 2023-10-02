/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DrawViewSection_h_
#define DrawViewSection_h_

#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawViewPart.h"

class Bnd_Box;
class gp_Pln;
class gp_Pnt;
class TopoDS_Face;
class TopoDS_Wire;
class gp_Ax2;

namespace TechDraw
{
class Face;
class DrawProjGroupItem;
class DrawGeomHatch;
class PATLineSpec;
class LineSet;
class DashSet;

//changes in direction of complex section line. also marks at arrow positions.
class ChangePoint
{
public:
    ChangePoint(QPointF location, QPointF preDirection, QPointF postDirection);
    ChangePoint(gp_Pnt location, gp_Dir preDirection, gp_Dir postDirection);
    ~ChangePoint() = default;

    QPointF getLocation() const { return m_location; }
    void setLocation(QPointF newLocation) { m_location = newLocation; }
    QPointF getPreDirection() const { return m_preDirection; }
    QPointF getPostDirection() const { return m_postDirection; }
    void scale(double scaleFactor);

private:
    QPointF m_location;
    QPointF m_preDirection;
    QPointF m_postDirection;
};

using ChangePointVector = std::vector<ChangePoint>;

class TechDrawExport DrawViewSection: public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawViewSection);

public:
    DrawViewSection();
    ~DrawViewSection() override;

    App::PropertyLink BaseView;
    App::PropertyVector SectionNormal;
    App::PropertyVector SectionOrigin;
    App::PropertyString SectionSymbol;


    App::PropertyEnumeration SectionDirection; //to be made obsolete eventually
    App::PropertyEnumeration CutSurfaceDisplay;//new v019
    App::PropertyFile FileHatchPattern;
    App::PropertyFile FileGeomPattern;//new v019
    App::PropertyFileIncluded SvgIncluded;
    App::PropertyFileIncluded PatIncluded;
    App::PropertyString NameGeomPattern;
    App::PropertyFloat HatchScale;
    App::PropertyFloat HatchRotation;
    App::PropertyVector HatchOffset;

    App::PropertyBool FuseBeforeCut;
    App::PropertyBool TrimAfterCut;//new v021
    App::PropertyBool UsePreviousCut;   // new v022

    App::PropertyFloat SectionLineStretch;  // new v022


    bool isReallyInBox(const Base::Vector3d v, const Base::BoundBox3d bb) const;
    bool isReallyInBox(const gp_Pnt p, const Bnd_Box& bb) const;

    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderViewSection";
    }
    void unsetupObject() override;
    short mustExecute() const override;

    void sectionExec(TopoDS_Shape& s);
    virtual void makeSectionCut(const TopoDS_Shape& baseShape);
    void postHlrTasks(void) override;
    virtual void postSectionCutTasks();
    void waitingForCut(bool s) { m_waitingForCut = s; }
    bool waitingForCut(void) const { return m_waitingForCut; }
    bool waitingForResult() const override;

    virtual TopoDS_Shape makeCuttingTool(double shapeSize);
    virtual TopoDS_Shape getShapeToCut();
    virtual bool isBaseValid() const;
    virtual TopoDS_Shape prepareShape(const TopoDS_Shape& rawShape, double shapeSize);
    virtual TopoDS_Shape getShapeToPrepare() const { return m_cutPieces; }

    //CS related methods
    gp_Ax2 getProjectionCS(Base::Vector3d pt = Base::Vector3d(0.0, 0.0, 0.0)) const override;
    void setCSFromBase(const std::string sectionName);
    void setCSFromBase(Base::Vector3d localUnit);
    void setCSFromLocalUnit(const Base::Vector3d localUnit);
    virtual gp_Ax2 getCSFromBase(const std::string sectionName) const;
    gp_Ax2 getSectionCS() const;
    Base::Vector3d getXDirection() const override;//don't use XDirection.getValue()

    TechDraw::DrawViewPart* getBaseDVP() const;

    //section face related methods
    std::vector<TechDraw::FacePtr> getTDFaceGeometry() { return m_tdSectionFaces; }
    TopoDS_Face getSectionTopoDSFace(int i);
    virtual TopoDS_Compound alignSectionFaces(TopoDS_Shape faceIntersections);
    TopoDS_Compound mapToPage(TopoDS_Shape& shapeToAlign);
    virtual std::vector<TechDraw::FacePtr> makeTDSectionFaces(TopoDS_Compound topoDSFaces);
    virtual TopoDS_Shape getShapeToIntersect() { return m_cutPieces; }

    void makeLineSets(void);
    std::vector<LineSet> getDrawableLines(int i = 0);
    std::vector<PATLineSpec> getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern);

    TopoDS_Shape getCutShape() const { return m_cutShape; }
    TopoDS_Shape getCutShapeRaw() const { return m_cutShapeRaw; }

    TopoDS_Shape getShapeForDetail() const override;

    static const char* SectionDirEnums[];
    static const char* CutSurfaceEnums[];

    virtual std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds();
    virtual ChangePointVector getChangePointsFromSectionLine();

    bool showSectionEdges(void);

    TopoDS_Shape makeFaceFromWires(std::vector<TopoDS_Wire> &inWires);

public Q_SLOTS:
    virtual void onSectionCutFinished(void);

protected:
    TopoDS_Compound m_sectionTopoDSFaces;//needed for hatching
    std::vector<LineSet> m_lineSets;
    std::vector<TechDraw::FacePtr> m_tdSectionFaces;


    virtual gp_Pln getSectionPlane() const;
    virtual TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& shape);
    void getParameters();
    bool debugSection() const;
    int prefCutSurface() const;
    bool trimAfterCut() const;

    TopoDS_Shape m_cutShape;        // centered, scaled, rotated result of cut
    TopoDS_Shape m_cutShapeRaw;     // raw result of cut w/o center/scale/rotate

    void onDocumentRestored() override;
    void setupObject() override;
    void replaceSvgIncluded(std::string newSvgFile);
    void replacePatIncluded(std::string newPatFile);

    TopoDS_Shape m_cutPieces;//the shape after cutting, but before centering & scaling
    gp_Ax2 m_projectionCS;
    TopoDS_Shape m_preparedShape;//the shape after cutting, centering, scaling etc

    QMetaObject::Connection connectCutWatcher;
    QFutureWatcher<void> m_cutWatcher;
    QFuture<void> m_cutFuture;
    bool m_waitingForCut;
    TopoDS_Shape m_cuttingTool;
    double m_shapeSize;
};

using DrawViewSectionPython = App::FeaturePythonT<DrawViewSection>;

}//namespace TechDraw

#endif
