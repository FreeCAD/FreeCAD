/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <memory>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <App/Property.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>

#include "Points.h"
#include "PointsAlgos.h"
#include "PointsPy.h"
#include "Properties.h"
#include "Structured.h"


namespace Points
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Points")
    {
        add_varargs_method("open", &Module::open);
        add_varargs_method("insert", &Module::importer);
        add_varargs_method("export", &Module::exporter);
        add_varargs_method("show",
                           &Module::show,
                           "show(points,[string]) -- Add the points to the active document or "
                           "create one if no document exists.");
        initialize("This module is the Points module.");  // register with Python
    }

private:
    std::tuple<bool, bool, double> readE57Settings() const
    {
        Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                                 .GetUserParameter()
                                                 .GetGroup("BaseApp")
                                                 ->GetGroup("Preferences")
                                                 ->GetGroup("Mod/Points/E57");
        bool useColor = hGrp->GetBool("UseColor", true);
        bool checkState = hGrp->GetBool("CheckInvalidState", true);
        double minDistance = hGrp->GetFloat("MinDistance", -1.);

        return std::make_tuple(useColor, checkState, minDistance);
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::Console().Log("Open in Points with %s", EncodedName.c_str());
            Base::FileInfo file(EncodedName.c_str());

            // extract ending
            if (file.extension().empty()) {
                throw Py::RuntimeError("No file extension");
            }

            std::unique_ptr<Reader> reader;
            if (file.hasExtension("asc")) {
                reader = std::make_unique<AscReader>();
            }
            else if (file.hasExtension("e57")) {
                auto setting = readE57Settings();
                reader = std::make_unique<E57Reader>(std::get<0>(setting),
                                                     std::get<1>(setting),
                                                     std::get<2>(setting));
            }
            else if (file.hasExtension("ply")) {
                reader = std::make_unique<PlyReader>();
            }
            else if (file.hasExtension("pcd")) {
                reader = std::make_unique<PcdReader>();
            }
            else {
                throw Py::RuntimeError("Unsupported file extension");
            }

            reader->read(EncodedName);

            App::Document* pcDoc = App::GetApplication().newDocument();

            Points::Feature* pcFeature = nullptr;
            if (reader->hasProperties()) {
                // Scattered or structured points?
                if (reader->isStructured()) {
                    pcFeature = new Points::StructuredCustom();

                    App::PropertyInteger* width =
                        static_cast<App::PropertyInteger*>(pcFeature->getPropertyByName("Width"));
                    if (width) {
                        width->setValue(reader->getWidth());
                    }
                    App::PropertyInteger* height =
                        static_cast<App::PropertyInteger*>(pcFeature->getPropertyByName("Height"));
                    if (height) {
                        height->setValue(reader->getHeight());
                    }
                }
                else {
                    pcFeature = new Points::FeatureCustom();
                }

                pcFeature->Points.setValue(reader->getPoints());
                // add gray values
                if (reader->hasIntensities()) {
                    Points::PropertyGreyValueList* prop =
                        static_cast<Points::PropertyGreyValueList*>(
                            pcFeature->addDynamicProperty("Points::PropertyGreyValueList",
                                                          "Intensity"));
                    if (prop) {
                        prop->setValues(reader->getIntensities());
                    }
                }
                // add colors
                if (reader->hasColors()) {
                    App::PropertyColorList* prop = static_cast<App::PropertyColorList*>(
                        pcFeature->addDynamicProperty("App::PropertyColorList", "Color"));
                    if (prop) {
                        prop->setValues(reader->getColors());
                    }
                }
                // add normals
                if (reader->hasNormals()) {
                    Points::PropertyNormalList* prop = static_cast<Points::PropertyNormalList*>(
                        pcFeature->addDynamicProperty("Points::PropertyNormalList", "Normal"));
                    if (prop) {
                        prop->setValues(reader->getNormals());
                    }
                }

                // delayed adding of the points feature
                pcDoc->addObject(pcFeature, file.fileNamePure().c_str());
                pcDoc->recomputeFeature(pcFeature);
                pcFeature->purgeTouched();
            }
            else {
                if (reader->isStructured()) {
                    Structured* structured = new Points::Structured();
                    structured->Width.setValue(reader->getWidth());
                    structured->Height.setValue(reader->getHeight());
                    pcFeature = structured;
                }
                else {
                    pcFeature = new Points::Feature();
                }

                // delayed adding of the points feature
                pcFeature->Points.setValue(reader->getPoints());
                pcDoc->addObject(pcFeature, file.fileNamePure().c_str());
                pcDoc->recomputeFeature(pcFeature);
                pcFeature->purgeTouched();
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object importer(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "ets", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::Console().Log("Import in Points with %s", EncodedName.c_str());
            Base::FileInfo file(EncodedName.c_str());

            // extract ending
            if (file.extension().empty()) {
                throw Py::RuntimeError("No file extension");
            }

            std::unique_ptr<Reader> reader;
            if (file.hasExtension("asc")) {
                reader = std::make_unique<AscReader>();
            }
            else if (file.hasExtension("e57")) {
                auto setting = readE57Settings();
                reader = std::make_unique<E57Reader>(std::get<0>(setting),
                                                     std::get<1>(setting),
                                                     std::get<2>(setting));
            }
            else if (file.hasExtension("ply")) {
                reader = std::make_unique<PlyReader>();
            }
            else if (file.hasExtension("pcd")) {
                reader = std::make_unique<PcdReader>();
            }
            else {
                throw Py::RuntimeError("Unsupported file extension");
            }

            reader->read(EncodedName);

            App::Document* pcDoc = App::GetApplication().getDocument(DocName);
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument(DocName);
            }

            Points::Feature* pcFeature = nullptr;
            if (reader->hasProperties()) {
                // Scattered or structured points?
                if (reader->isStructured()) {
                    pcFeature = new Points::StructuredCustom();

                    App::PropertyInteger* width =
                        static_cast<App::PropertyInteger*>(pcFeature->getPropertyByName("Width"));
                    if (width) {
                        width->setValue(reader->getWidth());
                    }
                    App::PropertyInteger* height =
                        static_cast<App::PropertyInteger*>(pcFeature->getPropertyByName("Height"));
                    if (height) {
                        height->setValue(reader->getHeight());
                    }
                }
                else {
                    pcFeature = new Points::FeatureCustom();
                }

                pcFeature->Points.setValue(reader->getPoints());
                // add gray values
                if (reader->hasIntensities()) {
                    Points::PropertyGreyValueList* prop =
                        static_cast<Points::PropertyGreyValueList*>(
                            pcFeature->addDynamicProperty("Points::PropertyGreyValueList",
                                                          "Intensity"));
                    if (prop) {
                        prop->setValues(reader->getIntensities());
                    }
                }
                // add colors
                if (reader->hasColors()) {
                    App::PropertyColorList* prop = static_cast<App::PropertyColorList*>(
                        pcFeature->addDynamicProperty("App::PropertyColorList", "Color"));
                    if (prop) {
                        prop->setValues(reader->getColors());
                    }
                }
                // add normals
                if (reader->hasNormals()) {
                    Points::PropertyNormalList* prop = static_cast<Points::PropertyNormalList*>(
                        pcFeature->addDynamicProperty("Points::PropertyNormalList", "Normal"));
                    if (prop) {
                        prop->setValues(reader->getNormals());
                    }
                }

                // delayed adding of the points feature
                pcDoc->addObject(pcFeature, file.fileNamePure().c_str());
                pcDoc->recomputeFeature(pcFeature);
                pcFeature->purgeTouched();
            }
            else {
                Points::Feature* pcFeature = static_cast<Points::Feature*>(
                    pcDoc->addObject("Points::Feature", file.fileNamePure().c_str()));
                pcFeature->Points.setValue(reader->getPoints());
                pcDoc->recomputeFeature(pcFeature);
                pcFeature->purgeTouched();
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object exporter(const Py::Tuple& args)
    {
        PyObject* object;
        char* Name;

        if (!PyArg_ParseTuple(args.ptr(), "Oet", &object, "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string encodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(encodedName);

        // extract ending
        if (file.extension().empty()) {
            throw Py::RuntimeError("No file extension");
        }

        Py::Sequence list(object);
        Base::Type pointsId = Base::Type::fromName("Points::Feature");
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj =
                    static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(pointsId)) {
                    // get relative placement
                    Points::Feature* fea = static_cast<Points::Feature*>(obj);
                    Base::Placement globalPlacement = fea->globalPlacement();

                    const PointKernel& kernel = fea->Points.getValue();
                    std::unique_ptr<Writer> writer;
                    if (file.hasExtension("asc")) {
                        writer = std::make_unique<AscWriter>(kernel);
                    }
                    else if (file.hasExtension("ply")) {
                        writer = std::make_unique<PlyWriter>(kernel);
                    }
                    else if (file.hasExtension("pcd")) {
                        writer = std::make_unique<PcdWriter>(kernel);
                    }
                    else {
                        throw Py::RuntimeError("Unsupported file extension");
                    }

                    // get additional properties if there
                    App::PropertyInteger* width =
                        dynamic_cast<App::PropertyInteger*>(fea->getPropertyByName("Width"));
                    if (width) {
                        writer->setWidth(width->getValue());
                    }
                    App::PropertyInteger* height =
                        dynamic_cast<App::PropertyInteger*>(fea->getPropertyByName("Height"));
                    if (height) {
                        writer->setHeight(height->getValue());
                    }
                    // get gray values
                    Points::PropertyGreyValueList* grey =
                        dynamic_cast<Points::PropertyGreyValueList*>(
                            fea->getPropertyByName("Intensity"));
                    if (grey) {
                        writer->setIntensities(grey->getValues());
                    }
                    // get colors
                    App::PropertyColorList* col =
                        dynamic_cast<App::PropertyColorList*>(fea->getPropertyByName("Color"));
                    if (col) {
                        writer->setColors(col->getValues());
                    }
                    // get normals
                    Points::PropertyNormalList* nor =
                        dynamic_cast<Points::PropertyNormalList*>(fea->getPropertyByName("Normal"));
                    if (nor) {
                        writer->setNormals(nor->getValues());
                    }

                    writer->setPlacement(globalPlacement);
                    writer->write(encodedName);

                    break;
                }
                else {
                    Base::Console().Message("'%s' is not a point object, export will be ignored.\n",
                                            obj->Label.getValue());
                }
            }
        }

        return Py::None();
    }

    Py::Object show(const Py::Tuple& args)
    {
        PyObject* pcObj;
        char* name = "Points";
        if (!PyArg_ParseTuple(args.ptr(), "O!|s", &(PointsPy::Type), &pcObj, &name)) {
            throw Py::Exception();
        }

        try {
            App::Document* pcDoc = App::GetApplication().getActiveDocument();
            if (!pcDoc) {
                pcDoc = App::GetApplication().newDocument();
            }
            PointsPy* pPoints = static_cast<PointsPy*>(pcObj);
            Points::Feature* pcFeature =
                static_cast<Points::Feature*>(pcDoc->addObject("Points::Feature", name));
            // copy the data
            pcFeature->Points.setValue(*(pPoints->getPointKernelPtr()));
            return Py::asObject(pcFeature->getPyObject());
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Points
