/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Mod/Part/App/TopoShapePy.h>
#include "ProjectionAlgos.h"
#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <boost/regex.hpp>

#include <Mod/Part/App/OCCError.h>

using Part::TopoShapePy;
using Part::TopoShape;

namespace Drawing {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Drawing")
    {
        add_varargs_method("project",&Module::project,
            "[visiblyG0,visiblyG1,hiddenG0,hiddenG1] = project(TopoShape[,App.Vector Direction, string type])\n"
            " -- Project a shape and return the visible/invisible parts of it."
        );
        add_varargs_method("projectEx",&Module::projectEx,
            "[V,V1,VN,VO,VI,H,H1,HN,HO,HI] = projectEx(TopoShape[,App.Vector Direction, string type])\n"
            " -- Project a shape and return the all parts of it."
        );
        add_varargs_method("projectToSVG",&Module::projectToSVG,
            "string = projectToSVG(TopoShape[,App.Vector Direction, string type])\n"
            " -- Project a shape and return the SVG representation as string."
        );
        add_varargs_method("projectToDXF",&Module::projectToDXF,
            "string = projectToDXF(TopoShape[,App.Vector Direction, string type])\n"
            " -- Project a shape and return the DXF representation as string."
        );
        add_varargs_method("removeSvgTags",&Module::removeSvgTags,
            "string = removeSvgTags(string) -- Removes the opening and closing svg tags\n"
            "and other metatags from a svg code, making it embeddable"
        );
        initialize("This module is the Drawing module."); // register with Python
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
    Py::Object project(const Py::Tuple& args)
    {
        PyObject *pcObjShape;
        PyObject *pcObjDir=0;

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!",
            &(Part::TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir))
            throw Py::Exception();

        Part::TopoShapePy* pShape = static_cast<Part::TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

        Py::List list;
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.V)) , true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.V1)), true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.H)) , true));
        list.append(Py::Object(new Part::TopoShapePy(new Part::TopoShape(Alg.H1)), true));

        return list;
    }
    Py::Object projectEx(const Py::Tuple& args)
    {
        PyObject *pcObjShape;
        PyObject *pcObjDir=0;

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!",
            &(TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

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
    Py::Object projectToSVG(const Py::Tuple& args)
    {
        PyObject *pcObjShape;
        PyObject *pcObjDir=0;
        const char *type=0;
        float scale=1.0f;
        float tol=0.1f;

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!sff",
            &(TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir, &type, &scale, &tol))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getSVG(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale, tol));
        return result;
    }
    Py::Object projectToDXF(const Py::Tuple& args)
    {
        PyObject *pcObjShape;
        PyObject *pcObjDir=0;
        const char *type=0;
        float scale=1.0f;
        float tol=0.1f;

        if (!PyArg_ParseTuple(args.ptr(), "O!|O!sff",
            &(TopoShapePy::Type), &pcObjShape,
            &(Base::VectorPy::Type), &pcObjDir, &type, &scale, &tol))
            throw Py::Exception();

        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getDXF(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale, tol));
        return result;
    }
    Py::Object removeSvgTags(const Py::Tuple& args)
    {
        const char* svgcode;
        if (!PyArg_ParseTuple(args.ptr(), "s",&svgcode))
            throw Py::Exception();

        std::string svg(svgcode);
        std::string empty = "";
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
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Drawing
