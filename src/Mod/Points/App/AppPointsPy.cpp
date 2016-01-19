/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
#endif

// PCL test
#ifdef HAVE_PCL_IO
#  include <iostream>
#  include <pcl/io/ply_io.h>
#  include <pcl/point_types.h>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>

#include "Points.h"
#include "PointsPy.h"
#include "PointsAlgos.h"
#include "PointsFeature.h"
#include "Properties.h"

namespace Points {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Points")
    {
        add_varargs_method("open",&Module::open
        );
        add_varargs_method("insert",&Module::insert
        );
        add_varargs_method("show",&Module::show
        );
        initialize("This module is the Points module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::Console().Log("Open in Points with %s",EncodedName.c_str());
            Base::FileInfo file(EncodedName.c_str());

            // extract ending
            if (file.extension().empty())
                throw Py::RuntimeError("No file extension");

            if (file.hasExtension("asc")) {
                // create new document and add Import feature
                App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
                Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", file.fileNamePure().c_str());
                Points::PointKernel pkTemp;
                pkTemp.load(EncodedName.c_str());
                pcFeature->Points.setValue( pkTemp );

            }
#ifdef HAVE_PCL_IO
            else if (file.hasExtension("ply")) {
                PlyReader reader;
                reader.read(EncodedName);

                App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
                if (reader.hasProperties()) {
                    Points::FeatureCustom *pcFeature = new Points::FeatureCustom();
                    pcFeature->Points.setValue(reader.getPoints());
                    // add gray values
                    if (reader.hasIntensities()) {
                        Points::PropertyGreyValueList* prop = static_cast<Points::PropertyGreyValueList*>
                            (pcFeature->addDynamicProperty("Points::PropertyGreyValueList", "Intensity"));
                        if (prop) {
                            prop->setValues(reader.getIntensities());
                        }
                    }
                    // add colors
                    if (reader.hasColors()) {
                        App::PropertyColorList* prop = static_cast<App::PropertyColorList*>
                            (pcFeature->addDynamicProperty("App::PropertyColorList", "Color"));
                        if (prop) {
                            prop->setValues(reader.getColors());
                        }
                    }
                    // add normals
                    if (reader.hasNormals()) {
                        Points::PropertyNormalList* prop = static_cast<Points::PropertyNormalList*>
                            (pcFeature->addDynamicProperty("Points::PropertyNormalList", "Normal"));
                        if (prop) {
                            prop->setValues(reader.getNormals());
                        }
                    }

                    // delayed adding of the points feature
                    pcDoc->addObject(pcFeature, file.fileNamePure().c_str());
                    pcDoc->recomputeFeature(pcFeature);
                }
                else {
                    Points::Feature *pcFeature = static_cast<Points::Feature*>
                        (pcDoc->addObject("Points::Feature", file.fileNamePure().c_str()));
                    pcFeature->Points.setValue(reader.getPoints());
                    pcDoc->recomputeFeature(pcFeature);
                }
            }
#endif
            else {
                throw Py::RuntimeError("Unsupported file extension");
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "ets","utf-8",&Name,&DocName))
            throw Py::Exception();
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        try {
            Base::Console().Log("Import in Points with %s",EncodedName.c_str());
            Base::FileInfo file(EncodedName.c_str());

            // extract ending
            if (file.extension().empty())
                throw Py::RuntimeError("No file extension");

            if (file.hasExtension("asc")) {
                // add Import feature
                App::Document *pcDoc = App::GetApplication().getDocument(DocName);
                if (!pcDoc) {
                    pcDoc = App::GetApplication().newDocument(DocName);
                }

                Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", file.fileNamePure().c_str());
                Points::PointKernel pkTemp;
                pkTemp.load(EncodedName.c_str());
                pcFeature->Points.setValue( pkTemp );
            }
#ifdef HAVE_PCL_IO
            else if (file.hasExtension("ply")) {
                App::Document *pcDoc = App::GetApplication().getDocument(DocName);
                if (!pcDoc) {
                    pcDoc = App::GetApplication().newDocument(DocName);
                }

                PlyReader reader;
                reader.read(EncodedName);

                if (reader.hasProperties()) {
                    Points::FeatureCustom *pcFeature = new Points::FeatureCustom();
                    pcFeature->Points.setValue(reader.getPoints());
                    // add gray values
                    if (reader.hasIntensities()) {
                        Points::PropertyGreyValueList* prop = static_cast<Points::PropertyGreyValueList*>
                            (pcFeature->addDynamicProperty("Points::PropertyGreyValueList", "Intensity"));
                        if (prop) {
                            prop->setValues(reader.getIntensities());
                        }
                    }
                    // add colors
                    if (reader.hasColors()) {
                        App::PropertyColorList* prop = static_cast<App::PropertyColorList*>
                            (pcFeature->addDynamicProperty("App::PropertyColorList", "Color"));
                        if (prop) {
                            prop->setValues(reader.getColors());
                        }
                    }
                    // add normals
                    if (reader.hasNormals()) {
                        Points::PropertyNormalList* prop = static_cast<Points::PropertyNormalList*>
                            (pcFeature->addDynamicProperty("Points::PropertyNormalList", "Normal"));
                        if (prop) {
                            prop->setValues(reader.getNormals());
                        }
                    }

                    // delayed adding of the points feature
                    pcDoc->addObject(pcFeature, file.fileNamePure().c_str());
                    pcDoc->recomputeFeature(pcFeature);
                }
                else {
                    Points::Feature *pcFeature = static_cast<Points::Feature*>
                        (pcDoc->addObject("Points::Feature", file.fileNamePure().c_str()));
                    pcFeature->Points.setValue(reader.getPoints());
                    pcDoc->recomputeFeature(pcFeature);
                }
            }
#endif
            else {
                throw Py::RuntimeError("Unsupported file extension");
            }
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }

    Py::Object show(const Py::Tuple& args)
    {
        PyObject *pcObj;
        if (!PyArg_ParseTuple(args.ptr(), "O!", &(PointsPy::Type), &pcObj))
            throw Py::Exception();

        try {
            App::Document *pcDoc = App::GetApplication().getActiveDocument();
            if (!pcDoc)
                pcDoc = App::GetApplication().newDocument();
            PointsPy* pPoints = static_cast<PointsPy*>(pcObj);
            Points::Feature *pcFeature = (Points::Feature *)pcDoc->addObject("Points::Feature", "Points");
            // copy the data
            //TopoShape* shape = new MeshObject(*pShape->getTopoShapeObjectPtr());
            pcFeature->Points.setValue(*(pPoints->getPointKernelPtr()));
            //pcDoc->recompute();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module())->module().ptr();
}

} // namespace Points
