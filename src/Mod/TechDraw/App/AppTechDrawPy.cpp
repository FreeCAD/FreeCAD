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
# include <Python.h>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
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

#include "DrawProjectSplit.h"
#include "DrawViewPart.h"
#include "DrawViewPartPy.h"
#include "GeometryObject.h"
#include "EdgeWalker.h"
#include "DrawUtil.h"


namespace TechDraw {
//module level static C++ functions go here
}

using Part::TopoShape;
using Part::TopoShapePy;
using Part::TopoShapeEdgePy;
using Part::TopoShapeWirePy;

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
            throw Py::Exception("expected (listofedges,boolean");
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
            throw Py::Exception("expected (listofedges)");
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
            throw Py::Exception("expected (shape,scale,direction");
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
            throw Py::Exception("expected (DrawViewPart)");
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
                TechDrawGeometry::GeometryObject* go = dvp->getGeometryObject();
                TopoDS_Shape s = TechDrawGeometry::mirrorShape(go->getVisHard());
                ss << dxfOut.exportEdges(s);
                s = TechDrawGeometry::mirrorShape(go->getVisOutline());
                ss << dxfOut.exportEdges(s);
                if (dvp->SmoothVisible.getValue()) {
                    s = TechDrawGeometry::mirrorShape(go->getVisSmooth());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SeamVisible.getValue()) {
                    s = TechDrawGeometry::mirrorShape(go->getVisSeam());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->HardHidden.getValue()) {
                    s = TechDrawGeometry::mirrorShape(go->getHidHard());
                    ss << dxfOut.exportEdges(s);
                    s = TechDrawGeometry::mirrorShape(go->getHidOutline());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SmoothHidden.getValue()) {
                    s = TechDrawGeometry::mirrorShape(go->getHidSmooth());
                    ss << dxfOut.exportEdges(s);
                }
                if (dvp->SeamHidden.getValue()) {
                    s = TechDrawGeometry::mirrorShape(go->getHidSeam());
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
            throw Py::Exception("expected (DrawViewPart)");
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
                TechDrawGeometry::GeometryObject* go = dvp->getGeometryObject();
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

 };

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace TechDraw
