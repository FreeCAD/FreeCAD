/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#include <iomanip>
#include <cmath>

# include <QFile>
# include <QFileInfo>

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Standard_PrimitiveTypes.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <Precision.hxx>

#include <cmath>

#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

#include "HatchLine.h"
#include "DrawUtil.h"
#include "Geometry.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "DrawGeomHatch.h"

#include <Mod/TechDraw/App/DrawGeomHatchPy.h>  // generated from DrawGeomHatchPy.xml

using namespace TechDraw;
using namespace TechDrawGeometry;
using namespace std;

App::PropertyFloatConstraint::Constraints DrawGeomHatch::scaleRange = {Precision::Confusion(),
                                                                       std::numeric_limits<double>::max(),
                                                                       pow(10,- Base::UnitsApi::getDecimals())};

PROPERTY_SOURCE(TechDraw::DrawGeomHatch, App::DocumentObject)


DrawGeomHatch::DrawGeomHatch(void)
{
    static const char *vgroup = "GeomHatch";

    ADD_PROPERTY_TYPE(Source,(0),vgroup,(App::PropertyType)(App::Prop_None),"The View + Face to be crosshatched");
    ADD_PROPERTY_TYPE(FilePattern ,(""),vgroup,App::Prop_None,"The crosshatch pattern file for this area");
    ADD_PROPERTY_TYPE(NamePattern,(""),vgroup,App::Prop_None,"The name of the pattern");
    ADD_PROPERTY_TYPE(ScalePattern,(1.0),vgroup,App::Prop_None,"GeomHatch pattern size adjustment");
    ScalePattern.setConstraints(&scaleRange);

    getParameters();

}

DrawGeomHatch::~DrawGeomHatch()
{
}

void DrawGeomHatch::onChanged(const App::Property* prop)
{
    if (prop == &Source ) {
        if (!isRestoring()) {
              DrawGeomHatch::execute();
        }
    }

    if (prop == &FilePattern    ||
        prop == &NamePattern ) {
          if ((!FilePattern.isEmpty())  &&
              (!NamePattern.isEmpty())) {
                  std::vector<HatchLine> specs = getDecodedSpecsFromFile();
                  m_lineSets.clear();
                  for (auto& hl: specs) {
                      //hl.dump("hl from file");
                      LineSet ls;
                      ls.setHatchLine(hl);
                      m_lineSets.push_back(ls);
                  }
                      
          }
    }
    
    App::DocumentObject::onChanged(prop);
}

short DrawGeomHatch::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Source.isTouched()  ||
                    FilePattern.isTouched() ||
                    NamePattern.isTouched() );
    }

    if (result) {
        return result;
    }
    return App::DocumentObject::mustExecute();
}


App::DocumentObjectExecReturn *DrawGeomHatch::execute(void)
{
    
    return App::DocumentObject::StdReturn;
}

DrawViewPart* DrawGeomHatch::getSourceView(void) const
{
    App::DocumentObject* obj = Source.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

std::vector<HatchLine> DrawGeomHatch::getDecodedSpecsFromFile()
{
    std::string fileSpec = FilePattern.getValue();
    std::string myPattern = NamePattern.getValue();
    return getDecodedSpecsFromFile(fileSpec,myPattern);
}

     
//!get all the specification lines and decode them into HatchLine structures
/*static*/
std::vector<HatchLine> DrawGeomHatch::getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern)
{
    std::vector<HatchLine> result;
    Base::FileInfo fi(fileSpec);
    if (!fi.isReadable()) {
        Base::Console().Error("DrawGeomHatch::getDecodedSpecsFromFile not able to open %s!\n",fileSpec.c_str());
        return result;
    }
    result = HatchLine::getSpecsForPattern(fileSpec,myPattern);
    
    return result;
}

std::vector<LineSet>  DrawGeomHatch::getDrawableLines(int i)   //get the drawable lines for face i
{
    std::vector<LineSet> result;
    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        Base::Console().Message("TRACE - DC::getDrawableLines - no source geometry\n");
        return result;
    }
    return getDrawableLines(source, m_lineSets,i, ScalePattern.getValue());
}

/* static */
std::vector<LineSet> DrawGeomHatch::getDrawableLines(DrawViewPart* source, std::vector<LineSet> lineSets, int iface, double scale )
{
    std::vector<LineSet> result;

    //is source is a section?
    DrawViewSection* section = dynamic_cast<DrawViewSection*>(source);
    bool usingSection = false;
    if (section != nullptr) {
        usingSection = true;
    }
    
    if (lineSets.empty()) {
        Base::Console().Log("INFO - DC::getDrawableLines - no LineSets!\n");
        return result;
    }

    std::vector<TopoDS_Wire> faceWires;
    if (usingSection) { 
        faceWires = section->getWireForFace(iface);
    } else { 
        faceWires = source->getWireForFace(iface);
    }

    //build face(s) from geometry
    gp_Pnt gOrg(0.0,0.0,0.0);
    gp_Dir gDir(0.0,0.0,1.0);
    gp_Pln plane(gOrg,gDir);
    
    BRepBuilderAPI_MakeFace mkFace(plane, faceWires.front(), true);
    std::vector<TopoDS_Wire>::iterator itWire = ++faceWires.begin();            //starting with second wire
    for (; itWire != faceWires.end(); itWire++) {
        mkFace.Add(*itWire);
    }
    if (!mkFace.IsDone()) {
         Base::Console().Log("INFO - DC::getDrawableLines - face creation failed\n");
         return result;
    }
    TopoDS_Face face = mkFace.Face();

    Bnd_Box bBox;
    BRepBndLib::Add(face, bBox);
    bBox.SetGap(0.0);
    
    for (auto& ls: lineSets) {
        HatchLine hl = ls.getHatchLine();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, scale);

        //make Compound for this linespec
        BRep_Builder builder;
        TopoDS_Compound Comp;
        builder.MakeCompound(Comp);
        for (auto& c: candidates) {
           builder.Add(Comp, c);
        }

        //Common Compound with Face
        BRepAlgoAPI_Common mkCommon(face, Comp);
        if ((!mkCommon.IsDone())  ||
            (mkCommon.Shape().IsNull()) ) {
            Base::Console().Log("INFO - DC::getDrawableLines - Common creation failed\n");
            return result;
        }
        TopoDS_Shape common = mkCommon.Shape();
     
        //Save edges from Common
        std::vector<TopoDS_Edge> resultEdges;
        std::vector<TechDrawGeometry::BaseGeom*> resultGeoms;
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(common, TopAbs_EDGE, mapOfEdges);
        for ( int i = 1 ; i <= mapOfEdges.Extent() ; i++ ) {
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfEdges(i));
            if (edge.IsNull()) {
                Base::Console().Log("INFO - DC::getDrawableLines - edge: %d is NULL\n",i);
                continue;
            }
            TechDrawGeometry::BaseGeom* base = BaseGeom::baseFactory(edge);
            if (base == nullptr) {
                Base::Console().Log("FAIL - DC::getDrawableLines - baseFactory failed for edge: %d\n",i);
                throw Base::Exception("GeometryObject::addGeomFromCompound - baseFactory failed");
            }
            resultGeoms.push_back(base);
            resultEdges.push_back(edge);
        }
        ls.setEdges(resultEdges);
        ls.setGeoms(resultGeoms);
        result.push_back(ls);
   }
    return result;
}
/* static */
std::vector<TopoDS_Edge> DrawGeomHatch::makeEdgeOverlay(HatchLine hl, Bnd_Box b, double scale)
{
    std::vector<TopoDS_Edge> result;

    double minX,maxX,minY,maxY,minZ,maxZ;
    b.Get(minX,minY,minZ,maxX,maxY,maxZ);

    Base::Vector3d start;
    Base::Vector3d end;
    Base::Vector3d origin = hl.getOrigin();
//    double interval = hl.getInterval() * ScalePattern.getValue();
    double interval = hl.getInterval() * scale;
    double angle = hl.getAngle();

    //only dealing with angles -180:180 for now
    if (angle > 90.0) {
         angle = -(180.0 - angle);
    } else if (angle < -90.0) {
        angle = (180 + angle);
    }
    angle = -angle;                   //not sure why this is required? inverted Y?
    double slope = tan(angle * M_PI/180.0);

    if (angle == 0.0) {         //odd case 1: horizontal lines
        double y  = origin.y;
        double x1 = minX;
        double x2 = maxX;
        start = Base::Vector3d(x1,y,0);
        end   = Base::Vector3d(x2,y,0);
        int repeatUp = (int) ceil(((maxY - y)/interval) + 1);
        int repeatDown  = (int) ceil(((y - minY)/interval) + 1);
        // make up repeats
        int i;
        for (i = 1; i < repeatUp; i++) {
            Base::Vector3d newStart(minX,y + float(i)*interval,0);
            Base::Vector3d newEnd(maxX,y + float(i)*interval,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
        // make down repeats
        for (i = 1; i < repeatDown; i++) {
            Base::Vector3d newStart(minX, y - float(i)*interval,0);
            Base::Vector3d newEnd(maxX, y - float(i)*interval,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    } else if (angle > 0) {      //bottomleft - topright
        double xRightAtom = origin.x + ((maxY - origin.y)/slope);
        double xLeftAtom = origin.x + ((minY - origin.y)/slope);
        start = Base::Vector3d(xLeftAtom,minY,0);
        end   = Base::Vector3d(xRightAtom,maxY,0);
        int repeatRight = (int) ceil(((maxX - xLeftAtom)/interval) + 1);
        int repeatLeft  = (int) ceil(((xRightAtom - minX)/interval) + 1);

        // make right repeats
        int i;
        for (i = 1; i < repeatRight; i++) {
            Base::Vector3d newStart(start.x + float(i)*interval,minY,0);
            Base::Vector3d newEnd(end.x + float(i)*interval,maxY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
        // make left repeats
        for (i = 1; i < repeatLeft; i++) {
            Base::Vector3d newStart(start.x - float(i)*interval,minY,0);
            Base::Vector3d newEnd(end.x - float(i)*interval,maxY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    } else {    //topleft - bottomright
        double x2 = origin.x + (maxY - origin.y)/slope;
        double x1 = origin.x + (minY - origin.y)/slope;
        start = Base::Vector3d(x2,maxY,0);
        end   = Base::Vector3d(x1,minY,0);
        int repeatRight = (int) ceil(((maxX - start.x)/interval) + 1);
        int repeatLeft  = (int) ceil(((end.x - minX)/interval) + 1);

        // make right repeats
        int i;
        for (i = 1; i < repeatRight; i++) {
            Base::Vector3d newStart(start.x + float(i)*interval,maxY,0);
            Base::Vector3d newEnd(end.x + float(i)*interval,minY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
        // make left repeats
        for (i = 1; i < repeatLeft; i++) {
            Base::Vector3d newStart(start.x - float(i)*interval,maxY,0);
            Base::Vector3d newEnd(end.x - float(i)*interval,minY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    } 
    //atom is centre line in a set of pattern lines.
    TopoDS_Edge atom = makeLine(start,end);
    result.push_back(atom);
    return result;
}

TopoDS_Edge DrawGeomHatch::makeLine(Base::Vector3d s, Base::Vector3d e)
{
    TopoDS_Edge result;
    gp_Pnt start(s.x,s.y,0.0);
    gp_Pnt end(e.x,e.y,0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1,v2);
    result = makeEdge1.Edge();
    return result;
}

void DrawGeomHatch::getParameters(void)
{
//this is probably "/build/data/Mod/TechDraw/PAT"
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/PAT");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/PAT/";
    std::string defaultFileName = defaultDir + "FCPAT.pat";
    QString patternFileName = QString::fromStdString(hGrp->GetASCII("FilePattern",defaultFileName.c_str()));
    if (patternFileName.isEmpty()) {
        patternFileName = QString::fromStdString(defaultFileName);
    }
    QFileInfo tfi(patternFileName);
        if (tfi.isReadable()) {
            FilePattern.setValue(patternFileName.toUtf8().constData());
        } else {
            Base::Console().Error("DrawGeomHatch: PAT file: %s Not Found\n",patternFileName.toUtf8().constData());
        }
    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/PAT");
    std::string defaultNamePattern = "Diamond";
    NamePattern.setValue(hGrp->GetASCII("NamePattern",defaultNamePattern.c_str()));

}


PyObject *DrawGeomHatch::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new DrawGeomHatchPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawGeomHatchPython, TechDraw::DrawGeomHatch)
template<> const char* TechDraw::DrawGeomHatchPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderGeomHatch";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawGeomHatch>;
}
