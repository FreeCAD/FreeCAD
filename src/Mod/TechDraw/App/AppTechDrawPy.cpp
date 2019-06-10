/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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
#include <Python.h>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Part/App/OCCError.h>

#include <Mod/Drawing/App/DrawingExport.h>
#include <Mod/Import/App/ImpExpDxf.h>

#include "DrawProjectSplit.h"
#include "DrawViewPart.h"
#include "DrawViewPartPy.h"
#include "DrawViewAnnotation.h"
#include "DrawViewDimension.h"
#include "DrawPage.h"
#include "DrawPagePy.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "EdgeWalker.h"
#include "DrawUtil.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"


namespace TechDraw {
//module level static C++ functions go here
}

using Part::TopoShape;
using Part::TopoShapePy;
using Part::TopoShapeEdgePy;
using Part::TopoShapeWirePy;
using Import::ImpExpDxfWrite;

namespace TechDraw {

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("TechDraw")
    {
        add_varargs_method("edgeWalker",&Module::edgeWalker,
            "[wires] = edgeWalker(edgePile,inclBiggest) -- Planar graph traversal finds wires in edge pile."
        );
        add_varargs_method("findOuterWire",&Module::findOuterWire,
            "wire = findOuterWire(edgeList) -- Planar graph traversal finds OuterWire in edge pile."
        );
        add_varargs_method("findShapeOutline",&Module::findShapeOutline,
            "wire = findShapeOutline(shape,scale,direction) -- Project shape in direction and find outer wire of result."
        );
        add_varargs_method("viewPartAsDxf",&Module::viewPartAsDxf,
            "string = viewPartAsDxf(DrawViewPart) -- Return the edges of a DrawViewPart in Dxf format."
        );
        add_varargs_method("viewPartAsSvg",&Module::viewPartAsSvg,
            "string = viewPartAsSvg(DrawViewPart) -- Return the edges of a DrawViewPart in Svg format."
        );
        add_varargs_method("writeDXFView",&Module::writeDXFView,
            "writeDXFView(view,filename): Exports a DrawViewPart to a DXF file."
        );
        add_varargs_method("writeDXFPage",&Module::writeDXFPage,
            "writeDXFPage(page,filename): Exports a DrawPage to a DXF file."
        );
        add_varargs_method("findCentroid",&Module::findCentroid,
            "vector = findCentroid(shape,direction): finds geometric centroid of shape looking in direction."
        );
        initialize("This is a module for making drawings"); // register with Python
    }
    virtual ~Module() {}

private:
    virtual Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args)
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
        PyObject *pcObj;
        PyObject *inclBig = Py_True;
        if (!PyArg_ParseTuple(args.ptr(), "O!|O", &(PyList_Type), &pcObj, &inclBig)) {
            throw Py::TypeError("expected (listofedges,boolean");
        }

        std::vector<TopoDS_Edge> edgeList;
        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeEdgePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    const TopoDS_Edge e = TopoDS::Edge(sh);
                    edgeList.push_back(e);
                }
            }
        }
        catch (Standard_Failure& e) {
    
            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            Base::Console().Log("LOG - edgeWalker: input is empty\n");
            return Py::None();
        }

        bool biggie;
        if (inclBig == Py_True) {
            biggie = true;
        } else {
            biggie = false;
        }
        PyObject* result = PyList_New(0);

        try {
            EdgeWalker ew;
            ew.loadEdges(edgeList);
            bool success = ew.perform();
            if (success) {
                std::vector<TopoDS_Wire> rw = ew.getResultNoDups();
                std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(rw,biggie);   //false==>do not include biggest wires
                for (auto& w:sortedWires) {
                    PyList_Append(result,new TopoShapeWirePy(new TopoShape(w)));
                }
            } else {
                Base::Console().Warning("edgeWalker: input is not planar graph. Wire detection not done\n");
            }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }
        return Py::asObject(result);
    }

    Py::Object findOuterWire(const Py::Tuple& args)
    {
        PyObject *pcObj;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(PyList_Type), &pcObj)) {
            throw Py::TypeError("expected (listofedges)");
        }

        std::vector<TopoDS_Edge> edgeList;

        try {
            Py::Sequence list(pcObj);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeEdgePy::Type))) {
                    const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                        getTopoShapePtr()->getShape();
                    const TopoDS_Edge e = TopoDS::Edge(sh);
                    edgeList.push_back(e);
                }
            }
        }
        catch (Standard_Failure& e) {
    
            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            Base::Console().Log("LOG - findOuterWire: input is empty\n");
            return Py::None();
        }

        PyObject* outerWire = nullptr;
        bool success = false;
        try {
            EdgeWalker ew;
            ew.loadEdges(edgeList);
            success = ew.perform();
            if (success) {
                std::vector<TopoDS_Wire> rw = ew.getResultNoDups();
                std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(rw,true);
                outerWire = new TopoShapeWirePy(new TopoShape(*sortedWires.begin()));
            } else {
                Base::Console().Warning("findOuterWire: input is not planar graph. Wire detection not done\n");
            }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }
        if (!success) {
            return Py::None();
        }
        return Py::asObject(outerWire);
    }

    Py::Object findShapeOutline(const Py::Tuple& args)
    {
        PyObject *pcObjShape;
        double scale;
        PyObject *pcObjDir;
        if (!PyArg_ParseTuple(args.ptr(), "OdO", &pcObjShape,
                                                 &scale,
                                                 &pcObjDir)) {
            throw Py::TypeError("expected (shape,scale,direction");
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
            edgeList = DrawProjectSplit::getEdgesForWalker(shape,scale,dir);
        }
        catch (Standard_Failure& e) {
    
            throw Py::Exception(Part::PartExceptionOCCError, e.GetMessageString());
        }

        if (edgeList.empty()) {
            Base::Console().Log("LOG - ATDP::findShapeOutline: input is empty\n");
            return Py::None();
        }

        PyObject* outerWire = nullptr;
        bool success = false;
        try {
            EdgeWalker ew;
            ew.loadEdges(edgeList);
            success = ew.perform();
            if (success) {
                std::vector<TopoDS_Wire> rw = ew.getResultNoDups();
                std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(rw,true);
                outerWire = new TopoShapeWirePy(new TopoShape(*sortedWires.begin()));
            } else {
                Base::Console().Warning("ATDP::findShapeOutline: input is not planar graph. Wire detection not done\n");
            }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }
        if (!success) {
            return Py::None();
        }
        return Py::asObject(outerWire);
    }

    Py::Object viewPartAsDxf(const Py::Tuple& args)
    {
        PyObject *viewObj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &viewObj)) {
            throw Py::TypeError("expected (DrawViewPart)");
        } 
        Py::String dxfReturn;

        try {
            App::DocumentObject* obj = 0;
            TechDraw::DrawViewPart* dvp = 0;
            Drawing::DXFOutput dxfOut;
            std::string dxfText; 
            std::stringstream ss;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);
                TechDraw::GeometryObject* go = dvp->getGeometryObject();
                TopoDS_Shape s = TechDraw::mirrorShape(go->getVisHard());
                ss << dxfOut.exportEdges(s);
                s = TechDraw::mirrorShape(go->getVisOutline());
                ss << dxfOut.exportEdges(s);
                if (dvp->SmoothVisible.getValue()) {
                    s = TechDraw::mirrorShape(go->getVisSmooth());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SeamVisible.getValue()) {
                    s = TechDraw::mirrorShape(go->getVisSeam());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->HardHidden.getValue()) {
                    s = TechDraw::mirrorShape(go->getHidHard());
                    ss << dxfOut.exportEdges(s);
                    s = TechDraw::mirrorShape(go->getHidOutline());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SmoothHidden.getValue()) {
                    s = TechDraw::mirrorShape(go->getHidSmooth());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SeamHidden.getValue()) {
                    s = TechDraw::mirrorShape(go->getHidSeam());
                    ss << dxfOut.exportEdges(s);
                }
                // ss now contains all edges as Dxf
                dxfReturn = Py::String(ss.str());
           }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }

        return dxfReturn;
    }

    Py::Object viewPartAsSvg(const Py::Tuple& args)
    {
        PyObject *viewObj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &viewObj)) {
            throw Py::TypeError("expected (DrawViewPart)");
        } 
        Py::String svgReturn;
        std::string grpHead1 = "<g fill=\"none\" stroke=\"#000000\" stroke-opacity=\"1\" stroke-width=\"";
        std::string grpHead2 = "\" stroke-linecap=\"butt\" stroke-linejoin=\"miter\" stroke-miterlimit=\"4\">\n";
        std::string grpTail  = "</g>\n";
        try {
            App::DocumentObject* obj = 0;
            TechDraw::DrawViewPart* dvp = 0;
            Drawing::SVGOutput svgOut;
            std::string svgText; 
            std::stringstream ss;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);
                TechDraw::GeometryObject* go = dvp->getGeometryObject();
                //visible group begin "<g ... >"
                ss << grpHead1;
//                double thick = dvp->LineWidth.getValue();
                double thick = DrawUtil::getDefaultLineWeight("Thick");
                ss << thick;
                ss << grpHead2;
                TopoDS_Shape s = go->getVisHard();
                ss << svgOut.exportEdges(s);
                s = (go->getVisOutline());
                ss << svgOut.exportEdges(s);
                if (dvp->SmoothVisible.getValue()) {
                    s = go->getVisSmooth();
                    ss << svgOut.exportEdges(s);
                }
                if (dvp->SeamVisible.getValue()) {
                    s = go->getVisSeam();
                    ss << svgOut.exportEdges(s);
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
                        s = go->getHidHard();
                        ss << svgOut.exportEdges(s);
                        s = go->getHidOutline();
                        ss << svgOut.exportEdges(s);
                    }
                    if (dvp->SmoothHidden.getValue()) {
                        s = go->getHidSmooth();
                        ss << svgOut.exportEdges(s);
                    }
                    if (dvp->SeamHidden.getValue()) {
                        s = go->getHidSeam();
                        ss << svgOut.exportEdges(s);
                    }
                    ss << grpTail;
                    //hidden group end
                }
                // ss now contains all edges as Svg
                svgReturn = Py::String(ss.str());
           }
        }
        catch (Base::Exception &e) {
            throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());
        }

        return svgReturn;
    }

    void write1ViewDxf( ImpExpDxfWrite& writer, TechDraw::DrawViewPart* dvp, bool alignPage)
    {
        TechDraw::GeometryObject* go = dvp->getGeometryObject();
        TopoDS_Shape s = TechDraw::mirrorShape(go->getVisHard());
        double offX = 0.0;
        double offY = 0.0;
        if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(dvp);
            TechDraw::DrawProjGroup*      dpg = dpgi->getPGroup();
            if (dpg != nullptr) {
                offX = dpg->X.getValue();
                offY = dpg->Y.getValue();
            }
        }
        double dvpX,dvpY;
        if (alignPage) {
            dvpX = dvp->X.getValue() + offX;
            dvpY = dvp->Y.getValue() + offY;
        } else {
            dvpX = 0.0;
            dvpY = 0.0;
        }
        gp_Trsf xLate;
        xLate.SetTranslation(gp_Vec(dvpX,dvpY,0.0));
        BRepBuilderAPI_Transform mkTrf(s, xLate);
        s = mkTrf.Shape();                
        writer.exportShape(s);
        s = TechDraw::mirrorShape(go->getVisOutline());
        mkTrf.Perform(s);
        s = mkTrf.Shape();
        writer.exportShape(s);
        if (dvp->SmoothVisible.getValue()) {
            s = TechDraw::mirrorShape(go->getVisSmooth());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
        }
        if (dvp->SeamVisible.getValue()) {
            s = TechDraw::mirrorShape(go->getVisSeam());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
        }
        if (dvp->HardHidden.getValue()) {
            s = TechDraw::mirrorShape(go->getHidHard());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
            s = TechDraw::mirrorShape(go->getHidOutline());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
        }
        if (dvp->SmoothHidden.getValue()) {
            s = TechDraw::mirrorShape(go->getHidSmooth());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
        }
        if (dvp->SeamHidden.getValue()) {
            s = TechDraw::mirrorShape(go->getHidSeam());
            mkTrf.Perform(s);
            s = mkTrf.Shape();
            writer.exportShape(s);
        }
    }
    
    Py::Object writeDXFView(const Py::Tuple& args)
    {
        PyObject *viewObj;
        char* name;
        PyObject *alignObj = Py_True;
        if (!PyArg_ParseTuple(args.ptr(), "Oet|O", &viewObj, "utf-8",&name,&alignObj)) {
            throw Py::TypeError("expected (view,path");
        } 
        
        std::string filePath = std::string(name);
        std::string layerName = "none";
        PyMem_Free(name);
        bool align;
        if (alignObj == Py_True) {
            align = true;
        } else {
            align = false;
        }

        try {
            ImpExpDxfWrite writer(filePath);
            writer.init();
            App::DocumentObject* obj = 0;
            TechDraw::DrawViewPart* dvp = 0;
            if (PyObject_TypeCheck(viewObj, &(TechDraw::DrawViewPartPy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(viewObj)->getDocumentObjectPtr();
                dvp = static_cast<TechDraw::DrawViewPart*>(obj);
                
                layerName = dvp->getNameInDocument();
                writer.setLayerName(layerName);
                write1ViewDxf(writer,dvp,align);
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
        PyObject *pageObj;
        char* name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet", &pageObj, "utf-8",&name)) {
            throw Py::TypeError("expected (page,path");
        }
        
        std::string filePath = std::string(name);
        std::string layerName = "none";
        PyMem_Free(name);

        try {
            ImpExpDxfWrite writer(filePath);
            writer.init();
            App::DocumentObject* obj = 0;
            TechDraw::DrawPage* dp = 0;
            if (PyObject_TypeCheck(pageObj, &(TechDraw::DrawPagePy::Type))) {
                obj = static_cast<App::DocumentObjectPy*>(pageObj)->getDocumentObjectPtr();
                dp = static_cast<TechDraw::DrawPage*>(obj);
                auto views = dp->getAllViews();
                for (auto& v: views) {
                    if (v->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
                        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(v);
                        layerName = dvp->getNameInDocument();
                        writer.setLayerName(layerName);
                        write1ViewDxf(writer,dvp,true);
                    } else if (v->isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId())) {
                        TechDraw::DrawViewAnnotation* dva = static_cast<TechDraw::DrawViewAnnotation*>(v);
                        layerName = dva->getNameInDocument();
                        writer.setLayerName(layerName);
                        double height = dva->TextSize.getValue();  //mm
                        int just = 1;                              //centered
                        double x = dva->X.getValue();
                        double y = dva->Y.getValue();
                        Base::Vector3d loc(x,y,0.0);
                        auto lines = dva->Text.getValues();
                        writer.exportText(lines[0].c_str(),loc,loc, height,just);
                    } else if (v->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
                        DrawViewDimension* dvd = static_cast<TechDraw::DrawViewDimension*>(v);
                        TechDraw::DrawViewPart* dvp = dvd->getViewPart();
                        if (dvp == nullptr) {
                            continue;
                        }
                        double grandParentX = 0.0;
                        double grandParentY = 0.0;
                        if (dvp->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
                            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(dvp);
                            TechDraw::DrawProjGroup* dpg = dpgi->getPGroup();
                            if (dpg == nullptr) {
                                continue;
                            }
                            grandParentX = dpg->X.getValue();
                            grandParentY = dpg->Y.getValue();
                        }
                        double parentX = dvp->X.getValue() + grandParentX;
                        double parentY = dvp->Y.getValue() + grandParentY;
                        Base::Vector3d parentPos(parentX,parentY,0.0);
                        std::string sDimText = dvd->getFormatedValue();
                        char* dimText = &sDimText[0u];                  //hack for const-ness
                        float gap = 5.0;                                //hack. don't know font size here.
                        layerName = dvd->getNameInDocument();
                        writer.setLayerName(layerName);
                        if ( dvd->Type.isValue("Distance")  ||
                             dvd->Type.isValue("DistanceX") ||
                             dvd->Type.isValue("DistanceY") )  {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            Base::Vector3d lineLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY,0.0);
                            pointPair pts = dvd->getLinearPoints();
                            Base::Vector3d dimLine = pts.first - pts.second;
                            Base::Vector3d norm(-dimLine.y,dimLine.x,0.0);
                            norm.Normalize();
                            lineLocn = lineLocn + (norm * gap);
                            Base::Vector3d extLine1Start = Base::Vector3d(pts.first.x,-pts.first.y,0.0) + 
                                                           Base::Vector3d(parentX,parentY,0.0);
                            Base::Vector3d extLine2Start = Base::Vector3d(pts.second.x, -pts.second.y, 0.0) + 
                                                           Base::Vector3d(parentX,parentY,0.0);
                            writer.exportLinearDim(textLocn, lineLocn, extLine1Start, extLine2Start, dimText);
                        } else if (dvd->Type.isValue("Angle")) {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            Base::Vector3d lineLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY,0.0);
                            anglePoints pts = dvd->getAnglePoints();
                            Base::Vector3d end1 = pts.ends.first;
                            end1.y = -end1.y;
                            Base::Vector3d end2 = pts.ends.second;
                            end2.y = -end2.y;

                            Base::Vector3d apex = pts.vertex;
                            apex.y = -apex.y;
                            apex = apex + parentPos;

                            Base::Vector3d dimLine = end2 - end1;
                            Base::Vector3d norm(-dimLine.y,dimLine.x,0.0);
                            norm.Normalize();
                            lineLocn = lineLocn + (norm * gap);
                            end1 = end1 + parentPos;
                            end2 = end2 + parentPos;
                            writer.exportAngularDim(textLocn, lineLocn, end1, end2, apex, dimText);
                        } else if (dvd->Type.isValue("Radius")) {
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            arcPoints pts = dvd->getArcPoints();
                            Base::Vector3d center = pts.center;
                            center.y = -center.y;
                            Base::Vector3d arcPoint = pts.onCurve.first;
                            arcPoint.y = -arcPoint.y;
                            center = center + parentPos;
                            arcPoint = arcPoint + parentPos;
                            writer.exportRadialDim(center, textLocn, arcPoint, dimText);
                        } else if(dvd->Type.isValue("Diameter")){
                            Base::Vector3d textLocn(dvd->X.getValue() + parentX, dvd->Y.getValue() + parentY, 0.0);
                            arcPoints pts = dvd->getArcPoints();
                            Base::Vector3d end1 = pts.onCurve.first;
                            end1.y = -end1.y;
                            Base::Vector3d end2 = pts.onCurve.second;
                            end2.y = -end2.y;
                            end1 = end1 + parentPos;
                            end2 = end2 + parentPos;
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
        PyObject *pcObjShape;
        PyObject *pcObjDir;
        if (!PyArg_ParseTuple(args.ptr(), "OO", &pcObjShape,
                                                &pcObjDir)) {
            throw Py::TypeError("expected (shape,direction");
        }

        if (!PyObject_TypeCheck(pcObjShape, &(TopoShapePy::Type))) {
            throw Py::TypeError("expected arg1 to be 'Shape'");
        }

        if (!PyObject_TypeCheck(pcObjDir, &(Base::VectorPy::Type))) {
            throw Py::TypeError("expected arg2 to be 'Vector'");
        }

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        if (!pShape) {
            Base::Console().Error("TechDraw::findCentroid - input shape is null\n");
            return Py::None();
        }

        const TopoDS_Shape& shape = pShape->getTopoShapePtr()->getShape();
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pcObjDir)->value();
        Base::Vector3d c = TechDraw::findCentroidVec(shape,dir);
        PyObject* result = nullptr;
        result = new Base::VectorPy(new Base::Vector3d(c));
        return Py::asObject(result);
    }

 };

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace TechDraw
