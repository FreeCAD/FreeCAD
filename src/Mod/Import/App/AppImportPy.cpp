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
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>
#include <Mod/Part/App/Interface.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PartFeaturePy.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/TechDraw/App/DrawPage.h>

#include "ImportOCAF2.h"
#include "ReaderGltf.h"
#include "ReaderIges.h"
#include "ReaderStep.h"
#include "WriterGltf.h"
#include "WriterIges.h"
#include "WriterStep.h"

#include <Python.h>  // For direct C-API calls

namespace Import
{

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Import")
    {
        add_keyword_method(
            "open",
            &Module::importer,
            "open(string) -- Open the file and create a new document."
        );
        add_keyword_method(
            "insert",
            &Module::importer,
            "insert(string,string) -- Insert the file into the given document."
        );
        add_keyword_method(
            "export",
            &Module::exporter,
            "export(list,string) -- Export a list of objects into a single file."
        );
        add_varargs_method(
            "readDXF",
            &Module::readDXF,
            "readDXF(filename,[document,ignore_errors,option_source]): Imports a "
            "DXF file into the given document. ignore_errors is True by default."
        );
        add_varargs_method(
            "writeDXFShape",
            &Module::writeDXFShape,
            "writeDXFShape([shape],filename [version,usePolyline,optionSource]): "
            "Exports Shape(s) to a DXF file."
        );
        add_varargs_method(
            "writeDXFObject",
            &Module::writeDXFObject,
            "writeDXFObject([objects],filename [,version,usePolyline,optionSource]): Exports "
            "DocumentObject(s) to a DXF file."
        );
        initialize("This module is the Import module.");  // register with Python
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

    Py::Object writeDXFObject(const Py::Tuple& args)
    {
        PyObject* docObj = nullptr;
        char* fname = nullptr;
        std::string filePath;
        const char* optionSource = nullptr;
        std::string defaultOptions = "User parameter:BaseApp/Preferences/Mod/Draft";
        int versionParm = -1;
        bool versionOverride = false;
        bool polyOverride = false;
        PyObject* usePolyline = Py_False;

        if (PyArg_ParseTuple(
                args.ptr(),
                "O!et|iOs",
                &(PyList_Type),
                &docObj,
                "utf-8",
                &fname,
                &versionParm,
                &usePolyline,
                &optionSource
            )) {
            filePath = std::string(fname);
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
                writer.init();

                PyObject* helperModule = PyImport_ImportModule("Draft.importDXF");
                if (!helperModule) {
                    throw Py::ImportError("Could not import Draft.importDXF module.");
                }

                Py::Sequence list(docObj);

                // Special case: If the list contains exactly one TechDraw Page, use a dedicated
                // exporter.
                bool pageExported = false;
                if (list.size() == 1) {
                    PyObject* item = list.getItem(0).ptr();
                    App::DocumentObject* obj =
                        static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                    if (obj->isDerivedFrom(TechDraw::DrawPage::getClassTypeId())) {
                        PyObject* export_page_func =
                            PyObject_GetAttrString(helperModule, "_export_techdraw_page");
                        if (export_page_func && PyCallable_Check(export_page_func)) {
                            // The implementation of the proxy and the Python helper is the next
                            // step. This structure prepares the C++ side for that implementation.
                            Base::Console().message("TechDraw Page detected, handing off to Python "
                                                    "helper (implementation pending).\n");
                            // Example of future call:
                            // PyObject* writerProxy = ... create proxy ...
                            // PyObject_CallFunctionObjArgs(export_page_func, item, writerProxy,
                            // NULL); Py_DECREF(writerProxy);
                        }
                        Py_XDECREF(export_page_func);
                        if (PyErr_Occurred()) {
                            PyErr_Clear();
                        }
                        pageExported = true;
                    }
                }

                if (!pageExported) {
                    // If it wasn't a page, or if there were multiple objects, process the list
                    // normally.
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        PyObject* item = (*it).ptr();
                        App::DocumentObject* obj =
                            static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();

                        // --- Get Layer and Color (common for all types) ---
                        std::string layerName = "0";
                        int aciColor = 256;

                        PyObject* get_layer_func =
                            PyObject_GetAttrString(helperModule, "_get_layer_name");
                        if (get_layer_func && PyCallable_Check(get_layer_func)) {
                            PyObject* pyLayerName =
                                PyObject_CallFunctionObjArgs(get_layer_func, item, NULL);
                            if (pyLayerName && PyUnicode_Check(pyLayerName)) {
                                layerName = PyUnicode_AsUTF8(pyLayerName);
                            }
                            Py_XDECREF(pyLayerName);
                        }
                        Py_XDECREF(get_layer_func);

                        PyObject* get_aci_func =
                            PyObject_GetAttrString(helperModule, "_get_aci_color");
                        if (get_aci_func && PyCallable_Check(get_aci_func)) {
                            PyObject* pyAciColor =
                                PyObject_CallFunctionObjArgs(get_aci_func, item, NULL);
                            if (pyAciColor && PyLong_Check(pyAciColor)) {
                                aciColor = PyLong_AsLong(pyAciColor);
                            }
                            Py_XDECREF(pyAciColor);
                        }
                        Py_XDECREF(get_aci_func);

                        if (PyErr_Occurred()) {
                            PyErr_Clear();
                        }

                        writer.setLayerName(layerName);
                        writer.setColor(aciColor);

                        // --- Type Dispatcher ---
                        if (obj->isDerivedFrom(App::Annotation::getClassTypeId())) {
                            PyObject* get_text_data_func =
                                PyObject_GetAttrString(helperModule, "_get_text_data");
                            if (get_text_data_func && PyCallable_Check(get_text_data_func)) {
                                PyObject* text_data_list =
                                    PyObject_CallFunctionObjArgs(get_text_data_func, item, NULL);
                                if (text_data_list && PyList_Check(text_data_list)) {
                                    Py_ssize_t size = PyList_Size(text_data_list);
                                    for (Py_ssize_t i = 0; i < size; ++i) {
                                        PyObject* text_tuple = PyList_GetItem(text_data_list, i);
                                        char* text_str;
                                        double p1[3], p2[3], height, rotation;
                                        int justification;
                                        if (PyArg_ParseTuple(text_tuple,
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
                                            writer.writeText(text_str,
                                                             p1,
                                                             p2,
                                                             height,
                                                             justification);
                                        }
                                    }
                                }
                                Py_XDECREF(text_data_list);
                            }
                            Py_XDECREF(get_text_data_func);
                        }
                        else if (obj->getPropertyByName("Dimline") != nullptr) {
                            PyObject* get_dim_data_func =
                                PyObject_GetAttrString(helperModule, "_get_dimension_data");
                            if (get_dim_data_func && PyCallable_Check(get_dim_data_func)) {
                                PyObject* dim_tuple =
                                    PyObject_CallFunctionObjArgs(get_dim_data_func, item, NULL);
                                if (dim_tuple && PyTuple_Check(dim_tuple)) {
                                    const char* dim_text;
                                    double text_mid[3], line_def[3], p1[3], p2[3];
                                    int dim_type;
                                    if (PyArg_ParseTuple(dim_tuple,
                                                         "(ddd)(ddd)(ddd)(ddd)si",
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
                                                         &dim_type)) {
                                        writer.writeLinearDim(text_mid,
                                                              line_def,
                                                              p1,
                                                              p2,
                                                              dim_text,
                                                              dim_type);
                                    }
                                }
                                Py_XDECREF(dim_tuple);
                            }
                            Py_XDECREF(get_dim_data_func);
                        }
                        else if (auto* part = dynamic_cast<Part::Feature*>(obj)) {
                            if (SketchExportHelper::isSketch(obj)) {
                                writer.exportShape(SketchExportHelper::getFlatSketchXY(obj));
                            }
                            else {
                                writer.exportShape(part->Shape.getValue());
                            }
                        }

                        if (PyErr_Occurred()) {
                            PyErr_Clear();
                        }
                    }
                }

                Py_DECREF(helperModule);
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
                &(App::DocumentObjectPy::Type),
                &docObj,
                "utf-8",
                &fname,
                &versionParm,
                &usePolyline,
                &optionSource
            )) {
            filePath = std::string(fname);
            PyMem_Free(fname);
            App::DocumentObject* obj
                = static_cast<App::DocumentObjectPy*>(docObj)->getDocumentObjectPtr();
            Base::Console().message("Imp:writeDXFObject - docObj: %s\n", obj->getNameInDocument());

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
                writer.init();

                PyObject* item = docObj;
                // --- Call Python helpers using the C-API ---
                std::string layerName = "0";  // Default layer
                int aciColor = 256;           // Default color (BYLAYER)

                PyObject* helperModule = PyImport_ImportModule("Draft.importDXF");
                if (!helperModule) {
                    throw Py::ImportError("Could not import Draft.importDXF module.");
                }

                PyObject* get_layer_func = PyObject_GetAttrString(helperModule, "_get_layer_name");
                if (get_layer_func && PyCallable_Check(get_layer_func)) {
                    PyObject* pyLayerName =
                        PyObject_CallFunctionObjArgs(get_layer_func, item, NULL);
                    if (pyLayerName && PyUnicode_Check(pyLayerName)) {
                        layerName = PyUnicode_AsUTF8(pyLayerName);
                    }
                    Py_XDECREF(pyLayerName);
                }
                Py_XDECREF(get_layer_func);

                PyObject* get_aci_func = PyObject_GetAttrString(helperModule, "_get_aci_color");
                if (get_aci_func && PyCallable_Check(get_aci_func)) {
                    PyObject* pyAciColor = PyObject_CallFunctionObjArgs(get_aci_func, item, NULL);
                    if (pyAciColor && PyLong_Check(pyAciColor)) {
                        aciColor = PyLong_AsLong(pyAciColor);
                    }
                    Py_XDECREF(pyAciColor);
                }
                Py_XDECREF(get_aci_func);

                if (PyErr_Occurred()) {
                    PyErr_Clear();
                    Base::Console().warning(
                        "A Python error occurred while getting layer/color for object %s\n",
                        obj->getNameInDocument());
                }
                Py_DECREF(helperModule);

                writer.setLayerName(layerName);
                writer.setColor(aciColor);

                TopoDS_Shape shapeToExport;
                if (SketchExportHelper::isSketch(obj)) {
                    shapeToExport = SketchExportHelper::getFlatSketchXY(obj);
                }
                else {
                    Part::Feature* part = static_cast<Part::Feature*>(obj);
                    shapeToExport = part->Shape.getValue();
                }
                writer.exportShape(shapeToExport);
                writer.endRun();
                return Py::None();
            }
            catch (const Base::Exception& e) {
                throw Py::RuntimeError(e.what());
            }
        }

        throw Py::TypeError("expected ([DocObject],path) or (DocObject,path)");
    }
};


PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Import
