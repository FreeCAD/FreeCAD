/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <gp_Trsf.hxx>
# include <gp_Vec.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
#endif

#include <boost_regex.hpp>

#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Mod/Import/App/dxf/ImpExpDxf.h>
#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeCompoundPy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DimensionGeometry.h"
#include "DrawDimHelper.h"
#include "DrawGeomHatch.h"
#include "DrawPage.h"
#include "DrawPagePy.h"
#include "DrawProjectSplit.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
#include "DrawUtil.h"
#include "DrawViewAnnotation.h"
#include "DrawViewDimension.h"
#include "DrawViewPart.h"
#include "DrawViewPartPy.h"
#include "EdgeWalker.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "ProjectionAlgos.h"
#include "TechDrawExport.h"


namespace TechDraw {
//module level static C++ functions go here
}

using Part::TopoShape;
using Part::TopoShapePy;
using Part::TopoShapeEdgePy;
using Part::TopoShapeFacePy;
using Part::TopoShapeWirePy;
using Part::TopoShapeCompoundPy;
using Import::ImpExpDxfWrite;

using TechDraw::ProjectionAlgos;

using namespace std;
using namespace Part;

namespace TechDraw {

/** Copies a Python dictionary of Python strings to a C++ container.
 *
 * After the function call, the key-value pairs of the Python
 * dictionary are copied into the target buffer as C++ pairs
 * (pair<string, string>).
 *
 * @param sourceRange is a Python dictionary (Py::Dict). Both, the
 * keys and the values must be Python strings.
 *
 * @param targetIt refers to where the data should be inserted. Must
 * be of concept output iterator.
 */
template<typename OutputIt>
void copy(Py::Dict sourceRange, OutputIt targetIt)
{
  string key;
  string value;

  for (const auto& keyPy : sourceRange.keys()) {
    key = Py::String(keyPy);
    value = Py::String(sourceRange[keyPy]);
    *targetIt = {key, value};
    ++targetIt;
  }
}


class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("TechDraw")
    {
        add_varargs_method("edgeWalker", &Module::edgeWalker,
            "[wires] = edgeWalker(edgePile, inclBiggest) -- Planar graph traversal finds wires in edge pile."
        );
        add_varargs_method("findOuterWire", &Module::findOuterWire,
            "wire = findOuterWire(edgeList) -- Planar graph traversal finds OuterWire in edge pile."
        );
        add_varargs_method("findShapeOutline", &Module::findShapeOutline,
            "wire = findShapeOutline(shape, scale, direction) -- Project shape in direction and find outer wire of result."
        );
        add_varargs_method("viewPartAsDxf", &Module::viewPartAsDxf,
            "string = viewPartAsDxf(DrawViewPart) -- Return the edges of a DrawViewPart in Dxf format."
        );
        add_varargs_method("viewPartAsSvg", &Module::viewPartAsSvg,
            "string = viewPartAsSvg(DrawViewPart) -- Return the edges of a DrawViewPart in Svg format."
        );
        add_varargs_method("writeDXFView", &Module::writeDXFView,
            "writeDXFView(view, filename): Exports a DrawViewPart to a DXF file."
        );
        add_varargs_method("writeDXFPage", &Module::writeDXFPage,
            "writeDXFPage(page, filename): Exports a DrawPage to a DXF file."
        );
        add_varargs_method("findCentroid", &Module::findCentroid,
            "vector = findCentroid(shape, direction): finds geometric centroid of shape looking in direction."
        );
        add_varargs_method("makeExtentDim", &Module::makeExtentDim,
            "makeExtentDim(DrawViewPart, [edges], direction) -- draw horizontal or vertical extent dimension for edges (or all of DrawViewPart if edge list is empty. direction:  0 - Horizontal, 1 - Vertical."
        );
        add_varargs_method("makeDistanceDim", &Module::makeDistanceDim,
            "makeDistanceDim(DrawViewPart, dimType, fromPoint, toPoint) -- draw a Length dimension between fromPoint to toPoint.  FromPoint and toPoint are unscaled 2d View points. dimType is one of ['Distance', 'DistanceX', 'DistanceY'."
        );
        add_varargs_method("makeDistanceDim3d", &Module::makeDistanceDim3d,
            "makeDistanceDim(DrawViewPart, dimType, 3dFromPoint, 3dToPoint) -- draw a Length dimension between fromPoint to toPoint.  FromPoint and toPoint are unscaled 3d model points. dimType is one of ['Distance', 'DistanceX', 'DistanceY'."
        );
        add_varargs_method("makeGeomHatch", &Module::makeGeomHatch,
            "makeGeomHatch(face, [patScale], [patName], [patFile]) -- draw a geom hatch on a given face, using optionally the given scale (default 1) and a given pattern name (ex. Diamond) and .pat file (the default pattern name and/or .pat files set in preferences are used if none are given). Returns a Part compound shape."
        );
        add_varargs_method("project", &Module::project,
            "[visiblyG0, visiblyG1, hiddenG0, hiddenG1] = project(TopoShape[, App.Vector Direction, string type])\n"
            " -- Project a shape and return the visible/invisible parts of it."
        );
        add_varargs_method("projectEx", &Module::projectEx,
            "[V, V1, VN, VO, VI, H,H1, HN, HO, HI] = projectEx(TopoShape[, App.Vector Direction, string type])\n"
            " -- Project a shape and return the all parts of it."
        );
        add_keyword_method("projectToSVG", &Module::projectToSVG,
            "string = projectToSVG(TopoShape[, App.Vector direction, string type, float tolerance, dict vStyle, dict v0Style, dict v1Style, dict hStyle, dict h0Style, dict h1Style])\n"
            " -- Project a shape and return the SVG representation as string."
        );
        add_varargs_method("projectToDXF", &Module::projectToDXF,
            "string = projectToDXF(TopoShape[, App.Vector Direction, string type])\n"
            " -- Project a shape and return the DXF representation as string."
        );
        add_varargs_method("removeSvgTags", &Module::removeSvgTags,
            "string = removeSvgTags(string) -- Removes the opening and closing svg tags\n"
            "and other metatags from a svg code, making it embeddable"
        );
        add_varargs_method("exportSVGEdges", &Module::exportSVGEdges,
            "string = exportSVGEdges(TopoShape) -- export an SVG string of the shape\n"
        );
        add_varargs_method("build3dCurves", &Module::build3dCurves,
            "TopoShape = build3dCurves(TopoShape) -- convert the edges to a 3D curve\n"
	    "which is useful for shapes obtained Part.HLRBRep.Algo"
        );
        initialize("This is a module for making drawings"); // register with Python
    }
    ~Module() override {}

private:
    Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            Base::Console().Error("%s\n", str.c_str());
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object edgeWalker(const Py::Tuple& args)
    {
        PyObject *pcObj = nullptr;
        PyObject *inclBig = Py_True;
        if (!PyArg_ParseTuple(args.ptr(), "O!|O", &(PyList_Type), &pcObj, &inclBig)) {
            throw Py::TypeError("expected (listofedges, boolean");
        }

        std::vector<TopoDS_Edge> edgeList;
        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeEdgePy::Type))) {
                    const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    const TopoDS_Edge edge = TopoDS::Edge(shape);
                    edgeList.push_back(edge);
                }
            }
        }
        catch (Standard_Failure& e) {

            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            return Py::None();
        }

        bool biggie = false;
        if (inclBig == Py_True) {
            biggie = true;
        }

        Py::List result;

        std::vector<TopoDS_Edge> closedEdges;
        edgeList = DrawProjectSplit::scrubEdges(edgeList, closedEdges);

        std::vector<TopoDS_Wire> sortedWires;
        try {
            EdgeWalker eWalker;
            sortedWires = eWalker.execute(edgeList, biggie);
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        if (sortedWires.empty()) {
            Base::Console().Warning("ATDP::edgeWalker: Wire detection failed\n");
            return Py::None();
        }
        else {
            for (auto& w : sortedWires) {
                PyObject* wire = new TopoShapeWirePy(new Part::TopoShape(w));
                result.append(Py::asObject(wire));
            }
        }
        return result;
    }

    Py::Object findOuterWire(const Py::Tuple& args)
    {
        PyObject *pcObj = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(PyList_Type), &pcObj)) {
            throw Py::TypeError("expected (listofedges)");
        }

        std::vector<TopoDS_Edge> edgeList;

        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeEdgePy::Type))) {
                    const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    const TopoDS_Edge edge = TopoDS::Edge(shape);
                    edgeList.push_back(edge);
                }
            }
        }
        catch (Standard_Failure& e) {

            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            Base::Console().Message("ATDP::findOuterWire: input is empty\n");
            return Py::None();
        }

        std::vector<TopoDS_Edge> closedEdges;
        edgeList = DrawProjectSplit::scrubEdges(edgeList, closedEdges);

        PyObject* outerWire = nullptr;
        std::vector<TopoDS_Wire> sortedWires;
        try {
            EdgeWalker eWalker;
            sortedWires = eWalker.execute(edgeList);
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        if(sortedWires.empty()) {
            Base::Console().Warning("ATDP::findOuterWire: Outline wire detection failed\n");
            return Py::None();
        } else {
            outerWire = new TopoShapeWirePy(new TopoShape(*sortedWires.begin()));
        }
        return Py::asObject(outerWire);
    }

    Py::Object findShapeOutline(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);
        double scale(1.0);
        PyObject *pcObjDir(nullptr);
        if (!PyArg_ParseTuple(args.ptr(), "OdO", &pcObjShape,
                                                 &scale,
                                                 &pcObjDir)) {
            throw Py::TypeError("expected (shape, scale, direction");
        }

        if (!PyObject_TypeCheck(pcObjShape, &(TopoShapePy::Type))) {
            throw Py::TypeError("expected arg1 to be 'Shape'");
        }

        if (!PyObject_TypeCheck(pcObjDir, &(Base::VectorPy::Type))) {
            throw Py::TypeError("expected arg3 to be 'Vector'");
        }

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        if (!pShape) {
            Base::Console().Message("TRACE - AATDP::findShapeOutline - input shape is null\n");
            return Py::None();
        }

        const TopoDS_Shape& shape = pShape->getTopoShapePtr()->getShape();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pcObjDir)->value();
        std::vector<TopoDS_Edge> edgeList;
        try {
            edgeList = DrawProjectSplit::getEdgesForWalker(shape, scale, dir);
        }
        catch (Standard_Failure& e) {

            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            return Py::None();
        }

        std::vector<TopoDS_Edge> closedEdges;
        edgeList = DrawProjectSplit::scrubEdges(edgeList, closedEdges);

        PyObject* outerWire = nullptr;
        std::vector<TopoDS_Wire> sortedWires;
        try {
            EdgeWalker eWalker;
            sortedWires = eWalker.execute(edgeList);
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        if(sortedWires.empty()) {
            Base::Console().Warning("ATDP::findShapeOutline: Outline wire detection failed\n");
            return Py::None();
        } else {
            outerWire = new TopoShapeWirePy(new TopoShape(*sortedWires.begin()));
        }
        return Py::asObject(outerWire);
    }

    Py::Object viewPartAsDxf(const Py::Tuple& args)
    {
        PyObject *viewObj(nullptr);
        if (!PyArg_ParseTuple(args.ptr(), "O", &viewObj)) {
            throw Py::TypeError("expected (DrawViewPart)");
        }
        Py::String dxfReturn;

        try {
            App::DocumentObject* obj = nullptr;
            TechDraw::DrawViewPart* dvp = nullptr;
            TechDraw::DXFOutput dxfOut;
            std::string dxfText;
            std::stringstream ss;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);
                TechDraw::GeometryObjectPtr gObj = dvp->getGeometryObject();
                TopoDS_Shape shape = ShapeUtils::mirrorShape(gObj->getVisHard());
                ss << dxfOut.exportEdges(shape);
                shape = ShapeUtils::mirrorShape(gObj->getVisOutline());
                ss << dxfOut.exportEdges(shape);
                if (dvp->SmoothVisible.getValue()) {
                    shape = ShapeUtils::mirrorShape(gObj->getVisSmooth());
                    ss << dxfOut.exportEdges(shape);
                }
                if (dvp->SeamVisible.getValue()) {
                    shape = ShapeUtils::mirrorShape(gObj->getVisSeam());
                    ss << dxfOut.exportEdges(shape);
                }
                if (dvp->HardHidden.getValue()) {
                    shape = ShapeUtils::mirrorShape(gObj->getHidHard());
                    ss << dxfOut.exportEdges(shape);
                    shape = ShapeUtils::mirrorShape(gObj->getHidOutline());
                    ss << dxfOut.exportEdges(shape);
                }
                if (dvp->SmoothHidden.getValue()) {
                    shape = ShapeUtils::mirrorShape(gObj->getHidSmooth());
                    ss << dxfOut.exportEdges(shape);
                }
                if (dvp->SeamHidden.getValue()) {
                    shape = ShapeUtils::mirrorShape(gObj->getHidSeam());
                    ss << dxfOut.exportEdges(shape);
                }
                // ss now contains all edges as Dxf
                dxfReturn = Py::String(ss.str());
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return dxfReturn;
    }

    Py::Object viewPartAsSvg(const Py::Tuple& args)
    {
        PyObject *viewObj(nullptr);
        if (!PyArg_ParseTuple(args.ptr(), "O", &viewObj)) {
            throw Py::TypeError("expected (DrawViewPart)");
        }
        Py::String svgReturn;
        std::string grpHead1 = "<g fill=\"none\" stroke=\"#000000\" stroke-opacity=\"1\" stroke-width=\"";
        std::string grpHead2 = "\" stroke-linecap=\"butt\" stroke-linejoin=\"miter\" stroke-miterlimit=\"4\">\n";
        std::string grpTail  = "</g>\n";
        try {
            App::DocumentObject* obj = nullptr;
            TechDraw::DrawViewPart* dvp = nullptr;
            TechDraw::SVGOutput svgOut;
            std::string svgText;
            std::stringstream ss;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);
                TechDraw::GeometryObjectPtr gObj = dvp->getGeometryObject();
                //visible group begin "<g ... >"
                ss << grpHead1;
//                double thick = dvp->LineWidth.getValue();
                double thick = DrawUtil::getDefaultLineWeight("Thick");
                ss << thick;
                ss << grpHead2;
                TopoDS_Shape shape = gObj->getVisHard();
                ss << svgOut.exportEdges(shape);
                shape = gObj->getVisOutline();
                ss << svgOut.exportEdges(shape);
                if (dvp->SmoothVisible.getValue()) {
                    shape = gObj->getVisSmooth();
                    ss << svgOut.exportEdges(shape);
                }
                if (dvp->SeamVisible.getValue()) {
                    shape = gObj->getVisSeam();
                    ss << svgOut.exportEdges(shape);
                }
                //visible group end "</g>"
                ss << grpTail;

                if ( dvp->HardHidden.getValue()  ||
                     dvp->SmoothHidden.getValue() ||
                     dvp->SeamHidden.getValue() ) {
                    //hidden group begin
                    ss << grpHead1;
//                    thick = dvp->HiddenWidth.getValue();
                    thick = DrawUtil::getDefaultLineWeight("Thin");
                    ss << thick;
                    ss << grpHead2;
                    if (dvp->HardHidden.getValue()) {
                        shape = gObj->getHidHard();
                        ss << svgOut.exportEdges(shape);
                        shape = gObj->getHidOutline();
                        ss << svgOut.exportEdges(shape);
                    }
                    if (dvp->SmoothHidden.getValue()) {
                        shape = gObj->getHidSmooth();
                        ss << svgOut.exportEdges(shape);
                    }
                    if (dvp->SeamHidden.getValue()) {
                        shape = gObj->getHidSeam();
                        ss << svgOut.exportEdges(shape);
                    }
                    ss << grpTail;
                    //hidden group end
                }
                // ss now contains all edges as Svg
                svgReturn = Py::String(ss.str());
           }
        }
        catch (Base::Exception &e) {
            e.setPyException();
            throw Py::Exception();
        }

        return svgReturn;
    }

    void write1ViewDxf( ImpExpDxfWrite& writer, TechDraw::DrawViewPart* dvp, bool alignPage)
    {
        if(!dvp->hasGeometry())
            return;
        TechDraw::GeometryObjectPtr gObj = dvp->getGeometryObject();
        TopoDS_Shape shape = ShapeUtils::mirrorShape(gObj->getVisHard());
        double offX = 0.0;
        double offY = 0.0;
        if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(dvp);
            TechDraw::DrawProjGroup*      dpg = dpgi->getPGroup();
            if (dpg) {
                offX = dpg->X.getValue();
                offY = dpg->Y.getValue();
            }
        }
        double dvpX(0.0);
        double dvpY(0.0);
        if (alignPage) {
            dvpX = dvp->X.getValue() + offX;
            dvpY = dvp->Y.getValue() + offY;
        }
        gp_Trsf xLate;
        xLate.SetTranslation(gp_Vec(dvpX, dvpY, 0.0));
        BRepBuilderAPI_Transform mkTrf(shape, xLate);
        shape = mkTrf.Shape();
        writer.exportShape(shape);
        shape = ShapeUtils::mirrorShape(gObj->getVisOutline());
        mkTrf.Perform(shape);
        shape = mkTrf.Shape();
        writer.exportShape(shape);
        if (dvp->SmoothVisible.getValue()) {
            shape = ShapeUtils::mirrorShape(gObj->getVisSmooth());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
        if (dvp->SeamVisible.getValue()) {
            shape = ShapeUtils::mirrorShape(gObj->getVisSeam());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
        if (dvp->HardHidden.getValue()) {
            shape = ShapeUtils::mirrorShape(gObj->getHidHard());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
            shape = ShapeUtils::mirrorShape(gObj->getHidOutline());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
        if (dvp->SmoothHidden.getValue()) {
            shape = ShapeUtils::mirrorShape(gObj->getHidSmooth());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
        if (dvp->SeamHidden.getValue()) {
            shape = ShapeUtils::mirrorShape(gObj->getHidSeam());
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
        //add the cosmetic edges also
        std::vector<TechDraw::BaseGeomPtr> geoms = dvp->getEdgeGeometry();
        std::vector<TopoDS_Edge> cosmeticEdges;
        for (auto& g : geoms) {
            if (g->getHlrVisible() && g->getCosmetic()) {
                cosmeticEdges.push_back(g->getOCCEdge());
            }
        }
        if (!cosmeticEdges.empty()) {
            shape = ShapeUtils::mirrorShape(DrawUtil::vectorToCompound(cosmeticEdges));
            mkTrf.Perform(shape);
            shape = mkTrf.Shape();
            writer.exportShape(shape);
        }
    }

    Py::Object writeDXFView(const Py::Tuple& args)
    {
        PyObject *viewObj(nullptr);
        char* name(nullptr);
        PyObject *alignObj = Py_True;
        if (!PyArg_ParseTuple(args.ptr(), "Oet|O", &viewObj, "utf-8", &name, &alignObj)) {
            throw Py::TypeError("expected (view, path");
        }

        std::string filePath = std::string(name);
        std::string layerName = "none";
        PyMem_Free(name);
        bool align = false;
        if (alignObj == Py_True) {
            align = true;
        }

        try {
            ImpExpDxfWrite writer(filePath);
            writer.init();
            App::DocumentObject* obj = nullptr;
            TechDraw::DrawViewPart* dvp = nullptr;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);

                layerName = dvp->getNameInDocument();
                writer.setLayerName(layerName);
                write1ViewDxf(writer, dvp, align);
            }
            writer.endRun();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object writeDXFPage(const Py::Tuple& args)
    {
        PyObject *pageObj(nullptr);
        char* name(nullptr);
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8", &name)) {
            throw Py::TypeError("expected (page, path");
        }

        std::string filePath = std::string(name);
        std::string layerName = "none";
        PyMem_Free(name);

        try {
            ImpExpDxfWrite writer(filePath);
            writer.init();
            App::DocumentObject* obj = nullptr;
            TechDraw::DrawPage* dPage = nullptr;
            if (PyObject_TypeCheck(pageObj, &(TechDraw::DrawPagePy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(pageObj)->getDocumentObjectPtr();
                dPage = static_cast<TechDraw::DrawPage*>(obj);
                auto views = dPage->getAllViews();
                for (auto& view : views) {
                    if (view->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
                        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(view);
                        layerName = dvp->getNameInDocument();
                        writer.setLayerName(layerName);
                        write1ViewDxf(writer, dvp, true);

                    } else if (view->isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId())) {
                        TechDraw::DrawViewAnnotation* dva = static_cast<TechDraw::DrawViewAnnotation*>(view);
                        layerName = dva->getNameInDocument();
                        writer.setLayerName(layerName);
                        double height = dva->TextSize.getValue();  //mm
                        int just = 1;                              //centered
                        Base::Vector3d loc(dva->X.getValue(), dva->Y.getValue(), 0.0);
                        auto lines = dva->Text.getValues();
                        writer.exportText(lines[0].c_str(), loc, loc, height, just);

                    } else if (view->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
                        DrawViewDimension* dvd = static_cast<TechDraw::DrawViewDimension*>(view);
                        TechDraw::DrawViewPart* dvp = dvd->getViewPart();
                        if (!dvp) {
                            continue;
                        }
                        double grandParentX = 0.0;
                        double grandParentY = 0.0;
                        if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
                            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(dvp);
                            TechDraw::DrawProjGroup* dpg = dpgi->getPGroup();
                            if (!dpg) {
                                continue;
                            }
                            grandParentX = dpg->X.getValue();
                            grandParentY = dpg->Y.getValue();
                        }
                        double parentX = dvp->X.getValue() + grandParentX;
                        double parentY = dvp->Y.getValue() + grandParentY;
                        Base::Vector3d parentPos(parentX, parentY, 0.0);
                        std::string sDimText;
                        //this is the same code as in QGIViewDimension::updateDim
                        if (dvd->isMultiValueSchema()) {
                            sDimText = dvd->getFormattedDimensionValue(0); //don't format multis
                        } else {
                            sDimText = dvd->getFormattedDimensionValue(1);
                        }
                        char* dimText = &sDimText[0u];                  //hack for const-ness
                        float gap = 5.0;                                //hack. don't know font size here.
                        layerName = dvd->getNameInDocument();
                        writer.setLayerName(layerName);
                        int type = 0;                                   //Aligned/Distance
                        if ( dvd->Type.isValue("Distance")  ||
                             dvd->Type.isValue("DistanceX") ||
                             dvd->Type.isValue("DistanceY") )  {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            Base::Vector3d lineLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            pointPair pts = dvd->getLinearPoints();
                            Base::Vector3d dimLine = pts.first() - pts.second();
                            Base::Vector3d norm(-dimLine.y, dimLine.x, 0.0);
                            norm.Normalize();
                            lineLocn = lineLocn + (norm * gap);
                            Base::Vector3d extLine1Start = Base::Vector3d(pts.first().x, - pts.first().y, 0.0) +
                                                           Base::Vector3d(parentX, parentY, 0.0);
                            Base::Vector3d extLine2Start = Base::Vector3d(pts.second().x, - pts.second().y, 0.0) +
                                                           Base::Vector3d(parentX, parentY, 0.0);
                            if (dvd->Type.isValue("DistanceX") ) {
                                type = 1;
                            } else if (dvd->Type.isValue("DistanceY") ) {
                                type = 2;
                            }
                            writer.exportLinearDim(textLocn, lineLocn, extLine1Start, extLine2Start, dimText, type);
                        } else if (dvd->Type.isValue("Angle")) {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            Base::Vector3d lineLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            anglePoints pts = dvd->getAnglePoints();
                            Base::Vector3d end1 = pts.first();
                            end1.y = -end1.y;
                            Base::Vector3d end2 = pts.second();
                            end2.y = -end2.y;

                            Base::Vector3d apex = pts.vertex();
                            apex.y = -apex.y;
                            apex = apex + parentPos;

                            Base::Vector3d dimLine = end2 - end1;
                            Base::Vector3d norm(-dimLine.y, dimLine.x, 0.0);
                            norm.Normalize();
                            lineLocn = lineLocn + (norm * gap);
                            end1 = end1 + parentPos;
                            end2 = end2 + parentPos;
                            writer.exportAngularDim(textLocn, lineLocn, end1, end2, apex, dimText);
                        } else if (dvd->Type.isValue("Radius")) {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            arcPoints pts = dvd->getArcPoints();
                            pointPair arrowPts = dvd->getArrowPositions();
                            Base::Vector3d center = pts.center;
                            center.y = -center.y;
                            center = center + parentPos;
                            Base::Vector3d lineDir = (arrowPts.first() - arrowPts.second()).Normalize();
                            Base::Vector3d arcPoint = center + lineDir * pts.radius;
                            writer.exportRadialDim(center, textLocn, arcPoint, dimText);
                        } else if(dvd->Type.isValue("Diameter")){
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            arcPoints pts = dvd->getArcPoints();
                            pointPair arrowPts = dvd->getArrowPositions();
                            Base::Vector3d center = pts.center;
                            center.y = -center.y;
                            center = center + parentPos;
                            Base::Vector3d lineDir = (arrowPts.first() - arrowPts.second()).Normalize();
                            Base::Vector3d end1 = center + lineDir * pts.radius;
                            Base::Vector3d end2 = center - lineDir * pts.radius;
                            writer.exportDiametricDim(textLocn, end1, end2, dimText);
                        }
                   }
                }
            }
            writer.endRun();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object findCentroid(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);
        PyObject *pcObjDir(nullptr);
        if (!PyArg_ParseTuple(args.ptr(), "OO", &pcObjShape,
                                                &pcObjDir)) {
            throw Py::TypeError("expected (shape, direction");
        }

        if (!PyObject_TypeCheck(pcObjShape, &(TopoShapePy::Type))) {
            throw Py::TypeError("expected arg1 to be 'Shape'");
        }

        if (!PyObject_TypeCheck(pcObjDir, &(Base::VectorPy::Type))) {
            throw Py::TypeError("expected arg2 to be 'Vector'");
        }

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        if (!pShape) {
            Base::Console().Error("ShapeUtils::findCentroid - input shape is null\n");
            return Py::None();
        }

        const TopoDS_Shape& shape = pShape->getTopoShapePtr()->getShape();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pcObjDir)->value();
        Base::Vector3d centroid = ShapeUtils::findCentroidVec(shape, dir);
        PyObject* result = nullptr;
        result = new Base::VectorPy(new Base::Vector3d(centroid));
        return Py::asObject(result);
    }

    Py::Object makeExtentDim(const Py::Tuple& args)
    {
        PyObject* pDvp(nullptr);
        PyObject* pEdgeList(nullptr);
        int direction = 0;  //Horizontal
        TechDraw::DrawViewPart* dvp = nullptr;

        if (!PyArg_ParseTuple(args.ptr(), "OO!i", &pDvp, &(PyList_Type), &pEdgeList, &direction)) {
            throw Py::TypeError("expected (DrawViewPart, listofedgesnames, direction");
        }
        if (PyObject_TypeCheck(pDvp, &(TechDraw::DrawViewPartPy::Type))) {
            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(pDvp)->getDocumentObjectPtr();
            dvp = static_cast<TechDraw::DrawViewPart*>(obj);
        }

        std::vector<std::string> edgeList;
        try {
            Py::Sequence list(pEdgeList);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyUnicode_Check((*it).ptr())) {
                    std::string temp = PyUnicode_AsUTF8((*it).ptr());
                    edgeList.push_back(temp);
                }
            }
        }
        catch (Standard_Failure& e) {

            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        DrawDimHelper::makeExtentDim(dvp,
                                     edgeList,
                                     direction);
        return Py::None();
    }

    Py::Object makeDistanceDim(const Py::Tuple& args)
    {
    //points come in unscaled, but makeDistDim unscales them so we need to prescale here.
    //makeDistDim was built for extent dims which work from scaled geometry
        PyObject* pDvp(nullptr);
        PyObject* pDimType(nullptr);
        PyObject* pFrom(nullptr);
        PyObject* pTo(nullptr);
        TechDraw::DrawViewPart* dvp = nullptr;
        std::string dimType;
        Base::Vector3d from;
        Base::Vector3d to;

        if (!PyArg_ParseTuple(args.ptr(), "OOOO", &pDvp, &pDimType, &pFrom, &pTo)) {
            throw Py::TypeError("expected (DrawViewPart, dimType, from, to");
        }
        //TODO: errors for all the type checks
        if (PyObject_TypeCheck(pDvp, &(TechDraw::DrawViewPartPy::Type))) {
            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(pDvp)->getDocumentObjectPtr();
            dvp = static_cast<TechDraw::DrawViewPart*>(obj);
        }
        else {
            throw Py::TypeError("expected (DrawViewPart, dimType, from, to");
        }
        if (PyUnicode_Check(pDimType) ) {
            dimType = PyUnicode_AsUTF8(pDimType);
        }

        if (PyObject_TypeCheck(pFrom, &(Base::VectorPy::Type))) {
            from = static_cast<Base::VectorPy*>(pFrom)->value();
        }
        if (PyObject_TypeCheck(pTo, &(Base::VectorPy::Type))) {
            to = static_cast<Base::VectorPy*>(pTo)->value();
        }
        DrawViewDimension* dvd =
        DrawDimHelper::makeDistDim(dvp,
                                   dimType,
                                   DrawUtil::invertY(from),
                                   DrawUtil::invertY(to));
        PyObject* dvdPy = dvd->getPyObject();
        return Py::asObject(dvdPy);
//        return Py::None();
    }

    Py::Object makeDistanceDim3d(const Py::Tuple& args)
    {
        PyObject* pDvp;
        PyObject* pDimType;
        PyObject* pFrom;
        PyObject* pTo;
        TechDraw::DrawViewPart* dvp = nullptr;
        std::string dimType;
        Base::Vector3d from;
        Base::Vector3d to;

        if (!PyArg_ParseTuple(args.ptr(), "OOOO", &pDvp, &pDimType, &pFrom, &pTo)) {
            throw Py::TypeError("expected (DrawViewPart, dimType, from, to");
        }
        //TODO: errors for all the type checks
        if (PyObject_TypeCheck(pDvp, &(TechDraw::DrawViewPartPy::Type))) {
            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(pDvp)->getDocumentObjectPtr();
            dvp = static_cast<TechDraw::DrawViewPart*>(obj);
        }
        else {
            throw Py::TypeError("expected (DrawViewPart, dimType, from, to");
        }
        if (PyUnicode_Check(pDimType)) {
            dimType = PyUnicode_AsUTF8(pDimType);
        }
        if (PyObject_TypeCheck(pFrom, &(Base::VectorPy::Type))) {
            from = static_cast<Base::VectorPy*>(pFrom)->value();
        }
        if (PyObject_TypeCheck(pTo, &(Base::VectorPy::Type))) {
            to = static_cast<Base::VectorPy*>(pTo)->value();
        }
        //3d points are not scaled
        from = DrawUtil::invertY(dvp->projectPoint(from));
        to   = DrawUtil::invertY(dvp->projectPoint(to));
        //DrawViewDimension* =
        DrawDimHelper::makeDistDim(dvp,
                                   dimType,
                                   from,
                                   to);

        return Py::None();
    }


    Py::Object makeGeomHatch(const Py::Tuple& args)
    {
        PyObject* pFace(nullptr);
        double scale = 1.0;
        char* pPatName = "";
        char* pPatFile = "";
        TechDraw::DrawViewPart* source = nullptr;
        TopoDS_Face face;

        if (!PyArg_ParseTuple(args.ptr(), "O|detet", &pFace, &scale, "utf-8", &pPatName, "utf-8", &pPatFile)) {
            throw Py::TypeError("expected (face, [scale], [patName], [patFile])");
        }

        std::string patName = std::string(pPatName);
        PyMem_Free(pPatName);
        std::string patFile = std::string(pPatFile);
        PyMem_Free(pPatFile);

        if (PyObject_TypeCheck(pFace, &(TopoShapeFacePy::Type))) {
            const TopoDS_Shape& shape = static_cast<TopoShapePy*>(pFace)->getTopoShapePtr()->getShape();
            face = TopoDS::Face(shape);
        }
        else {
            throw Py::TypeError("first argument must be a Part.Face instance");
        }
        if (patName.empty()) {
            patName = TechDraw::DrawGeomHatch::prefGeomHatchName();
        }
        if (patFile.empty()) {
            patFile = TechDraw::DrawGeomHatch::prefGeomHatchFile();
        }
        Base::FileInfo fi(patFile);
        if (!fi.isReadable()) {
            Base::Console().Error(".pat File: %s is not readable\n", patFile.c_str());
            return Py::None();
        }
        std::vector<TechDraw::PATLineSpec> specs = TechDraw::DrawGeomHatch::getDecodedSpecsFromFile(patFile, patName);
        std::vector<LineSet> lineSets;
        for (auto& hLine : specs) {
            TechDraw::LineSet lSet;
            lSet.setPATLineSpec(hLine);
            lineSets.push_back(lSet);
        }
        std::vector<LineSet> lsresult = TechDraw::DrawGeomHatch::getTrimmedLines(source, lineSets, face, scale);
        if (!lsresult.empty()) {
            /* below code returns a list of edges, but probably slower to handle
            Py::List result;
            try {
                for (auto& lsr:lsresult) {
                    std::vector<TopoDS_Edge> edgeList = lsr.getEdges();
                    for (auto& edge:edgeList) {
                        PyObject* pyedge = new TopoShapeEdgePy(new TopoShape(edge));
                        result.append(Py::asObject(pyedge));
                    }
                }
            }
            catch (Base::Exception &e) {
                e.setPyException();
                throw Py::Exception();
            }
            return result;
            */
            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);
            try {
                for (auto& lsr : lsresult) {
                    std::vector<TopoDS_Edge> edgeList = lsr.getEdges();
                    for (auto& edge : edgeList) {
                        if (!edge.IsNull()) {
                            builder.Add(comp, edge);
                        }
                    }
                }
            }
            catch (Base::Exception &e) {
                e.setPyException();
                throw Py::Exception();
            }
            PyObject* pycomp = new TopoShapeCompoundPy(new TopoShape(comp));
            return Py::asObject(pycomp);
        }
        return Py::None();
    }

    Py::Object project(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);
        PyObject *pcObjDir(nullptr);

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!",
            &(Part::TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir))
            throw Py::Exception();

        Part::TopoShapePy* pShape = static_cast<Part::TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0, 0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->getShape(), Vector);

        Py::List list;
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.V)) , true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.V1)), true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.H)) , true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.H1)), true));

        return list;
    }
    Py::Object projectEx(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);
        PyObject *pcObjDir(nullptr);

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!",
            &(TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0, 0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->getShape(), Vector);

        Py::List list;
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V1)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VN)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VO)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VI)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H1)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HN)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HO)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HI)), true));

        return list;
    }

    Py::Object projectToSVG(const Py::Tuple& args, const Py::Dict& keys)
        {
            static const std::array<const char *, 11> argNames{"topoShape", "direction", "type", "tolerance", "vStyle",
                                                               "v0Style", "v1Style", "hStyle", "h0Style", "h1Style",
                                                               nullptr};
            PyObject *pcObjShape = nullptr;
            PyObject *pcObjDir = nullptr;
            const char *extractionTypePy = nullptr;
            ProjectionAlgos::ExtractionType extractionType = ProjectionAlgos::Plain;
            const float tol = 0.1f;
            PyObject* vStylePy = nullptr;
            ProjectionAlgos::XmlAttributes vStyle;
            PyObject* v0StylePy = nullptr;
            ProjectionAlgos::XmlAttributes v0Style;
            PyObject* v1StylePy = nullptr;
            ProjectionAlgos::XmlAttributes v1Style;
            PyObject* hStylePy = nullptr;
            ProjectionAlgos::XmlAttributes hStyle;
            PyObject* h0StylePy = nullptr;
            ProjectionAlgos::XmlAttributes h0Style;
            PyObject* h1StylePy = nullptr;
            ProjectionAlgos::XmlAttributes h1Style;

            // Get the arguments

            if (!Base::Wrapped_ParseTupleAndKeywords(
                    args.ptr(), keys.ptr(),
                    "O!|O!sfOOOOOO",
                    argNames,
                    &(TopoShapePy::Type), &pcObjShape,
                    &(Base::VectorPy::Type), &pcObjDir,
                    &extractionTypePy, &tol,
                    &vStylePy, &v0StylePy, &v1StylePy,
                    &hStylePy, &h0StylePy, &h1StylePy)) {
                throw Py::Exception();
            }

            // Convert all arguments into the right format

            TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);

            Base::Vector3d directionVector(0, 0,1);
            if (pcObjDir)
                directionVector = static_cast<Base::VectorPy*>(pcObjDir)->value();

            if (extractionTypePy && std::string(extractionTypePy) == "ShowHiddenLines")
                extractionType = ProjectionAlgos::WithHidden;

            if (vStylePy)
                copy(Py::Dict(vStylePy), inserter(vStyle, vStyle.begin()));
            if (v0StylePy)
                copy(Py::Dict(v0StylePy), inserter(v0Style, v0Style.begin()));
            if (v1StylePy)
                copy(Py::Dict(v1StylePy), inserter(v1Style, v1Style.begin()));
            if (hStylePy)
                copy(Py::Dict(hStylePy), inserter(hStyle, hStyle.begin()));
            if (h0StylePy)
                copy(Py::Dict(h0StylePy), inserter(h0Style, h0Style.begin()));
            if (h1StylePy)
                copy(Py::Dict(h1StylePy), inserter(h1Style, h1Style.begin()));

            // Execute the SVG generation

            ProjectionAlgos Alg(pShape->getTopoShapePtr()->getShape(),
                                directionVector);
            Py::String result(Alg.getSVG(extractionType, tol,
                                         vStyle, v0Style, v1Style,
                                         hStyle, h0Style, h1Style));
            return result;
        }

    Py::Object projectToDXF(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);
        PyObject *pcObjDir=nullptr;
        const char *type=nullptr;
        float scale=1.0f;
        float tol=0.1f;

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!sff",
            &(TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir, &type, &scale, &tol))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0, 0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->getShape(), Vector);

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getDXF(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale, tol));
        return result;
    }
    Py::Object removeSvgTags(const Py::Tuple& args)
    {
        const char* svgcode;
        if (!PyArg_ParseTuple(args.ptr(), "s", &svgcode))
            throw Py::Exception();

        std::string svg(svgcode);
        std::string empty;
        std::string endline = "--endOfLine--";
        std::string linebreak = "\\n";
        // removing linebreaks for regex to work
        boost::regex e1 ("\\n");
        svg = boost::regex_replace(svg, e1, endline);
        // removing starting xml definition
        boost::regex e2 ("<\\?xml.*?\\?>");
        svg = boost::regex_replace(svg, e2, empty);
        // removing starting svg tag
        boost::regex e3 ("<svg.*?>");
        svg = boost::regex_replace(svg, e3, empty);
        // removing sodipodi tags -- DANGEROUS, some sodipodi tags are single, better leave it
        //boost::regex e4 ("<sodipodi.*?>");
        //svg = boost::regex_replace(svg, e4, empty);
        // removing metadata tags
        boost::regex e5 ("<metadata.*?</metadata>");
        svg = boost::regex_replace(svg, e5, empty);
        // removing closing svg tags
        boost::regex e6 ("</svg>");
        svg = boost::regex_replace(svg, e6, empty);
        // restoring linebreaks
        boost::regex e7 ("--endOfLine--");
        svg = boost::regex_replace(svg, e7, linebreak);
        Py::String result(svg);
        return result;
    }

    Py::Object exportSVGEdges(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);

        if (!PyArg_ParseTuple(args.ptr(), "O!",
			      &(TopoShapePy::Type), &pcObjShape))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
	SVGOutput output;
	Py::String result(output.exportEdges(pShape->getTopoShapePtr()->getShape()));

	return result;
    }

    Py::Object build3dCurves(const Py::Tuple& args)
    {
        PyObject *pcObjShape(nullptr);

        if (!PyArg_ParseTuple(args.ptr(), "O!",
			      &(TopoShapePy::Type), &pcObjShape))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
	const TopoShape& nShape =
	    TechDraw::build3dCurves(pShape->getTopoShapePtr()->getShape());
	
	return Py::Object(new TopoShapePy(new TopoShape(nShape)));
    }
 };

 PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace TechDraw
