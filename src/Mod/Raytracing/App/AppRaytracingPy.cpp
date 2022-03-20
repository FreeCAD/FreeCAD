/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include "PovTools.h"
#include "LuxTools.h"
// automatically generated.....
#include "FreeCADpov.h"

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <App/Application.h>


using namespace std;


namespace Raytracing {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Raytracing")
    {
        add_varargs_method("writeProjectFile",&Module::writeProjectFile
        );
        add_varargs_method("getProjectFile",&Module::getProjectFile
        );
        add_varargs_method("getPartAsPovray",&Module::getPartAsPovray
        );
        add_varargs_method("getPartAsLux",&Module::getPartAsLux
        );
        add_varargs_method("writePartFile",&Module::writePartFile
        );
        add_varargs_method("writeDataFile",&Module::writeDataFile
        );
        add_varargs_method("writePartFileCSV",&Module::writePartFileCSV
        );
        add_varargs_method("writeCameraFile",&Module::writeCameraFile
        );
        add_varargs_method("copyResource",&Module::copyResource
        );
        initialize("This module is the Raytracing module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object writeProjectFile(const Py::Tuple& args)
    {
        char *fromPython;
        if (! PyArg_ParseTuple(args.ptr(), "(s)", &fromPython))
            throw Py::Exception();

        std::ofstream fout;
        if (fromPython)
            fout.open(fromPython);
        else
          fout.open("FreeCAD.pov");

        fout << FreeCAD ;
        fout.close();
        return Py::None();
    }
    Py::Object getProjectFile(const Py::Tuple& /*args*/)
    {
        return Py::String(FreeCAD);
    }
    Py::Object getPartAsPovray(const Py::Tuple& args)
    {
        float r=0.5,g=0.5,b=0.5;
        PyObject *ShapeObject;
        const char *PartName;
        if (! PyArg_ParseTuple(args.ptr(), "sO!|fff",&PartName,
            &(Part::TopoShapePy::Type), &ShapeObject,&r,&g,&b)) 
            throw Py::Exception();

        std::stringstream out;
        const TopoDS_Shape &aShape = static_cast<Part::TopoShapePy *>(ShapeObject)->getTopoShapePtr()->getShape();

        PovTools::writeShape(out,PartName,aShape,(float)0.1);
        // This must not be done in PovTools::writeShape!
        out << "// instance to render" << endl
            << "object {" << PartName << endl
            << "  texture {" << endl
            << "      pigment {color rgb <"<<r<<","<<g<<","<<b<<">}" << endl
            << "      finish {StdFinish } //definition on top of the project" << endl
            << "  }" << endl
            << "}" << endl   ;
        return Py::String(out.str());
    }
    Py::Object getPartAsLux(const Py::Tuple& args)
    {
        float r=0.5,g=0.5,b=0.5;
        PyObject *ShapeObject;
        const char *PartName;
        if (! PyArg_ParseTuple(args.ptr(), "sO!|fff",&PartName,
            &(Part::TopoShapePy::Type), &ShapeObject,&r,&g,&b)) 
            throw Py::Exception();

        std::stringstream out;
        const TopoDS_Shape &aShape = static_cast<Part::TopoShapePy *>(ShapeObject)->getTopoShapePtr()->getShape();

        // write a material entry
        // This must not be done in PovTools::writeShape!
        out << "MakeNamedMaterial \"FreeCADMaterial_" << PartName << "\"" << endl;
        out << "    \"color Kd\" [" << r << " " << g << " " << b << "]" << endl;
        out << "    \"float sigma\" [0.000000000000000]" << endl;
        out << "    \"string type\" [\"matte\"]" << endl << endl;

        LuxTools::writeShape(out,PartName,aShape,(float)0.1);
        return Py::String(out.str());
    }
    Py::Object writePartFile(const Py::Tuple& args)
    {
        PyObject *ShapeObject;
        const char *FileName,*PartName;
        if (! PyArg_ParseTuple(args.ptr(), "ssO!",&FileName,&PartName,
            &(Part::TopoShapePy::Type), &ShapeObject)) 
            throw Py::Exception();

        const TopoDS_Shape &aShape = static_cast<Part::TopoShapePy *>(ShapeObject)->getTopoShapePtr()->getShape();

        PovTools::writeShape(FileName,PartName,aShape,(float)0.1);

        return Py::None();
    }
    Py::Object writeDataFile(const Py::Tuple& args)
    {
        PyObject *dataObject;
        const char *FileName,*PartName;
        if (! PyArg_ParseTuple(args.ptr(), "ssO!",&FileName,&PartName,
            &(Data::ComplexGeoDataPy::Type), &dataObject)) 
            throw Py::Exception();

        const Data::ComplexGeoData* aData = static_cast<Data::ComplexGeoDataPy *>
            (dataObject)->getComplexGeoDataPtr();

        PovTools::writeData(FileName,PartName,aData,0.1f);

        return Py::None();
    }
    Py::Object writePartFileCSV(const Py::Tuple& args)
    {
        PyObject *ShapeObject;
        const char *FileName;
        float Acur,Length;
        if (!PyArg_ParseTuple(args.ptr(), "O!sff",&(Part::TopoShapePy::Type),
            &ShapeObject,&FileName,&Acur,&Length))
            throw Py::Exception();

        TopoDS_Shape aShape = static_cast<Part::TopoShapePy *>(ShapeObject)->getTopoShapePtr()->getShape();
        PovTools::writeShapeCSV(FileName,aShape,Acur,Length);
        return Py::None();
    }
    Py::Object writeCameraFile(const Py::Tuple& args)
    {
        PyObject *Arg[4];
        const char *FileName;
        double vecs[4][3];
        if (!PyArg_ParseTuple(args.ptr(), "sO!O!O!O!",&FileName,&PyTuple_Type,
            &Arg[0],&PyTuple_Type, &Arg[1],&PyTuple_Type, &Arg[2],&PyTuple_Type, &Arg[3])) 
            throw Py::Exception();

        // go through the Tuple of Tuples
        for (int i=0;i<4;i++) {
            // check the right size of the Tuple of floats
            if (PyTuple_GET_SIZE(Arg[i]) != 3)
                throw Py::ValueError("Wrong parameter format, four Tuple of three floats needed!");

            // go through the Tuple of floats
            for (int l=0;l<3;l++) {
                PyObject* temp = PyTuple_GetItem(Arg[i],l);
                // check Type
                if (PyFloat_Check(temp))
                    vecs[i][l] = PyFloat_AsDouble(temp);
                else if (PyLong_Check(temp))
                    vecs[i][l] = (double) PyLong_AsLong(temp);
                else
                    throw Py::ValueError("Wrong parameter format, four Tuple of three floats needed!");
            }
        }

        // call the write method of PovTools....
        PovTools::writeCamera(FileName,CamDef(gp_Vec(vecs[0][0],vecs[0][1],vecs[0][2]),
                                              gp_Vec(vecs[1][0],vecs[1][1],vecs[1][2]),
                                              gp_Vec(vecs[2][0],vecs[2][1],vecs[2][2]),
                                              gp_Vec(vecs[3][0],vecs[3][1],vecs[3][2])));

        return Py::None();
    }
    Py::Object copyResource(const Py::Tuple& args)
    {
        const char *FileName,*DestDir;
        if (! PyArg_ParseTuple(args.ptr(), "ss",&FileName,&DestDir))
            throw Py::Exception();

        std::string resName = App::Application::getHomePath();
        resName += "Mod"; 
        resName += PATHSEP ;
        resName += "Raytracing"; 
        resName += PATHSEP ;
        resName += "resources"; 
        resName += PATHSEP;
        resName += FileName;

        Base::Console().Warning("Using fileName = %s\nRaytracer scene file not generated "
                                "because function is not implemented yet.\nYou can copy "
                                "the standard scene file FreeCAD.pov to your raytracing "
                                "directory to render the scene.\n",resName.c_str());

    // This command should create the povray scene file, but does currently do nothing.

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Raytracing
