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
# include <QFileInfo>
# include <QIcon>
# include <QImage>
# include <sstream>
#endif

#include "MDIViewPage.h"
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
//#include <Mod/Drawing/App/ProjectionAlgos.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <App/Application.h>
#include <App/DocumentObjectPy.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>

using namespace TechDrawGui;

//TODO: TechDraw does not open/import/export SVG/DXF files.  Not sure what belongs here.
/* module functions */
static PyObject *
open(PyObject *self, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        Base::FileInfo file(EncodedName.c_str());
        if (file.hasExtension("svg") || file.hasExtension("svgz")) {
            QString fileName = QString::fromUtf8(EncodedName.c_str());
#if 0
            // Displaying the image in a view
            MDIViewPage* view = new MDIViewPage(0, 0, Gui::getMainWindow());    //TODO: hack to get a compile
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/techdraw-landscape"));
            QFileInfo fi(fileName);
            view->setWindowTitle(fi.fileName());
            view->resize( 400, 300 );
            Gui::getMainWindow()->addWindow(view);
#endif
        }
        else {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "unknown filetype");
            return NULL;
        }
    } PY_CATCH;

    Py_Return;
}

/* module functions */
static PyObject *
importer(PyObject *self, PyObject *args)
{
    char* Name;
    const char* dummy;
    if (!PyArg_ParseTuple(args, "et|s","utf-8",&Name,&dummy))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        Base::FileInfo file(EncodedName.c_str());
        if (file.hasExtension("svg") || file.hasExtension("svgz")) {
            QString fileName = QString::fromUtf8(EncodedName.c_str());

#if 0
            // Displaying the image in a view
            MDIViewPage* view = new MDIViewPage(0, 0, Gui::getMainWindow());    //TODO: hack to get a compile
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/techdraw-landscape"));
            QFileInfo fi(fileName);
            view->setWindowTitle(fi.fileName());
            view->resize( 400, 300 );
            Gui::getMainWindow()->addWindow(view);
#endif
        } else {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "unknown filetype");
            return NULL;
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject *
exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    char* Name;
    if (!PyArg_ParseTuple(args, "Oet",&object,"utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
        Py::Sequence list(object);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(TechDraw::DrawPage::getClassTypeId())) {
                    Base::FileInfo fi_out(EncodedName.c_str());
                    Base::ofstream str_out(fi_out, std::ios::out | std::ios::binary);
                    if (!str_out) {
                        std::stringstream str;
                        str << "Cannot open file '" << EncodedName << "' for writing";
                        PyErr_SetString(PyExc_IOError, str.str().c_str());
                        return NULL;
                    }
                    if (fi_out.hasExtension("svg")) {
//                        std::string fn = static_cast<TechDraw::DrawPage*>(obj)->PageResult.getValue();
                        std::string fn;
                        Base::FileInfo fi_in(fn);
                        Base::ifstream str_in(fi_in, std::ios::in | std::ios::binary);
                        if (!str_in) {
                            std::stringstream str;
                            str << "Cannot open file '" << fn << "' for reading";
                            PyErr_SetString(PyExc_IOError, str.str().c_str());
                            return NULL;
                        }

                        str_in >> str_out.rdbuf();
                        str_in.close();
                        str_out.close();
                        break;
                    }
                    else if (fi_out.hasExtension("dxf")) {
                        const std::vector<App::DocumentObject*>& views = static_cast<TechDraw::DrawPage*>(obj)->Views.getValues();
                        for (std::vector<App::DocumentObject*>::const_iterator it = views.begin(); it != views.end(); ++it) {
                            if ((*it)->getTypeId().isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
                                TechDraw::DrawViewPart* view = static_cast<TechDraw::DrawViewPart*>(*it);
                                std::string viewName = view->Label.getValue();
                                App::DocumentObject* link = view->Source.getValue();
                                if (!link) {
                                    PyErr_SetString(Base::BaseExceptionFreeCADError, "No object linked");
                                    return 0;
                                }
                                if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                                    PyErr_SetString(PyExc_TypeError, "Linked object is not a Part object");
                                    return 0;
                                }
                                TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape()._Shape;
                                if (!shape.IsNull()) {
#if 0
                                    Base::Vector3d dir = view->Direction.getValue();
                                    bool hidden = view->ShowHiddenLines.getValue();
                                    bool smooth = view->ShowSmoothLines.getValue();
                                    Drawing::ProjectionAlgos::ExtractionType type = Drawing::ProjectionAlgos::Plain;
                                    if (hidden) type = (Drawing::ProjectionAlgos::ExtractionType)(type|Drawing::ProjectionAlgos::WithHidden);
                                    if (smooth) type = (Drawing::ProjectionAlgos::ExtractionType)(type|Drawing::ProjectionAlgos::WithSmooth);
                                    float scale = view->Scale.getValue();
                                    float tol = view->Tolerance.getValue();

                                    Drawing::ProjectionAlgos project(shape, dir);
                                    str_out << project.getDXF(type, scale, tol);
#endif
                                    break; // TODO: How to add several shapes?
                                }
                            }
                        }
                        str_out.close();
                        break;
                    }
                    else {
                        PyErr_SetString(PyExc_TypeError, "Export of page object as this file format is not supported by Drawing module");
                        return 0;
                    }
                }
                else {
                    PyErr_SetString(PyExc_TypeError, "Export of this object type is not supported by Drawing module");
                    return 0;
                }
            }
        }
    } PY_CATCH;

    Py_Return;
}

/* registration table  */
struct PyMethodDef TechDrawGui_Import_methods[] = {
    {"open"     ,open ,     METH_VARARGS}, /* method name, C func ptr, always-tuple */
    {"insert"   ,importer,  METH_VARARGS},
    {"export"   ,exporter,  METH_VARARGS},
    {NULL, NULL}                    /* end of table marker */
};
