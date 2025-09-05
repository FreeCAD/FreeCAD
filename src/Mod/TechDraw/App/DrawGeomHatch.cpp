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
# include <limits>
# include <sstream>

#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
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
#include <Base/Converter.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

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
//    Base::Console().message("DGH::execute()\n");
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
//    Base::Console().message("DGH::replaceFileIncluded(%s)\n", newHatchFileName.c_str());
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
//    Base::Console().message("DGH::setupObject()\n");
    replacePatIncluded(FilePattern.getValue());
}

void DrawGeomHatch::unsetupObject()
{
//    Base::Console().message("DGH::unsetupObject() - status: %lu  removing: %d \n", getStatus(), isRemoving());
    App::DocumentObject* source = Source.getValue();
    DrawView* dv = freecad_cast<DrawView*>(source);
    if (dv) {
        dv->requestPaint();
    }
    App::DocumentObject::unsetupObject();
}

//-----------------------------------------------------------------------------------

void DrawGeomHatch::makeLineSets()
{
//    Base::Console().message("DGH::makeLineSets()\n");
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
    DrawViewPart* result = freecad_cast<DrawViewPart*>(obj);
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
        Base::Console().error("DrawGeomHatch::getDecodedSpecsFromFile not able to open %s!\n", fileSpec.c_str());
        return std::vector<PATLineSpec>();
    }
    return PATLineSpec::getSpecsForPattern(fileSpec, myPattern);
}

std::vector<LineSet>  DrawGeomHatch::getTrimmedLines(int iFace)   //get the trimmed hatch lines for face i
{
    if (m_lineSets.empty()) {
        makeLineSets();
    }

    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        return std::vector<LineSet>();
    }
    return getTrimmedLines(source, m_lineSets, iFace, ScalePattern.getValue(),
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
//    Base::Console().message("DGH::getTrimmedLines() - rotation: %.3f hatchOffset: %s\n", hatchRotation, DrawUtil::formatVector(hatchOffset).c_str());
    (void)source;
    std::vector<LineSet> result;

    if (lineSets.empty()) {
        return result;
    }

    TopoDS_Face face = f;

    Bnd_Box bBox;
    BRepBndLib::AddOptimal(face, bBox);
    bBox.SetGap(0.0);
    gp_Vec translateVector(hatchOffset.x, hatchOffset.y, 0.);
    auto cornerMin = bBox.CornerMin().Translated(-translateVector);
    auto cornerMax = bBox.CornerMax().Translated(-translateVector);
    bBox = Bnd_Box(cornerMin, cornerMax);

    for (auto& ls: lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, scale, hatchRotation);   //completely cover face bbox with lines

        //make Compound for this linespec
        BRep_Builder builder;
        TopoDS_Compound gridComp;
        builder.MakeCompound(gridComp);
        for (auto& c: candidates) {
           builder.Add(gridComp, c);
        }

        TopoDS_Shape grid = gridComp;
        gp_Trsf xGridTranslate;
        xGridTranslate.SetTranslation(Base::convertTo<gp_Vec>(hatchOffset));
        BRepBuilderAPI_Transform mkTransTranslate(grid, xGridTranslate, true);
        grid = mkTransTranslate.Shape();

        //Common(Compound, Face)
        FCBRepAlgoAPI_Common mkCommon(face, grid);
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
std::vector<TopoDS_Edge> DrawGeomHatch::makeEdgeOverlay(PATLineSpec hatchLine, Bnd_Box bBox, double scale, double rotation)
{
    const size_t MaxNumberOfEdges = Preferences::getPreferenceGroup("PAT")->GetInt("MaxSeg", 10000l);

    std::vector<TopoDS_Edge> result;
    double minX, maxX, minY, maxY, minZ, maxZ;
    bBox.Get(minX, minY, minZ, maxX, maxY, maxZ);
    Base::Vector3d topLeft(minX, maxY, 0.);
    Base::Vector3d topRight(maxX, maxY, 0.);
    Base::Vector3d bottomLeft(minX, minY, 0.);
    Base::Vector3d bottomRight(maxX, minY, 0.);

    Base::Vector3d origin = hatchLine.getOrigin() * scale;
    double interval = hatchLine.getInterval() * scale;
    double offset = hatchLine.getOffset() * scale;
    double angle = hatchLine.getAngle() + rotation;
    origin.RotateZ(Base::toRadians(rotation));

    if (scale == 0. || interval == 0.)
        return {};

    const double hatchAngle = Base::toRadians(angle);
    Base::Vector3d hatchDirection(cos(hatchAngle), sin(hatchAngle), 0.);
    Base::Vector3d hatchPerpendicular(-hatchDirection.y, hatchDirection.x, 0.);
    Base::Vector3d hatchIntervalAndOffset = offset * hatchDirection + interval * hatchPerpendicular;

    std::array<double, 4> orthogonalProjections = {
        (topLeft - origin).Dot(hatchPerpendicular / interval),
        (topRight - origin).Dot(hatchPerpendicular / interval),
        (bottomLeft - origin).Dot(hatchPerpendicular / interval),
        (bottomRight - origin).Dot(hatchPerpendicular / interval)
    };
    auto minMaxIterators = std::minmax_element(orthogonalProjections.begin(), orthogonalProjections.end());
    int firstRepeatIndex = ceil(*minMaxIterators.first);
    int lastRepeatIndex = floor(*minMaxIterators.second);

    std::vector<double> dashParams = hatchLine.getDashParms().get();
    double globalDashStep = 0.;
    if (dashParams.empty()) {
        // we define a single dash with length equal to twice the diagonal of the bounding box
        double diagonalLength = (topRight - bottomLeft).Length();
        dashParams.push_back(2. * diagonalLength);
        globalDashStep = diagonalLength;
    }
    else {
        for (auto& x : dashParams) {
            x *= scale;
            globalDashStep += std::abs(x);
        }
    }
    if (globalDashStep == 0.) {
        return {};
    }

    // we handle hatch as a set of parallel lines made of dashes, here we loop on each line
    for (int i = firstRepeatIndex ; i <= lastRepeatIndex ; ++i) {
        Base::Vector3d currentOrigin = origin + static_cast<double>(i) * hatchIntervalAndOffset;

        int firstDashIndex, lastDashIndex;
        if (std::abs(hatchDirection.x) > std::abs(hatchDirection.y)) {  // we compute intersections with minX and maxX
            firstDashIndex = (hatchDirection.x > 0.)
                    ? std::floor((minX - currentOrigin.x) / (globalDashStep * hatchDirection.x))
                    : std::floor((maxX - currentOrigin.x) / (globalDashStep * hatchDirection.x));
            lastDashIndex = (hatchDirection.x > 0.)
                    ? std::ceil((maxX - currentOrigin.x) / (globalDashStep * hatchDirection.x))
                    : std::ceil((minX - currentOrigin.x) / (globalDashStep * hatchDirection.x));
        }
        else {  // we compute intersections with minY and maxY
            firstDashIndex = (hatchDirection.y > 0.)
                    ? std::floor((minY - currentOrigin.y) / (globalDashStep * hatchDirection.y))
                    : std::floor((maxY - currentOrigin.y) / (globalDashStep * hatchDirection.y));
            lastDashIndex = (hatchDirection.y > 0.)
                    ? std::ceil((maxY - currentOrigin.y) / (globalDashStep * hatchDirection.y))
                    : std::ceil((minY - currentOrigin.y) / (globalDashStep * hatchDirection.y));
        }

        for (int j = firstDashIndex ; j < lastDashIndex ; ++j) {
            Base::Vector3d current = currentOrigin + static_cast<double>(j) * globalDashStep * hatchDirection;
            for (auto dashParamsIterator = dashParams.begin() ; dashParamsIterator != dashParams.end() ; ++dashParamsIterator) {
                double len = *dashParamsIterator;
                Base::Vector3d next = current + std::abs(len) * hatchDirection;
                if (len > 0. && (current.x >= minX || next.x >= minX) && (current.x <= maxX || next.x <= maxX)
                        && (current.y >= minY || next.y >= minY) && (current.y <= maxY || next.y <= maxY)) {
                    TopoDS_Edge newLine = makeLine(current, next);
                    result.push_back(newLine);
                }
                std::swap(current, next);
            }
        }

        if (result.size() > MaxNumberOfEdges) {
            return {};
        }
    }

    return result;
}

TopoDS_Edge DrawGeomHatch::makeLine(const Base::Vector3d& s, const Base::Vector3d& e)
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
std::vector<LineSet> DrawGeomHatch::getFaceOverlay(int iFace)
{
//    Base::Console().message("TRACE - DGH::getFaceOverlay(%d)\n", iFace);
    std::vector<LineSet> result;
    DrawViewPart* source = getSourceView();
    if (!source ||
        !source->hasGeometry()) {
        return result;
    }

    TopoDS_Face face = extractFace(source, iFace);

    Bnd_Box bBox;
    BRepBndLib::AddOptimal(face, bBox);
    bBox.SetGap(0.0);

    if (m_lineSets.empty()) {
        makeLineSets();
    }

    for (auto& ls: m_lineSets) {
        PATLineSpec hl = ls.getPATLineSpec();
        std::vector<TopoDS_Edge> candidates = DrawGeomHatch::makeEdgeOverlay(hl, bBox, ScalePattern.getValue(), PatternRotation.getValue());
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

Base::Color DrawGeomHatch::prefGeomHatchColor()
{
    Base::Color fcColor;
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
