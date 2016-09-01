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

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>

#include <Mod/Part/App/OCCError.h>

#include "EdgeWalker.h"

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
        if (!PyArg_ParseTuple(args.ptr(), "O|O", &pcObj,&inclBig)) {
                throw Py::Exception();
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
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(Part::PartExceptionOCCError, e->GetMessageString());
        }

        bool biggie;
        if (inclBig == Py_True) {
            biggie = true;
        } else {
            biggie = false;
        }
        PyObject* result = PyList_New(0);

        EdgeWalker ew;
        ew.loadEdges(edgeList);
        ew.perform();
        std::vector<TopoDS_Wire> rw = ew.getResultWires();
        std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(rw,biggie);   //false==>do not include biggest wires
        for (auto& w:sortedWires) {
            PyList_Append(result,new TopoShapeWirePy(new TopoShape(w)));
        }

        return Py::asObject(result);
    }

    Py::Object findOuterWire(const Py::Tuple& args)
    {
        PyObject *pcObj;
        if (!PyArg_ParseTuple(args.ptr(), "O", &pcObj)) {
                throw Py::Exception();
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
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(Part::PartExceptionOCCError, e->GetMessageString());
        }

        PyObject* result = PyList_New(0);

        EdgeWalker ew;
        ew.loadEdges(edgeList);
        ew.perform();
        std::vector<TopoDS_Wire> rw = ew.getResultWires();
       std::vector<TopoDS_Wire> sortedWires = ew.sortStrip(rw,true);
        for (auto& w:sortedWires) {
            PyList_Append(result,new TopoShapeWirePy(new TopoShape(w)));
        }
        PyObject* outerWire = PyList_GetItem(result, 0);
        return Py::asObject(outerWire);
    }
 };

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace TechDraw
