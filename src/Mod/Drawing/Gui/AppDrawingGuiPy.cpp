/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <sstream>

#include <QFileInfo>
#endif

#include <App/DocumentObjectPy.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>

#include <Mod/Drawing/App/FeaturePage.h>
#include <Mod/Drawing/App/FeatureViewPart.h>
#include <Mod/Drawing/App/ProjectionAlgos.h>
#include <Mod/Part/App/PartFeature.h>

#include "DrawingView.h"


namespace DrawingGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("DrawingGui")
    {
        add_varargs_method("open", &Module::open);
        add_varargs_method("insert", &Module::importer);
        add_varargs_method("export", &Module::exporter);
        initialize("This module is the DrawingGui module.");  // register with Python
    }

    virtual ~Module()
    {}

private:
    virtual Py::Object invoke_method_varargs(void* method_def, const Py::Tuple& args)
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
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());
        if (file.hasExtension({"svg", "svgz"})) {
            QString fileName = QString::fromUtf8(EncodedName.c_str());
            // Displaying the image in a view
            DrawingView* view = new DrawingView(nullptr, Gui::getMainWindow());
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));
            QFileInfo fi(fileName);
            view->setWindowTitle(fi.fileName());
            view->resize(400, 300);
            Gui::getMainWindow()->addWindow(view);
        }
        else {
            throw Py::Exception(PyExc_IOError, "unknown filetype");
        }

        return Py::None();
    }
    Py::Object importer(const Py::Tuple& args)
    {
        char* Name;
        const char* dummy;
        if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &Name, &dummy)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Base::FileInfo file(EncodedName.c_str());
        if (file.hasExtension({"svg", "svgz"})) {
            QString fileName = QString::fromUtf8(EncodedName.c_str());
            // Displaying the image in a view
            DrawingView* view = new DrawingView(nullptr, Gui::getMainWindow());
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));
            QFileInfo fi(fileName);
            view->setWindowTitle(fi.fileName());
            view->resize(400, 300);
            Gui::getMainWindow()->addWindow(view);
        }
        else {
            throw Py::Exception(PyExc_IOError, "unknown filetype");
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

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj =
                    static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(Drawing::FeaturePage::getClassTypeId())) {
                    Base::FileInfo fi_out(EncodedName.c_str());
                    Base::ofstream str_out(fi_out, std::ios::out | std::ios::binary);
                    if (!str_out) {
                        std::stringstream str;
                        str << "Cannot open file '" << EncodedName << "' for writing";
                        throw Py::Exception(PyExc_IOError, str.str().c_str());
                    }
                    if (fi_out.hasExtension("svg")) {
                        std::string fn =
                            static_cast<Drawing::FeaturePage*>(obj)->PageResult.getValue();
                        Base::FileInfo fi_in(fn);
                        Base::ifstream str_in(fi_in, std::ios::in | std::ios::binary);
                        if (!str_in) {
                            std::stringstream str;
                            str << "Cannot open file '" << fn << "' for reading";
                            throw Py::Exception(PyExc_IOError, str.str().c_str());
                        }

                        str_in >> str_out.rdbuf();
                        str_in.close();
                        str_out.close();
                        break;
                    }
                    else if (fi_out.hasExtension("dxf")) {
                        const std::vector<App::DocumentObject*>& views =
                            static_cast<Drawing::FeaturePage*>(obj)->Group.getValues();
                        for (std::vector<App::DocumentObject*>::const_iterator it = views.begin();
                             it != views.end();
                             ++it) {
                            if ((*it)->getTypeId().isDerivedFrom(
                                    Drawing::FeatureViewPart::getClassTypeId())) {
                                Drawing::FeatureViewPart* view =
                                    static_cast<Drawing::FeatureViewPart*>(*it);
                                App::DocumentObject* link = view->Source.getValue();
                                if (!link) {
                                    throw Py::ValueError("No object linked");
                                }
                                if (!link->getTypeId().isDerivedFrom(
                                        Part::Feature::getClassTypeId())) {
                                    throw Py::TypeError("Linked object is not a Part object");
                                }
                                TopoDS_Shape shape =
                                    static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
                                if (!shape.IsNull()) {
                                    Base::Vector3d dir = view->Direction.getValue();
                                    bool hidden = view->ShowHiddenLines.getValue();
                                    bool smooth = view->ShowSmoothLines.getValue();
                                    Drawing::ProjectionAlgos::ExtractionType type =
                                        Drawing::ProjectionAlgos::Plain;
                                    if (hidden) {
                                        type = (Drawing::ProjectionAlgos::ExtractionType)(
                                            type | Drawing::ProjectionAlgos::WithHidden);
                                    }
                                    if (smooth) {
                                        type = (Drawing::ProjectionAlgos::ExtractionType)(
                                            type | Drawing::ProjectionAlgos::WithSmooth);
                                    }
                                    float scale = view->Scale.getValue();
                                    float tol = view->Tolerance.getValue();

                                    Drawing::ProjectionAlgos project(shape, dir);
                                    str_out << project.getDXF(type, scale, tol);
                                    break;  // TODO: How to add several shapes?
                                }
                            }
                        }
                        str_out.close();
                        break;
                    }
                    else {
                        throw Py::TypeError("Export of page object as this file format is not "
                                            "supported by Drawing module");
                    }
                }
                else {
                    throw Py::TypeError(
                        "Export of this object type is not supported by Drawing module");
                }
            }
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace DrawingGui
