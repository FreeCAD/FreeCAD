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

#include <gp_Ax2.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>

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
}

namespace TechDraw
{
class DrawProjGroupItem;
class DrawGeomHatch;
class PATLineSpec;
class LineSet;
class DashSet;

class TechDrawExport DrawViewSection : public DrawViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::DrawViewSection);

public:
    DrawViewSection();
    ~DrawViewSection() override;

    App::PropertyLink   BaseView;
    App::PropertyVector SectionNormal;
    App::PropertyVector SectionOrigin;
    App::PropertyEnumeration SectionDirection;

    App::PropertyEnumeration CutSurfaceDisplay;        //new v019
    App::PropertyFile   FileHatchPattern;
    App::PropertyFile   FileGeomPattern;               //new v019
    App::PropertyFileIncluded SvgIncluded;
    App::PropertyFileIncluded PatIncluded;
    App::PropertyString NameGeomPattern;
    App::PropertyFloat  HatchScale;

    App::PropertyString SectionSymbol;
    App::PropertyBool   FuseBeforeCut;

    bool isReallyInBox (const Base::Vector3d v, const Base::BoundBox3d bb) const;
    bool isReallyInBox (const gp_Pnt p, const Bnd_Box& bb) const;

    App::DocumentObjectExecReturn *execute() override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderViewSection";
    }
    void unsetupObject() override;
    short mustExecute() const override;

    void sectionExec(TopoDS_Shape& s);
    void makeSectionCut(TopoDS_Shape &baseShape);
    void postHlrTasks(void) override;
    virtual void postSectionCutTasks();
    void waitingForCut(bool s) { m_waitingForCut = s; }
    bool waitingForCut(void) const { return m_waitingForCut; }
    bool waitingForResult() const override;

    std::vector<TechDraw::FacePtr> getTDFaceGeometry() {return tdSectionFaces;}

    void setCSFromBase(const std::string sectionName);
    gp_Ax2 getCSFromBase(const std::string sectionName) const;

    gp_Ax2 getSectionCS() const;
    Base::Vector3d getXDirection() const override;       //don't use XDirection.getValue()

    TechDraw::DrawViewPart* getBaseDVP() const;
    TechDraw::DrawProjGroupItem* getBaseDPGI() const;

    TopoDS_Compound getSectionTFaces() { return sectionTopoDSFaces;}
    TopoDS_Face getSectionTopoDSFace(int i);
    void makeLineSets(void) ;
    std::vector<LineSet> getDrawableLines(int i = 0);
    std::vector<PATLineSpec> getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern);

    TopoDS_Shape getCutShape() {return m_cutShape;}

    static const char* SectionDirEnums[];
    static const char* CutSurfaceEnums[];

    std::pair<Base::Vector3d, Base::Vector3d> sectionLineEnds();

    bool showSectionEdges(void);

public Q_SLOTS:
    void onSectionCutFinished(void);

protected:
    TopoDS_Compound sectionTopoDSFaces;       //needed for hatching
    std::vector<LineSet> m_lineSets;
    std::vector<TechDraw::FacePtr> tdSectionFaces;


    gp_Pln getSectionPlane() const;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& shape);
    void getParameters();
    bool debugSection() const;
    int prefCutSurface() const;

    TopoDS_Shape m_cutShape;

    void onDocumentRestored() override;
    void setupObject() override;
    void setupSvgIncluded();
    void setupPatIncluded();
    void replaceSvgIncluded(std::string newSvgFile);
    void replacePatIncluded(std::string newPatFile);

    TopoDS_Shape m_rawShape;
    gp_Ax2 m_viewAxis;
    TopoDS_Shape m_scaledShape;

    QMetaObject::Connection connectCutWatcher;
    QFutureWatcher<void> m_cutWatcher;
    QFuture<void> m_cutFuture;
    bool m_waitingForCut;

};

using DrawViewSectionPython = App::FeaturePythonT<DrawViewSection>;

} //namespace TechDraw

#endif
