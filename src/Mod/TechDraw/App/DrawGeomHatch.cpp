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
# include <iomanip>
# include <sstream>

#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "DrawGeomHatch.h"
#include "DrawGeomHatchPy.h" // generated from DrawGeomHatchPy.xml
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "HatchLine.h"
#include "Preferences.h"


using namespace TechDraw;
using DU = DrawUtil;

App::PropertyFloatConstraint::Constraints DrawGeomHatch::scaleRange = {
    Precision::Confusion(), std::numeric_limits<double>::max(), (0.1)}; // increment by 0.1

PROPERTY_SOURCE(TechDraw::DrawGeomHatch, App::DocumentObject)

DrawGeomHatch::DrawGeomHatch()
{
    static const char *vgroup = "GeomHatch";

    ADD_PROPERTY_TYPE(Source, (nullptr), vgroup, App::PropertyType::Prop_None,
                      "The View + Face to be crosshatched");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(FilePattern, (prefGeomHatchFile()), vgroup, App::Prop_None,
                      "The crosshatch pattern file for this area");
    ADD_PROPERTY_TYPE(PatIncluded, (""), vgroup, App::Prop_None,
                      "Embedded Pat hatch file. System use only.");// n/a to end users
    ADD_PROPERTY_TYPE(NamePattern, (prefGeomHatchName()), vgroup, App::Prop_None,
                      "The name of the pattern");
    ADD_PROPERTY_TYPE(ScalePattern, (1.0), vgroup, App::Prop_None,
                      "GeomHatch pattern size adjustment");
    ScalePattern.setConstraints(&scaleRange);
    ADD_PROPERTY_TYPE(PatternRotation, (0.0), vgroup, App::Prop_None,
                      "Pattern rotation in degrees anticlockwise");
    ADD_PROPERTY_TYPE(PatternOffset, (0.0, 0.0, 0.0), vgroup, App::Prop_None,
                      "Pattern offset");

    m_saveFile = "";
    m_saveName = "";

    std::string patFilter("pat files (*.pat *.PAT);;All files (*)");
    FilePattern.setFilter(patFilter);
}

void DrawGeomHatch::onChanged(const App::Property* prop)
{
    if (isRestoring()) {
        App::DocumentObject::onChanged(prop);
        return;
    }

    if (prop == &Source) {
        //rebuild the linesets
        makeLineSets();
    }
    if (prop == &FilePattern) {
        replacePatIncluded(FilePattern.getValue());
        makeLineSets();
    }
    if (prop == &NamePattern) {
        makeLineSets();
    }

    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawGeomHatch::execute()
{
//    Base::Console().Message("DGH::execute()\n");
    //does execute even need to exist? Its all about the property value changes
    DrawViewPart* parent = getSourceView();
    if (parent) {
        parent->requestPaint();
    }
    return App::DocumentObject::StdReturn;
}

void DrawGeomHatch::onDocumentRestored()
{
    //rebuild the linesets
    makeLineSets();

    App::DocumentObject::onDocumentRestored();
}

void DrawGeomHatch::replacePatIncluded(std::string newHatchFileName)
{
//    Base::Console().Message("DGH::replaceFileIncluded(%s)\n", newHatchFileName.c_str());
    if (newHatchFileName.empty()) {
        return;
    }

    Base::FileInfo tfi(newHatchFileName);
    if (tfi.isReadable()) {
        PatIncluded.setValue(newHatchFileName.c_str());
    } else {
        throw Base::RuntimeError("Could not read the new PAT file");
    }
}

void DrawGeomHatch::setupObject()
{
//    Base::Console().Message("DGH::setupObject()\n");
    replacePatIncluded(FilePattern.getValue());
}

void DrawGeomHatch::unsetupObject()
{
//    Base::Console().Message("DGH::unsetupObject() - status: %lu  removing: %d \n", getStatus(), isRemoving());
    App::DocumentObject* source = Source.getValue();
    DrawView* dv = dynamic_cast<DrawView*>(source);
    if (dv) {
        dv->requestPaint();
    }
    App::DocumentObject::unsetupObject();
}

//-----------------------------------------------------------------------------------

void DrawGeomHatch::makeLineSets()
{
//    Base::Console().Message("DGH::makeLineSets()\n");
    if (!PatIncluded.isEmpty() &&
        !NamePattern.isEmpty()) {
        m_lineSets.clear();
        m_lineSets = makeLineSets(PatIncluded.getValue(),
                                  NamePattern.getValue());
    }
}

/*static*/
std::vector<LineSet> DrawGeomHatch::makeLineSets(std::string fileSpec, std::string myPattern)
{
    std::vector<LineSet> lineSets;
    if (fileSpec.empty() && myPattern.empty()) {
        return lineSets;
    }

    std::vector<PATLineSpec> specs =
                DrawGeomHatch::getDecodedSpecsFromFile(fileSpec,
                                                        myPattern);
    for (auto& hl: specs) {
        //hl.dump("hl from section");
        LineSet ls;
        ls.setPATLineSpec(hl);
        lineSets.push_back(ls);
    }
    return lineSets;
}

DrawViewPart* DrawGeomHatch::getSourceView() const
{
    App::DocumentObject* obj = Source.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

std::vector<PATLineSpec> DrawGeomHatch::getDecodedSpecsFromFile()
{
    std::string fileSpec = PatIncluded.getValue();
    std::string myPattern = NamePattern.getValue();
    return getDecodedSpecsFromFile(fileSpec, myPattern);
}


//!get all the specification lines and decode them into PATLineSpec structures
/*static*/
std::vector<PATLineSpec> DrawGeomHatch::getDecodedSpecsFromFile(std::string fileSpec, std::string myPattern)
{
    Base::FileInfo fi(fileSpec);
    if (!fi.isReadable()) {
        Base::Console().Error("DrawGeomHatch::getDecodedSpecsFromFile not able to open %s!\n", fileSpec.c_str());
        return std::vector<PATLineSpec>();
    }
    return PATLineSpec::getSpecsForPattern(fileSpec, myPattern);
}

std::vector<LineSet>  DrawGeomHatch::getTrimmedLines(int i)   //get the trimmed hatch lines for face i
{
    if (m_lineSets.empty()) {
        makeLineSets();
    }

    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        return std::vector<LineSet>();
    }
    return getTrimmedLines(source, m_lineSets, i, ScalePattern.getValue(),
                           PatternRotation.getValue(), PatternOffset.getValue());
}

/* static */
std::vector<LineSet>  DrawGeomHatch::getTrimmedLinesSection(DrawViewSection* source,
                                                            std::vector<LineSet> lineSets,
                                                            TopoDS_Face f,
                                                            double scale,
                                                            double hatchRotation,
                                                            Base::Vector3d hatchOffset)
{
    std::vector<LineSet> result;
    gp_Pln p;
    Base::Vector3d vfc = DrawUtil::getFaceCenter(f);
    gp_Pnt fc(vfc.x, vfc.y, vfc.z);
    double dir = -1.0;
    if (fc.Z() < 0.0) {
        dir = -dir;
    }
    Base::Vector3d stdZ(0.0, 0.0, 1.0);
    Base::Vector3d offset = stdZ * p.Distance(fc) * dir;

    //f may be above or below paper plane and must be moved so Common operation in
    //getTrimmedLines succeeds
    TopoDS_Shape moved = ShapeUtils::moveShape(f,
                                              offset);
    TopoDS_Face fMoved = TopoDS::Face(ShapeUtils::invertGeometry(moved));
    return getTrimmedLines(
        source,
        lineSets,
        fMoved,
        scale,
        hatchRotation,
        hatchOffset
    );
}

//! get hatch lines trimmed to face outline
std::vector<LineSet> DrawGeomHatch::getTrimmedLines(DrawViewPart* source, std::vector<LineSet> lineSets,
                                                    int iface, double scale, double hatchRotation ,
                                                    Base::Vector3d hatchOffset)
{
    TopoDS_Face face = extractFace(source, iface);
    return getTrimmedLines(
        source,
        lineSets,
        face,
        scale,
        hatchRotation,
        hatchOffset
    );
}

std::vector<LineSet> DrawGeomHatch::getTrimmedLines(DrawViewPart* source,
                                                    std::vector<LineSet> lineSets,
                                                    TopoDS_Face f,
                                                    double scale,
                                                    double hatchRotation,
                                                    Base::Vector3d hatchOffset)
{
//    Base::Console().Message("DGH::getTrimmedLines() - rotation: %.3f hatchOffset: %s\n", hatchRotation, DrawUtil::formatVector(hatchOffset).c_str());
    (void)source;
    std::vector<LineSet> result;

    if (lineSets.empty()) {
        return result;
    }

    TopoDS_Face face = f;

    Bnd_Box bBox;
    BRepBndLib::AddOptimal(face, bBox);
    bBox.SetGap(0.0);

    for (auto& ls: lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, scale);   //completely cover face bbox with lines

        //make Compound for this linespec
        BRep_Builder builder;
        TopoDS_Compound gridComp;
        builder.MakeCompound(gridComp);
        for (auto& c: candidates) {
           builder.Add(gridComp, c);
        }

        TopoDS_Shape grid = gridComp;
        if (hatchRotation != 0.0) {
            double hatchRotationRad = hatchRotation * M_PI / 180.0;
            gp_Ax1 gridAxis(gp_Pnt(0.0, 0.0, 0.0), gp_Vec(gp::OZ().Direction()));
            gp_Trsf xGridRotate;
            xGridRotate.SetRotation(gridAxis, hatchRotationRad);
            BRepBuilderAPI_Transform mkTransRotate(grid, xGridRotate, true);
            grid = mkTransRotate.Shape();
        }
        gp_Trsf xGridTranslate;
        xGridTranslate.SetTranslation(DrawUtil::togp_Vec(hatchOffset));
        BRepBuilderAPI_Transform mkTransTranslate(grid, xGridTranslate, true);
        grid = mkTransTranslate.Shape();

        //Common(Compound, Face)
        BRepAlgoAPI_Common mkCommon(face, grid);
        if (!mkCommon.IsDone() ||
            mkCommon.Shape().IsNull()) {
            return result;
        }
        TopoDS_Shape common = mkCommon.Shape();

        //save the boundingBox of hatch pattern
        Bnd_Box overlayBox;
        overlayBox.SetGap(0.0);
        BRepBndLib::AddOptimal(common, overlayBox);
        ls.setBBox(overlayBox);

        //get resulting edges
        std::vector<TopoDS_Edge> resultEdges;
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(common, TopAbs_EDGE, mapOfEdges);
        for ( int i = 1 ; i <= mapOfEdges.Extent() ; i++ ) {           //remember, TopExp makes no promises about the order it finds edges
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfEdges(i));
            if (edge.IsNull()) {
                continue;
            }
            resultEdges.push_back(edge);
        }

        std::vector<TechDraw::BaseGeomPtr> resultGeoms;
        for (auto& e: resultEdges) {
            TechDraw::BaseGeomPtr base = BaseGeom::baseFactory(e);
            if (!base) {
                throw Base::ValueError("DGH::getTrimmedLines - baseFactory failed");
            }
            resultGeoms.push_back(base);
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

    double minX, maxX, minY, maxY, minZ, maxZ;
    b.Get(minX, minY, minZ, maxX, maxY, maxZ);
    //make the overlay bigger to cover rotations. might need to be bigger than 2x.
    double centerX = (minX + maxX) / 2.0;
    double widthX = maxX - minX;
    minX = centerX - widthX;
    maxX = centerX + widthX;
    double centerY = (minY + maxY) / 2.0;
    double widthY = maxY - minY;
    minY = centerY - widthY;
    maxY = centerY + widthY;

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
            Base::Vector3d newStart(minX, yStart + float(i)*interval, 0);
            Base::Vector3d newEnd(maxX, yStart + float(i)*interval, 0);
            TopoDS_Edge newLine = makeLine(newStart, newEnd);
            result.push_back(newLine);
        }
    } else if (angle == 90.0 ||
               angle == -90.0) {         //odd case 2: vertical lines
        interval = hl.getInterval() * scale;
        double atomX  = origin.x;
        int repeatRight = (int) fabs((maxX - atomX)/interval);
        int repeatLeft  = (int) fabs((atomX - minX)/interval);
        int repeatTotal = repeatRight + repeatLeft + 1;
        double xStart = atomX - repeatLeft * interval;

        // make repeats
        for (int i = 0; i < repeatTotal; i++) {
            Base::Vector3d newStart(xStart + float(i)*interval, minY, 0);
            Base::Vector3d newEnd(xStart + float(i)*interval, maxY, 0);
            TopoDS_Edge newLine = makeLine(newStart, newEnd);
            result.push_back(newLine);
        }
//TODO: check if this makes 2-3 extra lines.  might be some "left" lines on "right" side of vv
    } else if (angle > 0) {      //oblique  (bottom left -> top right)
        //ex: 60, 0,0, 0,4.0, 25, -25
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
            Base::Vector3d newStart(leftStartX + (float(i) *  interval), minY, 0);
            Base::Vector3d newEnd (leftEndX + (float(i) * interval), maxY, 0);
            TopoDS_Edge newLine = makeLine(newStart, newEnd);
            result.push_back(newLine);
        }
    } else {    //oblique (bottom right -> top left)
        // ex: -60, 0,0, 0,4.0, 25.0, -12.5, 12.5, -6
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
            Base::Vector3d newStart(leftStartX + float(i)*interval, minY, 0);
            Base::Vector3d newEnd(leftEndX + float(i)*interval, maxY, 0);
            TopoDS_Edge newLine = makeLine(newStart, newEnd);
            result.push_back(newLine);
        }
    }

    return result;
}

TopoDS_Edge DrawGeomHatch::makeLine(Base::Vector3d s, Base::Vector3d e)
{
    gp_Pnt start(s.x, s.y, 0.0);
    gp_Pnt end(e.x, e.y, 0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1, v2);
    return makeEdge1.Edge();
}

//! get all the untrimmed hatchlines for a face
//! these will be clipped to shape on the gui side
std::vector<LineSet> DrawGeomHatch::getFaceOverlay(int fdx)
{
//    Base::Console().Message("TRACE - DGH::getFaceOverlay(%d)\n", fdx);
    std::vector<LineSet> result;
    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        return result;
    }

    TopoDS_Face face = extractFace(source, fdx);

    Bnd_Box bBox;
    BRepBndLib::AddOptimal(face, bBox);
    bBox.SetGap(0.0);

    if (m_lineSets.empty()) {
        makeLineSets();
    }

    for (auto& ls: m_lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, ScalePattern.getValue());
        std::vector<TechDraw::BaseGeomPtr> resultGeoms;
        for (auto& e: candidates) {
            TechDraw::BaseGeomPtr base = BaseGeom::baseFactory(e);
            if (!base) {
                throw Base::ValueError("DGH::getFaceOverlay - baseFactory failed");
            }
            resultGeoms.push_back(base);
        }
        ls.setEdges(candidates);
        ls.setGeoms(resultGeoms);
        result.push_back(ls);
     }

    return result;
}

/* static */
//! get TopoDS_Face(iface) from DVP
//! TODO: DVP can serve these up ready to use
TopoDS_Face DrawGeomHatch::extractFace(DrawViewPart* source, int iface )
{
    std::vector<TopoDS_Wire> faceWires = source->getWireForFace(iface);

    //build face(s) from geometry
    gp_Pnt gOrg(0.0, 0.0, 0.0);
    gp_Dir gDir(0.0, 0.0, 1.0);
    gp_Pln plane(gOrg, gDir);

    BRepBuilderAPI_MakeFace mkFace(plane, faceWires.front(), true);
    std::vector<TopoDS_Wire>::iterator itWire = ++faceWires.begin();            //starting with second wire
    for (; itWire != faceWires.end(); itWire++) {
        mkFace.Add(*itWire);
    }
    if (!mkFace.IsDone()) {
         return TopoDS_Face();
    }
    TopoDS_Face face = mkFace.Face();

    TopoDS_Shape temp;
    try {
        // mirror about the Y axis
        gp_Trsf mirrorTransform;
        mirrorTransform.SetMirror( gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0, 1, 0)) );
        BRepBuilderAPI_Transform mkTrf(face, mirrorTransform);
        temp = mkTrf.Shape();
    }
    catch (...) {
        return TopoDS_Face();
    }
    return TopoDS::Face(temp);
}

//! get a translated label string from the context (ex TaskActiveView), the base name (ex ActiveView) and
//! the unique name within the document (ex ActiveView001), and use it to update the Label property.
void DrawGeomHatch::translateLabel(std::string context, std::string baseName, std::string uniqueName)
{
    Label.setValue(DU::translateArbitrary(context, baseName, uniqueName));
}

//--------------------------------------------------------------------------------------------------

PyObject *DrawGeomHatch::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new DrawGeomHatchPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

std::string DrawGeomHatch::prefGeomHatchFile()
{
    return Preferences::patFile();
}

std::string DrawGeomHatch::prefGeomHatchName()
{
    std::string defaultNamePattern = "Diamond";
    std::string result = Preferences::getPreferenceGroup("PAT")->GetASCII("NamePattern", defaultNamePattern.c_str());
    if (result.empty()) {
        return defaultNamePattern;
    }
    return result;
}

App::Color DrawGeomHatch::prefGeomHatchColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("GeomHatch", 0x00FF0000));
    return fcColor;
}



// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawGeomHatchPython, TechDraw::DrawGeomHatch)
template<> const char* TechDraw::DrawGeomHatchPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderGeomHatch";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawGeomHatch>;
}
