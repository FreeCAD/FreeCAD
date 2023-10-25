/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TechDraw_DrawGeomHatch_h_
#define TechDraw_DrawGeomHatch_h_

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "HatchLine.h"


class TopoDS_Edge;
class TopoDS_Face;
class Bnd_Box;

namespace TechDraw
{
class BaseGeom;
}

namespace TechDraw
{
class DrawViewPart;
class DrawViewSection;
class PATLineSpec;
class LineSet;
class DashSet;

class TechDrawExport DrawGeomHatch : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawGeomHatch);

public:
    DrawGeomHatch();
    ~DrawGeomHatch() override = default;

    App::PropertyLinkSub     Source;                                   //the dvX & face(s) this crosshatch belongs to
    App::PropertyFile        FilePattern;
    App::PropertyFileIncluded PatIncluded;
    App::PropertyString      NamePattern;
    App::PropertyFloatConstraint ScalePattern;
    App::PropertyFloat       PatternRotation;
    App::PropertyVector      PatternOffset;

    App::DocumentObjectExecReturn *execute(void) override;
    void onChanged(const App::Property* prop) override;
    const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderGeomHatch";
    }
    PyObject *getPyObject(void) override;
    void setupObject() override;
    void unsetupObject(void) override;
    void onDocumentRestored() override;


    DrawViewPart* getSourceView(void) const;

    std::vector<LineSet> getFaceOverlay(int i = 0);
    std::vector<LineSet> getTrimmedLines(int i = 0);
    static std::vector<LineSet> getTrimmedLines(DrawViewPart* dvp, std::vector<LineSet> lineSets, int iface,
                                                double scale, double hatchRotation = 0.0,
                                                Base::Vector3d hatchOffset = Base::Vector3d(0.0, 0.0, 0.0));
    static std::vector<LineSet> getTrimmedLines(DrawViewPart* source,
                                                std::vector<LineSet> lineSets,
                                                TopoDS_Face face,
                                                double scale , double hatchRotation = 0.0,
                                                Base::Vector3d hatchOffset = Base::Vector3d(0.0, 0.0, 0.0));
    static std::vector<LineSet> getTrimmedLinesSection(DrawViewSection* source,
                                                                std::vector<LineSet> lineSets,
                                                                TopoDS_Face f,
                                                                double scale , double hatchRotation = 0.0,
                                                                Base::Vector3d hatchOffset = Base::Vector3d(0.0, 0.0, 0.0));

    static std::vector<TopoDS_Edge> makeEdgeOverlay(PATLineSpec hl, Bnd_Box bBox,
                                    double scale);
    static TopoDS_Edge makeLine(Base::Vector3d s, Base::Vector3d e);
    static std::vector<PATLineSpec> getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern);
    static TopoDS_Face extractFace(DrawViewPart* source, int iface );
    static std::string prefGeomHatchFile(void);
    static std::string prefGeomHatchName();
    static App::Color prefGeomHatchColor();
    static std::vector<LineSet> makeLineSets(std::string fileSpec, std::string myPattern);

    void translateLabel(std::string context, std::string baseName, std::string uniqueName);

protected:
    void replacePatIncluded(std::string newHatchFileName);

    void makeLineSets(void);

    std::vector<PATLineSpec> getDecodedSpecsFromFile();

private:
    std::vector<LineSet> m_lineSets;
    std::string m_saveFile;
    std::string m_saveName;
    static App::PropertyFloatConstraint::Constraints scaleRange;

};

using DrawGeomHatchPython = App::FeaturePythonT<DrawGeomHatch>;



} //namespace TechDraw
#endif
