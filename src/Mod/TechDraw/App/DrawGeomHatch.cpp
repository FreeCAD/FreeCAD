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
#include <BRepBuilderAPI_Transform.hxx>
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
#include "DrawPage.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "DrawGeomHatch.h"

#include <Mod/TechDraw/App/DrawGeomHatchPy.h>  // generated from DrawGeomHatchPy.xml

using namespace TechDraw;
using namespace std;

App::PropertyFloatConstraint::Constraints DrawGeomHatch::scaleRange = {Precision::Confusion(),
                                                                       std::numeric_limits<double>::max(),
                                                                       pow(10,- Base::UnitsApi::getDecimals())};

PROPERTY_SOURCE(TechDraw::DrawGeomHatch, App::DocumentObject)


DrawGeomHatch::DrawGeomHatch(void)
{
    static const char *vgroup = "GeomHatch";

    ADD_PROPERTY_TYPE(Source,(0),vgroup,(App::PropertyType)(App::Prop_None),"The View + Face to be crosshatched");
    Source.setScope(App::LinkScope::Global);
   ADD_PROPERTY_TYPE(FilePattern ,(""),vgroup,App::Prop_None,"The crosshatch pattern file for this area");
    ADD_PROPERTY_TYPE(NamePattern,(""),vgroup,App::Prop_None,"The name of the pattern");
    ADD_PROPERTY_TYPE(ScalePattern,(1.0),vgroup,App::Prop_None,"GeomHatch pattern size adjustment");
    ScalePattern.setConstraints(&scaleRange);

    m_saveFile = "";
    m_saveName = "";

    getParameters();

}

DrawGeomHatch::~DrawGeomHatch()
{
}

void DrawGeomHatch::onChanged(const App::Property* prop)
{
    if (prop == &Source )   {
        if (!isRestoring()) {
            DrawGeomHatch::execute();
        }
    }
    if (isRestoring()) {
        if ((prop == &FilePattern) ||                //make sure right pattern gets loaded at start up
            (prop == &NamePattern))   {
            DrawGeomHatch::execute();
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
    //save names & check if different
    if ((!FilePattern.isEmpty())  &&
        (!NamePattern.isEmpty())) {
        if ((m_saveFile != FilePattern.getValue()) ||
            (m_saveName != NamePattern.getValue()))  {
            m_saveFile = FilePattern.getValue();
            m_saveName = NamePattern.getValue();
            std::vector<PATLineSpec> specs = getDecodedSpecsFromFile();
            m_lineSets.clear();
            for (auto& hl: specs) {
                //hl.dump("hl from file");
                LineSet ls;
                ls.setPATLineSpec(hl);
                m_lineSets.push_back(ls);
            }
        }
    }
    return App::DocumentObject::StdReturn;
}

DrawViewPart* DrawGeomHatch::getSourceView(void) const
{
    App::DocumentObject* obj = Source.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

std::vector<PATLineSpec> DrawGeomHatch::getDecodedSpecsFromFile()
{
    std::string fileSpec = FilePattern.getValue();
    std::string myPattern = NamePattern.getValue();
    return getDecodedSpecsFromFile(fileSpec,myPattern);
}


//!get all the specification lines and decode them into PATLineSpec structures
/*static*/
std::vector<PATLineSpec> DrawGeomHatch::getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern)
{
    std::vector<PATLineSpec> result;
    Base::FileInfo fi(fileSpec);
    if (!fi.isReadable()) {
        Base::Console().Error("DrawGeomHatch::getDecodedSpecsFromFile not able to open %s!\n",fileSpec.c_str());
        return result;
    }
    result = PATLineSpec::getSpecsForPattern(fileSpec,myPattern);

    return result;
}

std::vector<LineSet>  DrawGeomHatch::getTrimmedLines(int i)   //get the trimmed hatch lines for face i
{
    std::vector<LineSet> result;
    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        Base::Console().Log("DGH::getTrimmedLines - no source geometry\n");
        return result;
    }
    return getTrimmedLines(source, m_lineSets,i, ScalePattern.getValue());
}

/* static */
//! get hatch lines trimmed to face outline
std::vector<LineSet> DrawGeomHatch::getTrimmedLines(DrawViewPart* source, std::vector<LineSet> lineSets, int iface, double scale )
{
    std::vector<LineSet> result;

    if (lineSets.empty()) {
        Base::Console().Log("INFO - DGH::getTrimmedLines - no LineSets!\n");
        return result;
    }

    TopoDS_Face face = extractFace(source,iface);

    Bnd_Box bBox;
    BRepBndLib::Add(face, bBox);
    bBox.SetGap(0.0);

    for (auto& ls: lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, scale);   //completely cover face bbox with lines

        //make Compound for this linespec
        BRep_Builder builder;
        TopoDS_Compound Comp;
        builder.MakeCompound(Comp);
        for (auto& c: candidates) {
           builder.Add(Comp, c);
        }

        //Common(Compound,Face)
        BRepAlgoAPI_Common mkCommon(face, Comp);
        if ((!mkCommon.IsDone())  ||
            (mkCommon.Shape().IsNull()) ) {
            Base::Console().Log("INFO - DGH::getTrimmedLines - Common creation failed\n");
            return result;
        }
        TopoDS_Shape common = mkCommon.Shape();

        //save the boundingBox of hatch pattern
        Bnd_Box overlayBox;
        overlayBox.SetGap(0.0);
        BRepBndLib::Add(common, overlayBox);
        ls.setBBox(overlayBox);

        //get resulting edges
        std::vector<TopoDS_Edge> resultEdges;
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(common, TopAbs_EDGE, mapOfEdges);
        for ( int i = 1 ; i <= mapOfEdges.Extent() ; i++ ) {           //remember, TopExp makes no promises about the order it finds edges
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfEdges(i));
            if (edge.IsNull()) {
                Base::Console().Log("INFO - DGH::getTrimmedLines - edge: %d is NULL\n",i);
                continue;
            }
            resultEdges.push_back(edge);
        }

        std::vector<TechDraw::BaseGeom*> resultGeoms;
        int i = 0;
        for (auto& e: resultEdges) {
            TechDraw::BaseGeom* base = BaseGeom::baseFactory(e);
            if (base == nullptr) {
                Base::Console().Log("FAIL - DGH::getTrimmedLines - baseFactory failed for edge: %d\n",i);
                throw Base::ValueError("DGH::getTrimmedLines - baseFactory failed");
            }
            resultGeoms.push_back(base);
            i++;
        }
        ls.setEdges(resultEdges);
        ls.setGeoms(resultGeoms);
        result.push_back(ls);
    }
    return result;
}
/* static */
std::vector<TopoDS_Edge> DrawGeomHatch::makeEdgeOverlay(PATLineSpec hl, Bnd_Box b, double scale)
{
    std::vector<TopoDS_Edge> result;

    double minX,maxX,minY,maxY,minZ,maxZ;
    b.Get(minX,minY,minZ,maxX,maxY,maxZ);

    Base::Vector3d start;
    Base::Vector3d end;
    Base::Vector3d origin = hl.getOrigin();
    double interval = hl.getIntervalX() * scale;
    double angle = hl.getAngle();

    //only dealing with angles -180:180 for now
    if (angle > 90.0) {
         angle = -(180.0 - angle);
    } else if (angle < -90.0) {
        angle = (180 + angle);
    }
    double slope = hl.getSlope();

    if (angle == 0.0) {         //odd case 1: horizontal lines
        interval = hl.getInterval() * scale;
        double atomY  = origin.y;
        int repeatUp = (int) fabs((maxY - atomY)/interval);
        int repeatDown  = (int) fabs(((atomY - minY)/interval));
        int repeatTotal = repeatUp + repeatDown + 1;
        double yStart = atomY - repeatDown * interval;

        // make repeats
        for (int i = 0; i < repeatTotal; i++) {
            Base::Vector3d newStart(minX,yStart + float(i)*interval,0);
            Base::Vector3d newEnd(maxX,yStart + float(i)*interval,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    } else if ((angle == 90.0)  ||
               (angle == -90.0))  {         //odd case 2: vertical lines
        interval = hl.getInterval() * scale;
        double atomX  = origin.x;
        int repeatRight = (int) fabs((maxX - atomX)/interval);
        int repeatLeft  = (int) fabs((atomX - minX)/interval);
        int repeatTotal = repeatRight + repeatLeft + 1;
        double xStart = atomX - repeatLeft * interval;

        // make repeats
        for (int i = 0; i < repeatTotal; i++) {
            Base::Vector3d newStart(xStart + float(i)*interval,minY,0);
            Base::Vector3d newEnd(xStart + float(i)*interval,maxY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
//TODO: check if this makes 2-3 extra lines.  might be some "left" lines on "right" side of vv
    } else if (angle > 0) {      //oblique  (bottom left -> top right)
        //ex: 60,0,0,0,4.0,25,-25
//        Base::Console().Message("TRACE - DGH-makeEdgeOverlay - making angle > 0\n");
        double xLeftAtom = origin.x + (minY - origin.y)/slope;                  //the "atom" is the fill line that passes through the
                                                                                //pattern-origin (not necc. R2 origin)
        double xRightAtom = origin.x + (maxY - origin.y)/slope;
        int repeatRight = (int) fabs((maxX - xLeftAtom)/interval);
        int repeatLeft  = (int) fabs((xRightAtom - minX)/interval);

        double leftStartX = xLeftAtom - (repeatLeft * interval);
        double leftEndX   = xRightAtom - (repeatLeft * interval);
        int repeatTotal = repeatRight + repeatLeft + 1;

        //make repeats
        for (int i = 0; i < repeatTotal; i++) {
            Base::Vector3d newStart(leftStartX + (float(i) *  interval),minY,0);
            Base::Vector3d newEnd (leftEndX + (float(i) * interval),maxY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    } else {    //oblique (bottom right -> top left)
        // ex: -60,0,0,0,4.0,25.0,-12.5,12.5,-6
//        Base::Console().Message("TRACE - DGH-makeEdgeOverlay - making angle < 0\n");
        double xRightAtom = origin.x + ((minY - origin.y)/slope);         //x-coord of left end of Atom line
        double xLeftAtom = origin.x + ((maxY - origin.y)/slope);          //x-coord of right end of Atom line
        int repeatRight = (int) fabs((maxX - xLeftAtom)/interval);        //number of lines to Right of Atom
        int repeatLeft  = (int) fabs((xRightAtom - minX)/interval);       //number of lines to Left of Atom
        double leftEndX = xLeftAtom - (repeatLeft * interval);
        double leftStartX   = xRightAtom - (repeatLeft * interval);
        int repeatTotal = repeatRight + repeatLeft + 1;

        // make repeats
        for (int i = 0; i < repeatTotal; i++) {
            Base::Vector3d newStart(leftStartX + float(i)*interval,minY,0);
            Base::Vector3d newEnd(leftEndX + float(i)*interval,maxY,0);
            TopoDS_Edge newLine = makeLine(newStart,newEnd);
            result.push_back(newLine);
        }
    }

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

//! get all the untrimmed hatchlines for a face
//! these will be clipped to shape on the gui side
std::vector<LineSet> DrawGeomHatch::getFaceOverlay(int fdx)
{
//    Base::Console().Message("TRACE - DGH::getFaceOverlay(%d)\n",fdx);
    std::vector<LineSet> result;
    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        Base::Console().Log("DGH::getFaceOverlay - no source geometry\n");
        return result;
    }

    TopoDS_Face face = extractFace(source,fdx);

    Bnd_Box bBox;
    BRepBndLib::Add(face, bBox);
    bBox.SetGap(0.0);

    for (auto& ls: m_lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, ScalePattern.getValue());
        std::vector<TechDraw::BaseGeom*> resultGeoms;
        int i = 0;
        for (auto& e: candidates) {
            TechDraw::BaseGeom* base = BaseGeom::baseFactory(e);
            if (base == nullptr) {
                Base::Console().Log("FAIL - DGH::getFaceOverlay - baseFactory failed for edge: %d\n",i);
                throw Base::ValueError("DGH::getFaceOverlay - baseFactory failed");
            }
            resultGeoms.push_back(base);
            i++;
        }
        ls.setEdges(candidates);
        ls.setGeoms(resultGeoms);
        result.push_back(ls);
     }

    return result;
}

/* static */
//! get TopoDS_Face(iface) from DVP
TopoDS_Face DrawGeomHatch::extractFace(DrawViewPart* source, int iface )
{
    TopoDS_Face result;

    //is source a section?
    DrawViewSection* section = dynamic_cast<DrawViewSection*>(source);
    bool usingSection = false;
    if (section != nullptr) {
        usingSection = true;
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
         Base::Console().Log("INFO - DGH::extractFace - face creation failed\n");
         return result;
    }
    TopoDS_Face face = mkFace.Face();

    TopoDS_Shape temp;
    try {
        // mirror about the Y axis
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror( gp_Ax2(gp_Pnt(0.0,0.0,0.0), gp_Dir(0, 1, 0)) );
        BRepBuilderAPI_Transform mkTrf(face, mirrorTransform);
        temp = mkTrf.Shape();
    }
    catch (...) {
        Base::Console().Log("DGH::extractFace - mirror failed.\n");
        return result;
    }
    result = TopoDS::Face(temp);
    return result;
}

void DrawGeomHatch::getParameters(void)
{
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
