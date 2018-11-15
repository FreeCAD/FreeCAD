/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <memory>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/VectorPy.h>
#include <Base/PlacementPy.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
//#include <Mod/Mesh/App/Core/MeshKernel.h>
//#include <Mod/Mesh/App/Core/Evaluation.h>
//#include <Mod/Mesh/App/Core/Iterator.h>

#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDS_MeshNode.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>

#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#include <StdMeshers_Arithmetic1D.hxx>

#include "FemMesh.h"
#include "FemMeshObject.h"
#include "FemMeshPy.h"
#ifdef FC_USE_VTK
#include "FemPostPipeline.h"
#include "FemVTKTools.h"
#endif

#include <cstdlib>

#include <Standard_Real.hxx>
#include <Base/Vector3D.h>
#include <Mod/Part/App/OCCError.h>

namespace Fem {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Fem")
    {
        add_varargs_method("open",&Module::open,
            "open(string) -- Create a new document and a Mesh::Import feature to load the file into the document."
        );
        add_varargs_method("insert",&Module::insert,
            "insert(string|mesh,[string]) -- Load or insert a mesh into the given or active document."
        );
        add_varargs_method("export",&Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        add_varargs_method("read",&Module::read,
            "Read a mesh from a file and returns a Mesh object."
        );
#ifdef FC_USE_VTK
        add_varargs_method("readResult",&Module::readResult,
            "Read a CFD or Mechanical result (auto detect) from a file (file format detected from file suffix)"
        );
        add_varargs_method("writeResult",&Module::writeResult,
            "write a CFD or FEM result (auto detect) to a file (file format detected from file suffix)"
        );
#endif
        add_varargs_method("show",&Module::show,
            "show(shape,[string]) -- Add the mesh to the active document or create one if no document exists."
        );
        initialize("This module is the Fem module."); // register with Python
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
            throw Py::Exception(Part::PartExceptionOCCError, str);
        }
        catch (const Base::Exception &e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const std::exception &e) {
            throw Py::RuntimeError(e.what());
        }
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        std::unique_ptr<FemMesh> mesh(new FemMesh);
        mesh->read(EncodedName.c_str());
        Base::FileInfo file(EncodedName.c_str());
        // create new document and add Import feature
        App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
        FemMeshObject *pcFeature = static_cast<FemMeshObject *>
            (pcDoc->addObject("Fem::FemMeshObject", file.fileNamePure().c_str()));
        pcFeature->Label.setValue(file.fileNamePure().c_str());
        pcFeature->FemMesh.setValuePtr(mesh.release());
        pcFeature->purgeTouched();

        return Py::None();
    }
    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName = 0;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        App::Document *pcDoc = 0;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();

        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        Base::FileInfo file(EncodedName.c_str());

        try {
            std::unique_ptr<FemMesh> mesh(new FemMesh);
            mesh->read(EncodedName.c_str());

            FemMeshObject *pcFeature = static_cast<FemMeshObject *>
                (pcDoc->addObject("Fem::FemMeshObject", file.fileNamePure().c_str()));
            pcFeature->Label.setValue(file.fileNamePure().c_str());
            pcFeature->FemMesh.setValuePtr(mesh.release());
            pcFeature->purgeTouched();
        }
        catch (Base::Exception&) {
#ifdef FC_USE_VTK
            if (FemPostPipeline::canRead(file)) {

                FemPostPipeline *pcFeature = static_cast<FemPostPipeline *>
                    (pcDoc->addObject("Fem::FemPostPipeline", file.fileNamePure().c_str()));

                pcFeature->Label.setValue(file.fileNamePure().c_str());
                pcFeature->read(file);
                pcFeature->touch();
                pcDoc->recomputeFeature(pcFeature);
            }
            else {
                throw;
            }
#else
            throw;
#endif
        }

        return Py::None();
    }
    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "Oet",&object,"utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Py::Sequence list(object);
        Base::Type meshId = Base::Type::fromName("Fem::FemMeshObject");
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(meshId)) {
                    static_cast<FemMeshObject*>(obj)->FemMesh.getValue().write(EncodedName.c_str());
                    break;
                }
            }
        }

        return Py::None();
    }
    Py::Object read(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        std::unique_ptr<FemMesh> mesh(new FemMesh);
        mesh->read(EncodedName.c_str());
        return Py::asObject(new FemMeshPy(mesh.release()));
    }

#ifdef FC_USE_VTK
    Py::Object readResult(const Py::Tuple& args)
    {
        char* fileName = NULL;
        char* objName = NULL;

        if (!PyArg_ParseTuple(args.ptr(), "et|et","utf-8", &fileName, "utf-8", &objName))
            throw Py::Exception();
        std::string EncodedName = std::string(fileName);
        PyMem_Free(fileName);
        std::string resName = std::string(objName);
        PyMem_Free(objName);

        if (resName.length())
        {
            App::Document* pcDoc = App::GetApplication().getActiveDocument();
            App::DocumentObject* obj = pcDoc->getObject(resName.c_str());
            FemVTKTools::readResult(EncodedName.c_str(), obj);
        }
        else
            FemVTKTools::readResult(EncodedName.c_str());  // assuming activeObject can hold Result

        return Py::None();
    }

    Py::Object writeResult(const Py::Tuple& args)
    {
        char* fileName = NULL;
        PyObject *pcObj = NULL;

        if (!PyArg_ParseTuple(args.ptr(), "et|O!","utf-8", &fileName, &(App::DocumentObjectPy::Type), &pcObj))
            throw Py::Exception();
        std::string EncodedName = std::string(fileName);
        PyMem_Free(fileName);

        if (pcObj)
        {
            if (PyObject_TypeCheck(pcObj, &(App::DocumentObjectPy::Type)))
            {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(pcObj)->getDocumentObjectPtr();
                FemVTKTools::writeResult(EncodedName.c_str(), obj);
            }
        }
        else
            FemVTKTools::writeResult(EncodedName.c_str());

        return Py::None();
    }
#endif

    Py::Object show(const Py::Tuple& args)
    {
        PyObject *pcObj;
        char *name = "Mesh";
        if (!PyArg_ParseTuple(args.ptr(), "O!|s", &(FemMeshPy::Type), &pcObj, &name))
            throw Py::Exception();

        App::Document *pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();

        FemMeshPy* pShape = static_cast<FemMeshPy*>(pcObj);
        Fem::FemMeshObject *pcFeature = static_cast<Fem::FemMeshObject*>
                (pcDoc->addObject("Fem::FemMeshObject", name));
        // copy the data
        pcFeature->FemMesh.setValue(*(pShape->getFemMeshPtr()));
        pcDoc->recompute();

        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Fem
