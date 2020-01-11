/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _DrawViewSection_h_
#define _DrawViewSection_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <App/Material.h>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>

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
    DrawViewSection(void);
    virtual ~DrawViewSection();

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

    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual void onChanged(const App::Property* prop) override;
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderViewSection";
    }
    virtual void unsetupObject() override;
    virtual short mustExecute() const override;

    std::vector<TechDraw::Face*> getFaceGeometry();

    void setCSFromBase(const std::string sectionName);
    gp_Ax2 getCSFromBase(const std::string sectionName) const;

    gp_Ax2 rotateCSArbitrary(gp_Ax2 oldCS,
                             Base::Vector3d axis,
                             double degAngle) const;
    gp_Ax2 getSectionCS() const;
    virtual Base::Vector3d getXDirection(void) const override;       //don't use XDirection.getValue()

    TechDraw::DrawViewPart* getBaseDVP() const;
    TechDraw::DrawProjGroupItem* getBaseDPGI() const;

    virtual std::vector<TopoDS_Wire> getWireForFace(int idx) const override;
    TopoDS_Compound getSectionFaces() { return sectionFaces;};
    std::vector<TopoDS_Wire> getSectionFaceWires(void) { return sectionFaceWires; }

    std::vector<LineSet> getDrawableLines(int i = 0);
    std::vector<PATLineSpec> getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern);

    TopoDS_Shape getCutShape(void) {return m_cutShape;}

    static const char* SectionDirEnums[];
    static const char* CutSurfaceEnums[];

protected:
    TopoDS_Compound sectionFaces;
    std::vector<TopoDS_Wire> sectionFaceWires;
    std::vector<LineSet> m_lineSets;

    gp_Pln getSectionPlane() const;
    TopoDS_Compound findSectionPlaneIntersections(const TopoDS_Shape& shape);
    TopoDS_Face projectFace(const TopoDS_Shape &face,
                            const gp_Ax2 CS);
                                     
    void getParameters(void);
    bool debugSection(void) const;

    TopoDS_Shape m_cutShape;

    void copyFile(std::string inSpec, std::string outSpec);


    virtual void onDocumentRestored();
    virtual void setupObject();
    void setupSvgIncluded(void);
    void setupPatIncluded(void);
    void replaceSvgIncluded(std::string newSvgFile);
    void replacePatIncluded(std::string newPatFile);


};

typedef App::FeaturePythonT<DrawViewSection> DrawViewSectionPython;

} //namespace TechDraw

#endif
