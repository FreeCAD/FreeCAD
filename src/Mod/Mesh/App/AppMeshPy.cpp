/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <algorithm>
#include <map>
#include <memory>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Interpreter.h>
#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>
#include "Core/Approximation.h"
#include "Core/Evaluation.h"
#include "Core/Iterator.h"
#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include "WildMagic4/Wm4ContBox3.h"

#include "Exporter.h"
#include "Importer.h"
#include "Mesh.h"
#include "MeshPy.h"


using namespace Mesh;
using namespace MeshCore;

namespace Mesh
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Mesh")
    {
        add_varargs_method("read",
                           &Module::read,
                           "Read a mesh from a file and returns a Mesh object.");
        add_varargs_method("open",
                           &Module::open,
                           "open(string)\n"
                           "Create a new document and a Mesh feature to load the file into\n"
                           "the document.");
        add_varargs_method("insert",
                           &Module::importer,
                           "insert(string|mesh,[string])\n"
                           "Load or insert a mesh into the given or active document.");
        add_keyword_method("export",
                           &Module::exporter,
                           "export(objects, filename, [tolerance=0.1, exportAmfCompressed=True])\n"
                           "Export a list of objects into a single file identified by filename.\n"
                           "tolerance is in mm and specifies the maximum acceptable deviation\n"
                           "between the specified objects and the exported mesh.\n"
                           "exportAmfCompressed specifies whether exported AMF files should be\n"
                           "compressed.\n");
        add_varargs_method("show",
                           &Module::show,
                           "show(shape,[string]) -- Add the mesh to the active document or create "
                           "one if no document exists.");
        add_varargs_method("createBox", &Module::createBox, "Create a solid mesh box");
        add_varargs_method("createPlane", &Module::createPlane, "Create a mesh XY plane normal +Z");
        add_varargs_method("createSphere", &Module::createSphere, "Create a tessellated sphere");
        add_varargs_method("createEllipsoid",
                           &Module::createEllipsoid,
                           "Create a tessellated ellipsoid");
        add_varargs_method("createCylinder",
                           &Module::createCylinder,
                           "Create a tessellated cylinder");
        add_varargs_method("createCone", &Module::createCone, "Create a tessellated cone");
        add_varargs_method("createTorus", &Module::createTorus, "Create a tessellated torus");
        add_varargs_method("calculateEigenTransform",
                           &Module::calculateEigenTransform,
                           "calculateEigenTransform(seq(Base.Vector))\n"
                           "Calculates the eigen Transformation from a list of points.\n"
                           "calculate the point's local coordinate system with the center\n"
                           "of gravity as origin. The local coordinate system is computed\n"
                           "this way that u has minimum and w has maximum expansion.\n"
                           "The local coordinate system is right-handed.\n");
        add_varargs_method("polynomialFit",
                           &Module::polynomialFit,
                           "polynomialFit(seq(Base.Vector)) -- Calculates a polynomial fit.");
        add_varargs_method(
            "minimumVolumeOrientedBox",
            &Module::minimumVolumeOrientedBox,
            "minimumVolumeOrientedBox(seq(Base.Vector)) -- Calculates the minimum\n"
            "volume oriented box containing all points. The return value is a\n"
            "tuple of seven items:\n"
            "    center, u, v, w directions and the lengths of the three vectors.\n");
        initialize("The functions in this module allow working with mesh objects.\n"
                   "A set of functions are provided for reading in registered mesh\n"
                   "file formats to either a new or existing document.\n"
                   "\n"
                   "open(string) -- Create a new document and a Mesh feature\n"
                   "                to load the file into the document.\n"
                   "insert(string, string) -- Create a Mesh feature to load\n"
                   "                          the file into the given document.\n"
                   "Mesh() -- Create an empty mesh object.\n"
                   "\n");
    }

private:
    Py::Object invoke_method_varargs(void* method_def, const Py::Tuple& args) override
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
        catch (const std::exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }
    Py::Object read(const Py::Tuple& args)
    {
        char* Name {};
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        std::unique_ptr<MeshObject> mesh(new MeshObject);
        mesh->load(EncodedName.c_str());
        return Py::asObject(new MeshPy(mesh.release()));
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name {};
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        // create new document and add Import feature
        App::Document* pcDoc = App::GetApplication().newDocument();

        Mesh::Importer import(pcDoc);
        import.load(EncodedName);

        return Py::None();
    }
    Py::Object importer(const Py::Tuple& args)
    {
        char* Name {};
        char* DocName = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        App::Document* pcDoc = nullptr;
        if (DocName) {
            pcDoc = App::GetApplication().getDocument(DocName);
        }
        else {
            pcDoc = App::GetApplication().getActiveDocument();
        }

        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        Mesh::Importer import(pcDoc);
        import.load(EncodedName);

        return Py::None();
    }

    Py::Object exporter(const Py::Tuple& args, const Py::Dict& keywds)
    {
        PyObject* objects {};
        char* fileNamePy {};

        // If tolerance is specified via python interface, use that.
        // If not, use the preference, if that exists, else default to 0.1mm.
        auto hGrp(App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Mesh"));
        auto fTolerance(hGrp->GetFloat("MaxDeviationExport", 0.1f));

        int exportAmfCompressed(hGrp->GetBool("ExportAmfCompressed", true));
        bool export3mfModel(hGrp->GetBool("Export3mfModel", true));

        static const std::array<const char*, 5> kwList {"objectList",
                                                        "filename",
                                                        "tolerance",
                                                        "exportAmfCompressed",
                                                        nullptr};

        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(),
                                                 keywds.ptr(),
                                                 "Oet|dp",
                                                 kwList,
                                                 &objects,
                                                 "utf-8",
                                                 &fileNamePy,
                                                 &fTolerance,
                                                 &exportAmfCompressed)) {
            throw Py::Exception();
        }

        std::string outputFileName(fileNamePy);
        PyMem_Free(fileNamePy);

        // Construct list of objects to export before making the Exporter, so
        // we don't get empty exports if the list can't be constructed.
        Py::Sequence list(objects);
        if (list.length() == 0) {
            return Py::None();
        }

        // collect all object types that can be exported as mesh
        std::vector<App::DocumentObject*> objectList;
        for (const auto& it : list) {
            PyObject* item = it.ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                auto obj(static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr());
                objectList.push_back(obj);
            }
        }

        if (objectList.empty()) {
            throw Py::TypeError("None of the objects can be exported to a mesh file");
        }

        auto exportFormat(MeshOutput::GetFormat(outputFileName.c_str()));

        std::unique_ptr<Exporter> exporter;
        if (exportFormat == MeshIO::AMF) {
            std::map<std::string, std::string> meta;
            meta["cad"] = App::Application::Config()["ExeName"] + " "
                + App::Application::Config()["ExeVersion"];
            meta[App::Application::Config()["ExeName"] + "-buildRevisionHash"] =
                App::Application::Config()["BuildRevisionHash"];

            exporter = std::make_unique<ExporterAMF>(outputFileName, meta, exportAmfCompressed);
        }
        else if (exportFormat == MeshIO::ThreeMF) {
            Extension3MFFactory::initialize();
            exporter = std::make_unique<Exporter3MF>(outputFileName,
                                                     Extension3MFFactory::createExtensions());
            dynamic_cast<Exporter3MF*>(exporter.get())->setForceModel(export3mfModel);
        }
        else if (exportFormat != MeshIO::Undefined) {
            exporter = std::make_unique<MergeExporter>(outputFileName, exportFormat);
        }
        else {
            std::string exStr("Can't determine mesh format from file name.\nPlease specify mesh "
                              "format file extension: '");
            exStr += outputFileName + "'";
            throw Py::ValueError(exStr.c_str());
        }

        for (auto it : objectList) {
            exporter->addObject(it, fTolerance);
        }

        exporter.reset();  // deletes Exporter, mesh file is written by destructor

        return Py::None();
    }

    Py::Object show(const Py::Tuple& args)
    {
        PyObject* pcObj {};
        char* name = "Mesh";
        if (!PyArg_ParseTuple(args.ptr(), "O!|s", &(MeshPy::Type), &pcObj, &name)) {
            throw Py::Exception();
        }

        App::Document* pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument();
        }
        MeshPy* pMesh = static_cast<MeshPy*>(pcObj);
        Mesh::Feature* pcFeature =
            static_cast<Mesh::Feature*>(pcDoc->addObject("Mesh::Feature", name));
        Mesh::MeshObject* mo = pMesh->getMeshObjectPtr();
        if (!mo) {
            throw Py::Exception(PyExc_ReferenceError, "object doesn't reference a valid mesh");
        }
        // copy the data
        pcFeature->Mesh.setValue(*mo);

        return Py::None();
    }
    Py::Object createBox(const Py::Tuple& args)
    {
        MeshObject* mesh = nullptr;

        do {
            float length = 10.0f;
            float width = 10.0f;
            float height = 10.0f;
            float edgelen = -1.0f;
            if (PyArg_ParseTuple(args.ptr(), "|ffff", &length, &width, &height, &edgelen)) {
                if (edgelen < 0.0f) {
                    mesh = MeshObject::createCube(length, width, height);
                }
                else {
                    mesh = MeshObject::createCube(length, width, height, edgelen);
                }
                break;
            }

            PyErr_Clear();
            PyObject* box {};
            if (PyArg_ParseTuple(args.ptr(), "O!", &Base::BoundBoxPy::Type, &box)) {
                Py::BoundingBox bbox(box, false);
                mesh = MeshObject::createCube(bbox.getValue());
                break;
            }

            throw Py::TypeError("Must be real numbers or BoundBox");
        } while (false);
        if (!mesh) {
            throw Py::RuntimeError("Creation of box failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object createPlane(const Py::Tuple& args)
    {
        float x = 1, y = 0, z = 0;
        if (!PyArg_ParseTuple(args.ptr(), "|fff", &x, &y, &z)) {
            throw Py::Exception();
        }

        if (y == 0) {
            y = x;
        }

        float hx = x / 2.0f;
        float hy = y / 2.0f;

        std::vector<MeshCore::MeshGeomFacet> TriaList;
        TriaList.emplace_back(Base::Vector3f(-hx, -hy, 0.0),
                              Base::Vector3f(hx, hy, 0.0),
                              Base::Vector3f(-hx, hy, 0.0));
        TriaList.emplace_back(Base::Vector3f(-hx, -hy, 0.0),
                              Base::Vector3f(hx, -hy, 0.0),
                              Base::Vector3f(hx, hy, 0.0));

        std::unique_ptr<MeshObject> mesh(new MeshObject);
        mesh->addFacets(TriaList);
        return Py::asObject(new MeshPy(mesh.release()));
    }
    Py::Object createSphere(const Py::Tuple& args)
    {
        float radius = 5.0f;
        int sampling = 50;
        if (!PyArg_ParseTuple(args.ptr(), "|fi", &radius, &sampling)) {
            throw Py::Exception();
        }

        MeshObject* mesh = MeshObject::createSphere(radius, sampling);
        if (!mesh) {
            throw Py::RuntimeError("Creation of sphere failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object createEllipsoid(const Py::Tuple& args)
    {
        float radius1 = 2.0f;
        float radius2 = 4.0f;
        int sampling = 50;
        if (!PyArg_ParseTuple(args.ptr(), "|ffi", &radius1, &radius2, &sampling)) {
            throw Py::Exception();
        }

        MeshObject* mesh = MeshObject::createEllipsoid(radius1, radius2, sampling);
        if (!mesh) {
            throw Py::RuntimeError("Creation of ellipsoid failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object createCylinder(const Py::Tuple& args)
    {
        float radius = 2.0f;
        float length = 10.0f;
        int closed = 1;
        float edgelen = 1.0f;
        int sampling = 50;
        if (!PyArg_ParseTuple(args.ptr(),
                              "|ffifi",
                              &radius,
                              &length,
                              &closed,
                              &edgelen,
                              &sampling)) {
            throw Py::Exception();
        }

        MeshObject* mesh = MeshObject::createCylinder(radius, length, closed, edgelen, sampling);
        if (!mesh) {
            throw Py::RuntimeError("Creation of cylinder failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object createCone(const Py::Tuple& args)
    {
        float radius1 = 2.0f;
        float radius2 = 4.0f;
        float len = 10.0f;
        int closed = 1;
        float edgelen = 1.0f;
        int sampling = 50;
        if (!PyArg_ParseTuple(args.ptr(),
                              "|fffifi",
                              &radius1,
                              &radius2,
                              &len,
                              &closed,
                              &edgelen,
                              &sampling)) {
            throw Py::Exception();
        }

        MeshObject* mesh = MeshObject::createCone(radius1, radius2, len, closed, edgelen, sampling);
        if (!mesh) {
            throw Py::RuntimeError("Creation of cone failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object createTorus(const Py::Tuple& args)
    {
        float radius1 = 10.0f;
        float radius2 = 2.0f;
        int sampling = 50;
        if (!PyArg_ParseTuple(args.ptr(), "|ffi", &radius1, &radius2, &sampling)) {
            throw Py::Exception();
        }

        MeshObject* mesh = MeshObject::createTorus(radius1, radius2, sampling);
        if (!mesh) {
            throw Py::RuntimeError("Creation of torus failed");
        }
        return Py::asObject(new MeshPy(mesh));
    }
    Py::Object calculateEigenTransform(const Py::Tuple& args)
    {
        PyObject* input {};

        if (!PyArg_ParseTuple(args.ptr(), "O", &input)) {
            throw Py::Exception();
        }

        if (!PySequence_Check(input)) {
            throw Py::TypeError("Input has to be a sequence of Base.Vector()");
        }

        MeshCore::MeshKernel aMesh;
        MeshCore::MeshPointArray vertices;
        vertices.clear();
        MeshCore::MeshFacetArray faces;
        faces.clear();
        MeshCore::MeshPoint current_node;

        Py::Sequence list(input);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* value = (*it).ptr();
            if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
                Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
                Base::Vector3d* val = pcObject->getVectorPtr();


                current_node.Set(float(val->x), float(val->y), float(val->z));
                vertices.push_back(current_node);
            }
        }

        MeshCore::MeshFacet aFacet;
        aFacet._aulPoints[0] = 0;
        aFacet._aulPoints[1] = 1;
        aFacet._aulPoints[2] = 2;
        faces.push_back(aFacet);
        // Fill the Kernel with the temp mesh structure and delete the current containers
        aMesh.Adopt(vertices, faces);
        MeshCore::MeshEigensystem pca(aMesh);
        pca.Evaluate();
        Base::Matrix4D Trafo = pca.Transform();

        return Py::asObject(new Base::PlacementPy(new Base::Placement(Trafo)));
    }
    Py::Object polynomialFit(const Py::Tuple& args)
    {
        PyObject* input {};

        if (!PyArg_ParseTuple(args.ptr(), "O", &input)) {
            throw Py::Exception();
        }

        if (!PySequence_Check(input)) {
            throw Py::TypeError("Input has to be a sequence of Base.Vector()");
        }

        MeshCore::SurfaceFit polyFit;

        Base::Vector3f point;
        Py::Sequence list(input);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* value = (*it).ptr();
            if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
                Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
                Base::Vector3d* val = pcObject->getVectorPtr();
                point.Set(float(val->x), float(val->y), float(val->z));
                polyFit.AddPoint(point);
            }
        }

        // fit quality
        float fit = polyFit.Fit();
        Py::Dict dict;
        dict.setItem(Py::String("Sigma"), Py::Float(fit));

        // coefficients
        double a {}, b {}, c {}, d {}, e {}, f {};
        polyFit.GetCoefficients(a, b, c, d, e, f);
        Py::Tuple p(6);
        p.setItem(0, Py::Float(a));
        p.setItem(1, Py::Float(b));
        p.setItem(2, Py::Float(c));
        p.setItem(3, Py::Float(d));
        p.setItem(4, Py::Float(e));
        p.setItem(5, Py::Float(f));
        dict.setItem(Py::String("Coefficients"), p);

        // residuals
        std::vector<Base::Vector3f> local = polyFit.GetLocalPoints();
        Py::Tuple r(local.size());
        for (std::vector<Base::Vector3f>::iterator it = local.begin(); it != local.end(); ++it) {
            double z = polyFit.Value(it->x, it->y);
            double d = it->z - z;
            r.setItem(it - local.begin(), Py::Float(d));
        }
        dict.setItem(Py::String("Residuals"), r);

        return dict;  // NOLINT
    }
    Py::Object minimumVolumeOrientedBox(const Py::Tuple& args)
    {
        PyObject* input {};

        if (!PyArg_ParseTuple(args.ptr(), "O", &input)) {
            throw Py::Exception();
        }

        if (!PySequence_Check(input)) {
            throw Py::TypeError("Input has to be a sequence of Base.Vector()");
        }

        Py::Sequence list(input);
        std::vector<Wm4::Vector3d> points;
        points.reserve(list.size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* value = (*it).ptr();
            if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
                Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
                Base::Vector3d* val = pcObject->getVectorPtr();
                Wm4::Vector3d pt;
                pt[0] = val->x;
                pt[1] = val->y;
                pt[2] = val->z;
                points.push_back(pt);
            }
        }

        if (points.size() < 4) {
            throw Py::RuntimeError("Too few points");
        }

        Wm4::Box3d mobox = Wm4::ContMinBox(points.size(), &(points[0]), 0.001, Wm4::Query::QT_REAL);
        Py::Tuple result(7);
        Base::Vector3d v;

        v.x = mobox.Center[0];
        v.y = mobox.Center[1];
        v.z = mobox.Center[2];
        result.setItem(0, Py::Vector(v));

        v.x = mobox.Axis[0][0];
        v.y = mobox.Axis[0][1];
        v.z = mobox.Axis[0][2];
        result.setItem(1, Py::Vector(v));

        v.x = mobox.Axis[1][0];
        v.y = mobox.Axis[1][1];
        v.z = mobox.Axis[1][2];
        result.setItem(2, Py::Vector(v));

        v.x = mobox.Axis[2][0];
        v.y = mobox.Axis[2][1];
        v.z = mobox.Axis[2][2];
        result.setItem(3, Py::Vector(v));

        result.setItem(4, Py::Float(mobox.Extent[0]));
        result.setItem(5, Py::Float(mobox.Extent[1]));
        result.setItem(6, Py::Float(mobox.Extent[2]));

        return result;  // NOLINT
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Mesh
