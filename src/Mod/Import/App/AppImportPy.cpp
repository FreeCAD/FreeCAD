// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#if defined(__MINGW32__)
# define WNT  // avoid conflict with GUID
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/indexed.hpp>
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wextra-semi"
#endif
#include <Interface_Static.hxx>
#include <OSD_Exception.hxx>
#include <Standard_Version.hxx>
#include <TColStd_IndexedDataMapOfStringString.hxx>
#include <TDocStd_Document.hxx>
#include <Transfer_TransientProcess.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_WorkSession.hxx>
#include <Message_ProgressRange.hxx>

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#include <chrono>
#include "dxf/ImpExpDxf.h"
#include "SketchExportHelper.h"
#include <App/Annotation.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <Base/Console.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/Interface.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PartFeaturePy.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/encodeFilename.h>

#include "ImportOCAF2.h"
#include "ReaderGltf.h"
#include "ReaderIges.h"
#include "ReaderStep.h"
#include "WriterGltf.h"
#include "WriterIges.h"
#include "WriterStep.h"

#include <Python.h>  // For direct C-API calls

namespace
{  // anonymous namespace for internal DXF export helpers

typedef struct
{
    PyObject_HEAD Import::ImpExpDxfWrite* writer_inst;
} DxfWriterProxy;

// Forward declarations of the static C functions
static void DxfWriterProxy_dealloc(DxfWriterProxy* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* DxfWriterProxy_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    DxfWriterProxy* self = (DxfWriterProxy*)type->tp_alloc(type, 0);
    if (self != nullptr) {
        self->writer_inst = nullptr;
    }
    return (PyObject*)self;
}

static PyObject* DxfWriterProxy_writeBlock(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    double p[3];
    if (!PyArg_ParseTuple(args, "s(ddd)", &name, &p[0], &p[1], &p[2])) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeBlock(name, p);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_writeEndBlock(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeEndBlock(name);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_writeInsert(DxfWriterProxy* self, PyObject* args)
{
    char* name;
    double p[3];
    double scale, rotation;
    if (!PyArg_ParseTuple(args, "s(ddd)dd", &name, &p[0], &p[1], &p[2], &scale, &rotation)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->writeInsert(name, p, scale, rotation);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_exportShape(DxfWriterProxy* self, PyObject* args)
{
    PyObject* shapeObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &shapeObj)) {
        return nullptr;
    }
    if (self->writer_inst) {
        Part::TopoShape* ts = static_cast<Part::TopoShapePy*>(shapeObj)->getTopoShapePtr();
        self->writer_inst->exportShape(ts->getShape());
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_setLayerName(DxfWriterProxy* self, PyObject* args)
{
    char* layer_name;
    if (!PyArg_ParseTuple(args, "s", &layer_name)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->setLayerName(layer_name);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_setColor(DxfWriterProxy* self, PyObject* args)
{
    int aci_color;
    if (!PyArg_ParseTuple(args, "i", &aci_color)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst->setColor(aci_color);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_addText(DxfWriterProxy* self, PyObject* args)
{
    char* text_str;
    double p1[3], p2[3], height, rotation;
    int justification;

    if (!PyArg_ParseTuple(args,
                          "s(ddd)(ddd)did",
                          &text_str,
                          &p1[0],
                          &p1[1],
                          &p1[2],
                          &p2[0],
                          &p2[1],
                          &p2[2],
                          &height,
                          &justification,
                          &rotation)) {
        return nullptr;  // PyArg_ParseTuple will set an exception
    }
    if (self->writer_inst) {
        // Call the C++ method named writeText
        self->writer_inst->writeText(text_str, p1, p2, height, justification);
    }
    Py_RETURN_NONE;
}

static PyObject* DxfWriterProxy_writeLinearDim(DxfWriterProxy* self, PyObject* args)
{
    const char* dim_text;
    double text_mid[3], line_def[3], p1[3], p2[3];
    int dim_type;
    double font_size;

    if (!PyArg_ParseTuple(args,
                          "(ddd)(ddd)(ddd)(ddd)sid",
                          &text_mid[0],
                          &text_mid[1],
                          &text_mid[2],
                          &line_def[0],
                          &line_def[1],
                          &line_def[2],
                          &p1[0],
                          &p1[1],
                          &p1[2],
                          &p2[0],
                          &p2[1],
                          &p2[2],
                          &dim_text,
                          &dim_type,
                          &font_size)) {
        return nullptr;
    }
    if (self->writer_inst) {
        self->writer_inst
            ->writeLinearDim(text_mid, line_def, p1, p2, dim_text, dim_type, font_size);
    }
    Py_RETURN_NONE;
}

// Method table
static PyMethodDef DxfWriterProxy_methods[] = {
    {"writeBlock",
     (PyCFunction)DxfWriterProxy_writeBlock,
     METH_VARARGS,
     "writeBlock(name, base_point_tuple)"},
    {"writeEndBlock",
     (PyCFunction)DxfWriterProxy_writeEndBlock,
     METH_VARARGS,
     "writeEndBlock(name)"},
    {"writeInsert",
     (PyCFunction)DxfWriterProxy_writeInsert,
     METH_VARARGS,
     "writeInsert(name, insertion_point_tuple, scale, rotation)"},
    {"exportShape",
     (PyCFunction)DxfWriterProxy_exportShape,
     METH_VARARGS,
     "exportShape(shape_object)"},
    {"setLayerName", (PyCFunction)DxfWriterProxy_setLayerName, METH_VARARGS, "setLayerName(name)"},
    {"setColor", (PyCFunction)DxfWriterProxy_setColor, METH_VARARGS, "setColor(aci_index)"},
    {"addText", (PyCFunction)DxfWriterProxy_addText, METH_VARARGS, "Writes a TEXT entity."},
    {"writeLinearDim",
     (PyCFunction)DxfWriterProxy_writeLinearDim,
     METH_VARARGS,
     "Writes a DIMENSION entity."},
    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

// Type definition
static PyTypeObject DxfWriterProxy_Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "Import.DxfWriterProxy", /* tp_name */
    sizeof(DxfWriterProxy),                                    /* tp_basicsize */
    0,                                                         /* tp_itemsize */
    (destructor)DxfWriterProxy_dealloc,                        /* tp_dealloc */
    0,                                                         /* tp_vectorcall_offset */
    nullptr,                                                   /* tp_getattr */
    nullptr,                                                   /* tp_setattr */
    nullptr,                                                   /* tp_as_async */
    nullptr,                                                   /* tp_repr */
    nullptr,                                                   /* tp_as_number */
    nullptr,                                                   /* tp_as_sequence */
    nullptr,                                                   /* tp_as_mapping */
    nullptr,                                                   /* tp_hash */
    nullptr,                                                   /* tp_call */
    nullptr,                                                   /* tp_str */
    PyObject_GenericGetAttr,                                   /* tp_getattro */
    nullptr,                                                   /* tp_setattro */
    nullptr,                                                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                        /* tp_flags */
    "A proxy for the C++ DXF writer.",                         /* tp_doc */
    nullptr,                                                   /* tp_traverse */
    nullptr,                                                   /* tp_clear */
    nullptr,                                                   /* tp_richcompare */
    0,                                                         /* tp_weaklistoffset */
    nullptr,                                                   /* tp_iter */
    nullptr,                                                   /* tp_iternext */
    DxfWriterProxy_methods,                                    /* tp_methods */
    nullptr,                                                   /* tp_members */
    nullptr,                                                   /* tp_getset */
    nullptr,                                                   /* tp_base */
    nullptr,                                                   /* tp_dict */
    nullptr,                                                   /* tp_descr_get */
    nullptr,                                                   /* tp_descr_set */
    0,                                                         /* tp_dictoffset */
    (initproc) nullptr,                                        /* tp_init */
    PyType_GenericAlloc,                                       /* tp_alloc */
    DxfWriterProxy_new,                                        /* tp_new */
    nullptr,                                                   /* tp_free */
    nullptr,                                                   /* tp_is_gc */
    nullptr,                                                   /* tp_bases */
    nullptr,                                                   /* tp_mro */
    nullptr,                                                   /* tp_cache */
    nullptr,                                                   /* tp_subclasses */
    nullptr,                                                   /* tp_weaklist */
    nullptr,                                                   /* tp_del */
    0,                                                         /* tp_version_tag */
    nullptr,                                                   /* tp_finalize */
    nullptr,                                                   /* tp_vectorcall */
};

}  // end anonymous namespace


namespace Import
{


void executeDxfExport(PyObject* objectList, ImpExpDxfWrite& writer, PyObject* helperModule)
{
    if (!helperModule) {
        throw Py::RuntimeError("A valid helper module was not provided to the DXF exporter.");
    }

    // Get the single master dispatcher function from Python
    PyObject* export_object_func = PyObject_GetAttrString(helperModule, "_export_object");
    if (!export_object_func || !PyCallable_Check(export_object_func)) {
        Py_XDECREF(helperModule);
        Py_XDECREF(export_object_func);
        throw Py::RuntimeError("Cannot find the _export_object helper function in importDXF.py");
    }

    Py::Sequence list(objectList);

    // Create the C++ writer proxy once
    PyObject* writerProxyObj = DxfWriterProxy_Type.tp_new(&DxfWriterProxy_Type, nullptr, nullptr);
    ((DxfWriterProxy*)writerProxyObj)->writer_inst = &writer;

    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        PyObject* item = (*it).ptr();

        // Call the master dispatcher for every single object
        PyObject* result =
            PyObject_CallFunctionObjArgs(export_object_func, item, writerProxyObj, NULL);
        Py_XDECREF(result);  // We don't care about the return value

        if (PyErr_Occurred()) {
            // An error happened in Python, print it and clear it to continue
            PyErr_Print();
            PyErr_Clear();
        }
    }

    // Clean up
    Py_DECREF(writerProxyObj);
    Py_DECREF(export_object_func);
}

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Import")
    {
        add_keyword_method("open",
                           &Module::importer,
                           "open(string) -- Open the file and create a new document.");
        add_keyword_method("insert",
                           &Module::importer,
                           "insert(string,string) -- Insert the file into the given document.");
        add_keyword_method("export",
                           &Module::exporter,
                           "export(list,string) -- Export a list of objects into a single file.");
        add_varargs_method("readDXF",
                           &Module::readDXF,
                           "readDXF(filename,[document,ignore_errors,option_source]): Imports a "
                           "DXF file into the given document. ignore_errors is True by default.");
        add_varargs_method("writeDXFShape",
                           &Module::writeDXFShape,
                           "writeDXFShape([shape],filename [version,usePolyline,optionSource]): "
                           "Exports Shape(s) to a DXF file.");
        add_keyword_method("exportDxf",
                           &Module::exportDxf,
                           "exportDxf(obj=list, name=string, version=14, lwPoly=False): Exports "
                           "DocumentObject(s) to a DXF file.");

        // Call initialize() first, as it creates the module object.
        initialize("This module is the Import module.");

        // 1. Finalize the custom type
        if (PyType_Ready(&DxfWriterProxy_Type) < 0) {
            throw Py::Exception();  // PyType_Ready will set a Python exception
        }

        // 2. Add the finalized type to the module using the interpreter singleton.
        //    this->module() returns the Py::Object wrapper for the module.
        //    this->module().ptr() gets the raw PyObject* pointer.
        Base::Interpreter().addType(&DxfWriterProxy_Type, this->module().ptr(), "DxfWriterProxy");
    }

    ~Module() override = default;

private:
    Py::Object importer(const Py::Tuple& args, const Py::Dict& kwds)
    {
        char* Name = nullptr;
        char* DocName = nullptr;
        PyObject* importHidden = Py_None;
        PyObject* merge = Py_None;
        PyObject* useLinkGroup = Py_None;
        int mode = -1;
        static const std::array<const char*, 7>
            kwd_list {"name", "docName", "importHidden", "merge", "useLinkGroup", "mode", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(
                args.ptr(),
                kwds.ptr(),
                "et|sO!O!O!i",
                kwd_list,
                "utf-8",
                &Name,
                &DocName,
                &PyBool_Type,
                &importHidden,
                &PyBool_Type,
                &merge,
                &PyBool_Type,
                &useLinkGroup,
                &mode
            )) {
            throw Py::Exception();
        }

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::FileInfo file(Utf8Name.c_str());

            App::Document* pcDoc = nullptr;
            if (DocName) {
                pcDoc = App::GetApplication().getDocument(DocName);
            }
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument();
            }

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            if (file.hasExtension({"stp", "step"})) {
                try {
                    Import::ReaderStep reader(file);
                    reader.read(hDoc);
                }
                catch (OSD_Exception& e) {
                    Base::Console().error("%s\n", e.GetMessageString());
                    Base::Console().message("Try to load STEP file without colors...\n");

                    Part::ImportStepParts(pcDoc, Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else if (file.hasExtension({"igs", "iges"})) {
                try {
                    Import::ReaderIges reader(file);
                    reader.read(hDoc);
                }
                catch (OSD_Exception& e) {
                    Base::Console().error("%s\n", e.GetMessageString());
                    Base::Console().message("Try to load IGES file without colors...\n");

                    Part::ImportIgesParts(pcDoc, Utf8Name.c_str());
                    pcDoc->recompute();
                }
            }
            else if (file.hasExtension({"glb", "gltf"})) {
                Import::ReaderGltf reader(file);
                reader.read(hDoc);
            }
            else {
                throw Py::Exception(PyExc_IOError, "no supported file format");
            }

            ImportOCAFExt ocaf(hDoc, pcDoc, file.fileNamePure());
            ocaf.setImportOptions(ImportOCAFExt::customImportOptions());
            if (merge != Py_None) {
                ocaf.setMerge(Base::asBoolean(merge));
            }
            if (importHidden != Py_None) {
                ocaf.setImportHiddenObject(Base::asBoolean(importHidden));
            }
            if (useLinkGroup != Py_None) {
                ocaf.setUseLinkGroup(Base::asBoolean(useLinkGroup));
            }
            if (mode >= 0) {
                ocaf.setMode(mode);
            }
            ocaf.loadShapes();

            hApp->Close(hDoc);

            if (!ocaf.partColors.empty()) {
                Py::List list;
                for (auto& it : ocaf.partColors) {
                    Py::Tuple tuple(2);
                    tuple.setItem(0, Py::asObject(it.first->getPyObject()));

                    App::PropertyColorList colors;
                    colors.setValues(it.second);
                    tuple.setItem(1, Py::asObject(colors.getPyObject()));

                    list.append(tuple);
                }

                return list;  // NOLINT
            }
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::PyExc_FC_GeneralError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }
    Py::Object exporter(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject* object = nullptr;
        char* Name = nullptr;
        PyObject* pyexportHidden = Py_None;
        PyObject* pylegacy = Py_None;
        PyObject* pykeepPlacement = Py_None;
        static const std::array<const char*, 6>
            kwd_list {"obj", "name", "exportHidden", "legacy", "keepPlacement", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(
                args.ptr(),
                kwds.ptr(),
                "Oet|O!O!O!",
                kwd_list,
                &object,
                "utf-8",
                &Name,
                &PyBool_Type,
                &pyexportHidden,
                &PyBool_Type,
                &pylegacy,
                &PyBool_Type,
                &pykeepPlacement
            )) {
            throw Py::Exception();
        }

        std::string Utf8Name = std::string(Name);
        PyMem_Free(Name);

        // clang-format off
        // determine export options
        Part::OCAF::ImportExportSettings settings;

        bool legacyExport = (pylegacy         == Py_None ? settings.getExportLegacy()
                                                         : Base::asBoolean(pylegacy));
        bool exportHidden = (pyexportHidden   == Py_None ? settings.getExportHiddenObject()
                                                         : Base::asBoolean(pyexportHidden));
        bool keepPlacement = (pykeepPlacement == Py_None ? settings.getExportKeepPlacement()
                                                         : Base::asBoolean(pykeepPlacement));
        // clang-format on

        try {
            Py::Sequence list(object);
            std::vector<App::DocumentObject*> objs;
            std::map<Part::Feature*, std::vector<Base::Color>> partColor;
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                PyObject* item = (*it).ptr();
                if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                    auto pydoc = static_cast<App::DocumentObjectPy*>(item);
                    objs.push_back(pydoc->getDocumentObjectPtr());
                }
                else if (PyTuple_Check(item) && PyTuple_Size(item) == 2) {
                    Py::Tuple tuple(*it);
                    Py::Object item0 = tuple.getItem(0);
                    Py::Object item1 = tuple.getItem(1);
                    if (PyObject_TypeCheck(item0.ptr(), &(App::DocumentObjectPy::Type))) {
                        auto pydoc = static_cast<App::DocumentObjectPy*>(item0.ptr());
                        App::DocumentObject* obj = pydoc->getDocumentObjectPtr();
                        objs.push_back(obj);
                        if (Part::Feature* part = dynamic_cast<Part::Feature*>(obj)) {
                            App::PropertyColorList colors;
                            colors.setPyObject(item1.ptr());
                            partColor[part] = colors.getValues();
                        }
                    }
                }
            }

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) hDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc);

            auto getShapeColors = [partColor](App::DocumentObject* obj, const char* subname) {
                std::map<std::string, Base::Color> cols;
                auto it = partColor.find(dynamic_cast<Part::Feature*>(obj));
                if (it != partColor.end() && boost::starts_with(subname, "Face")) {
                    const auto& colors = it->second;
                    std::string face("Face");
                    for (const auto& element : colors | boost::adaptors::indexed(1)) {
                        cols[face + std::to_string(element.index())] = element.value();
                    }
                }
                return cols;
            };

            Import::ExportOCAF2 ocaf(hDoc, getShapeColors);
            if (!legacyExport || !ocaf.canFallback(objs)) {
                ocaf.setExportOptions(ExportOCAF2::customExportOptions());
                ocaf.setExportHiddenObject(exportHidden);
                ocaf.setKeepPlacement(keepPlacement);

                ocaf.exportObjects(objs);
            }
            else {
                bool keepExplicitPlacement = true;
                ExportOCAFCmd ocaf(hDoc, keepExplicitPlacement);
                ocaf.setPartColorsMap(partColor);
                ocaf.exportObjects(objs);
            }

            Base::FileInfo file(Utf8Name.c_str());
            if (file.hasExtension({"stp", "step"})) {
                Import::WriterStep writer(file);
                writer.write(hDoc);
            }
            else if (file.hasExtension({"igs", "iges"})) {
                Import::WriterIges writer(file);
                writer.write(hDoc);
            }
            else if (file.hasExtension({"glb", "gltf"})) {
                Import::WriterGltf writer(file);
                writer.write(hDoc);
            }

            hApp->Close(hDoc);
        }
        catch (Standard_Failure& e) {
            throw Py::Exception(Base::PyExc_FC_GeneralError, e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }

    // This readDXF method is an almost exact duplicate of the one in ImportGui::Module.
    // The only difference is the CDxfRead class derivation that is created.
    // It would seem desirable to have most of this code in just one place, passing it
    // e.g. a pointer to a function that does the 4 lines during the lifetime of the
    // CDxfRead object, but right now Import::Module and ImportGui::Module cannot see
    // each other's functions so this shared code would need some place to live where
    // both places could include a declaration.
    Py::Object readDXF(const Py::Tuple& args)
    {
        char* Name = nullptr;
        const char* DocName = nullptr;
        const char* optionSource = nullptr;
        std::string defaultOptions = "User parameter:BaseApp/Preferences/Mod/Draft";
        bool IgnoreErrors = true;
        if (!PyArg_ParseTuple(args.ptr(), "et|sbs", "utf-8", &Name, &DocName, &IgnoreErrors, &optionSource)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());
        if (!file.exists()) {
            throw Py::RuntimeError("File doesn't exist");
        }

        if (optionSource) {
            defaultOptions = optionSource;
        }

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

        try {
            // read the DXF file
            ImpExpDxfRead dxf_file(EncodedName, pcDoc);
            dxf_file.setOptionSource(defaultOptions);
            dxf_file.setOptions();

            auto startTime = std::chrono::high_resolution_clock::now();
            dxf_file.DoRead(IgnoreErrors);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = endTime - startTime;
            dxf_file.setImportTime(elapsed.count());

            pcDoc->recompute();
            return dxf_file.getStatsAsPyObject();
        }
        catch (const Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }


    Py::Object writeDXFShape(const Py::Tuple& args)
    {
        Base::Console().message("Imp:writeDXFShape()\n");
        PyObject* shapeObj = nullptr;
        char* fname = nullptr;
        std::string filePath;
        std::string layerName;
        const char* optionSource = nullptr;
        std::string defaultOptions = "User parameter:BaseApp/Preferences/Mod/Draft";
        int versionParm = -1;
        bool versionOverride = false;
        bool polyOverride = false;
        PyObject* usePolyline = Py_False;

        // handle list of shapes
        if (PyArg_ParseTuple(
                args.ptr(),
                "O!et|iOs",
                &(PyList_Type),
                &shapeObj,
                "utf-8",
                &fname,
                &versionParm,
                &usePolyline,
                &optionSource
            )) {
            filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) || (versionParm == 14)) {
                versionOverride = true;
            }
            if (usePolyline == Py_True) {
                polyOverride = true;
            }
            if (optionSource) {
                defaultOptions = optionSource;
            }

            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(defaultOptions);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                Py::Sequence list(shapeObj);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                        Part::TopoShape* ts
                            = static_cast<Part::TopoShapePy*>((*it).ptr())->getTopoShapePtr();
                        TopoDS_Shape shape = ts->getShape();
                        writer.exportShape(shape);
                    }
                }
                writer.endRun();
                return Py::None();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(
                args.ptr(),
                "O!et|iOs",
                &(Part::TopoShapePy::Type),
                &shapeObj,
                "utf-8",
                &fname,
                &versionParm,
                &usePolyline,
                &optionSource
            )) {
            filePath = std::string(fname);
            layerName = "none";
            PyMem_Free(fname);

            if ((versionParm == 12) || (versionParm == 14)) {
                versionOverride = true;
            }
            if (usePolyline == Py_True) {
                polyOverride = true;
            }
            if (optionSource) {
                defaultOptions = optionSource;
            }

            try {
                ImpExpDxfWrite writer(filePath);
                writer.setOptionSource(defaultOptions);
                writer.setOptions();
                if (versionOverride) {
                    writer.setVersion(versionParm);
                }
                writer.setPolyOverride(polyOverride);
                writer.setLayerName(layerName);
                writer.init();
                Part::TopoShape* obj = static_cast<Part::TopoShapePy*>(shapeObj)->getTopoShapePtr();
                TopoDS_Shape shape = obj->getShape();
                writer.exportShape(shape);
                writer.endRun();
                return Py::None();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        }

        throw Py::TypeError("expected ([Shape],path");
    }

    Py::Object exportDxf(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject* objectList = nullptr;
        char* filename = nullptr;
        int version = 14;
        PyObject* use_lwpolyline = Py_False;
        PyObject* helperModule = nullptr;

        static const std::array<const char*, 6> kwd_list {"obj",
                                                          "name",
                                                          "version",
                                                          "lwPoly",
                                                          "helpers",
                                                          nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(),
                                                 kwds.ptr(),
                                                 "Oet|iO!O",
                                                 kwd_list,
                                                 &objectList,
                                                 "utf-8",
                                                 &filename,
                                                 &version,
                                                 &PyBool_Type,
                                                 &use_lwpolyline,
                                                 &helperModule)) {  // No type check for the module
            throw Py::Exception();
        }

        std::string utf8_filename = std::string(filename);
        PyMem_Free(filename);

        if (!PyList_Check(objectList)) {
            PyErr_SetString(PyExc_TypeError, "First argument ('obj') must be a list of objects.");
            throw Py::Exception();
        }

        try {  // NOLINT(bugprone-throw-keyword-missing)
            ImpExpDxfWrite writer(utf8_filename);
            writer.setOptions();
            writer.setVersion(version);
            writer.setPolyOverride(use_lwpolyline == Py_True);

            writer.init();
            Import::executeDxfExport(objectList, writer, helperModule);

            writer.endRun();
        }
        catch (const Base::Exception& e) {
            e.setPyException();
            throw Py::Exception();
        }

        return Py::None();
    }
};


PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Import
