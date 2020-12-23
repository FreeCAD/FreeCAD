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

#ifndef _TechDraw_DrawGeomHatch_h_
#define _TechDraw_DrawGeomHatch_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>
#include <App/PropertyFile.h>

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
class PATLineSpec;
class LineSet;
class DashSet;

class TechDrawExport DrawGeomHatch : public App::DocumentObject
{
    PROPERTY_HEADER(TechDraw::DrawGeomHatch);

public:
    DrawGeomHatch();
    virtual ~DrawGeomHatch();

    App::PropertyLinkSub     Source;                                   //the dvX & face(s) this crosshatch belongs to
    App::PropertyFile        FilePattern;
    App::PropertyString      NamePattern;
    App::PropertyFloatConstraint ScalePattern;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderGeomHatch";
    }
    virtual PyObject *getPyObject(void);

    DrawViewPart* getSourceView(void) const;

    std::vector<LineSet> getFaceOverlay(int i = 0);
    std::vector<LineSet> getTrimmedLines(int i = 0);
    static std::vector<LineSet> getTrimmedLines(DrawViewPart* dvp, std::vector<LineSet> lineSets, int iface, double scale);

    static std::vector<TopoDS_Edge> makeEdgeOverlay(PATLineSpec hl, Bnd_Box bBox, double scale);
    static TopoDS_Edge makeLine(Base::Vector3d s, Base::Vector3d e);
    static std::vector<PATLineSpec> getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern);
    static TopoDS_Face extractFace(DrawViewPart* source, int iface );

protected:
    void getParameters(void);
    std::vector<PATLineSpec> getDecodedSpecsFromFile();
    std::vector<LineSet> m_lineSets;
    std::string m_saveFile;
    std::string m_saveName;

private:
    static App::PropertyFloatConstraint::Constraints scaleRange;

};

typedef App::FeaturePythonT<DrawGeomHatch> DrawGeomHatchPython;



} //namespace TechDraw
#endif
