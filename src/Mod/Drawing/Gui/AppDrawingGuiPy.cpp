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
# include <QIcon>
# include <QImage>
# include <sstream>
#endif

#include "DrawingView.h"
#include <Mod/Drawing/App/FeaturePage.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <App/Application.h>
#include <App/DocumentObjectPy.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>

using namespace DrawingGui;


/* module functions */
static PyObject * 
open(PyObject *self, PyObject *args) 
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL; 
    
    PY_TRY {
        Base::FileInfo file(Name);
        if (file.hasExtension("svg") || file.hasExtension("svgz")) {
            QString fileName = QString::fromUtf8(Name);
            // Displaying the image in a view
            DrawingView* view = new DrawingView(Gui::getMainWindow());
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));
            view->setWindowTitle(QObject::tr("Drawing viewer"));
            view->resize( 400, 300 );
            Gui::getMainWindow()->addWindow(view);
        }
        else {
            PyErr_SetString(PyExc_Exception, "unknown filetype");
            return NULL;
        }
    } PY_CATCH;

    Py_Return; 
}

/* module functions */
static PyObject *
importer(PyObject *self, PyObject *args)
{
    const char* Name;
    const char* dummy;
    if (!PyArg_ParseTuple(args, "s|s",&Name,&dummy))
        return NULL; 
    
    PY_TRY {
        Base::FileInfo file(Name);
        if (file.hasExtension("svg") || file.hasExtension("svgz")) {
            QString fileName = QString::fromUtf8(Name);
            // Displaying the image in a view
            DrawingView* view = new DrawingView(Gui::getMainWindow());
            view->load(fileName);
            view->setWindowIcon(Gui::BitmapFactory().pixmap("actions/drawing-landscape"));
            view->setWindowTitle(QObject::tr("Drawing viewer"));
            view->resize( 400, 300 );
            Gui::getMainWindow()->addWindow(view);
        } else {
            PyErr_SetString(PyExc_Exception, "unknown filetype");
            return NULL;
        }
    } PY_CATCH;

    Py_Return; 
}

static PyObject * 
exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    PY_TRY {
        Py::List list(object);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(Drawing::FeaturePage::getClassTypeId())) {
                    std::string fn = static_cast<Drawing::FeaturePage*>(obj)->PageResult.getValue();
                    Base::FileInfo fi_in(fn);
                    Base::ifstream str_in(fi_in, std::ios::in | std::ios::binary);
                    if (!str_in) {
                        std::stringstream str;
                        str << "Cannot open file '" << fn << "' for reading";
                        PyErr_SetString(PyExc_IOError, str.str().c_str());
                        return NULL;
                    }

                    Base::FileInfo fi_out(filename);
                    Base::ofstream str_out(fi_out, std::ios::out | std::ios::binary);
                    if (!str_out) {
                        std::stringstream str;
                        str << "Cannot open file '" << filename << "' for writing";
                        PyErr_SetString(PyExc_IOError, str.str().c_str());
                        return NULL;
                    }

                    str_in >> str_out.rdbuf();
                    str_in.close();
                    str_out.close();
                    break;
                }
            }
        }
    } PY_CATCH;

    Py_Return;
}

/* registration table  */
struct PyMethodDef DrawingGui_Import_methods[] = {
    {"open"     ,open ,     METH_VARARGS}, /* method name, C func ptr, always-tuple */
    {"insert"   ,importer,  METH_VARARGS},
    {"export"   ,exporter,  METH_VARARGS},
    {NULL, NULL}                    /* end of table marker */
};
