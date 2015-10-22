/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <werner.wm.mayer@gmx.de>              *
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
# include <QFileInfo>
#endif

#include <App/DocumentObjectPy.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/MainWindow.h>
#include <Gui/TextEdit.h>
#include <Gui/EditorView.h>

#include <Mod/Fem/App/FemAnalysis.h>
#include "ActiveAnalysisObserver.h"
#include "AbaqusHighlighter.h"


/* module functions */
static PyObject * setActiveAnalysis(PyObject *self, PyObject *args)
{
    if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
        FemGui::ActiveAnalysisObserver::instance()->highlightActiveObject(Gui::Blue,false);
        FemGui::ActiveAnalysisObserver::instance()->setActiveObject(0);
    }

    PyObject *object=0;
    if (PyArg_ParseTuple(args,"|O!",&(App::DocumentObjectPy::Type), &object)&& object) {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        if (!obj || !obj->getTypeId().isDerivedFrom(Fem::FemAnalysis::getClassTypeId())){
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Active Analysis object have to be of type Fem::FemAnalysis!");
            return 0;
        }

        // get the gui document of the Analysis Item
        FemGui::ActiveAnalysisObserver::instance()->setActiveObject(static_cast<Fem::FemAnalysis*>(obj));
        FemGui::ActiveAnalysisObserver::instance()->highlightActiveObject(Gui::Blue,true);
    }

    Py_Return;
}

/* module functions */
static PyObject * getActiveAnalysis(PyObject *self, PyObject *args)
{
    if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
        return FemGui::ActiveAnalysisObserver::instance()->getActiveObject()->getPyObject();
    }

    Py_Return;
}

/* module functions */
static PyObject * open(PyObject *self, PyObject *args)
{
    char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "et|s","utf-8",&Name,&DocName))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    PY_TRY {
        QString fileName = QString::fromUtf8(EncodedName.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.completeSuffix().toLower();
        QList<Gui::EditorView*> views = Gui::getMainWindow()->findChildren<Gui::EditorView*>();
        for (QList<Gui::EditorView*>::Iterator it = views.begin(); it != views.end(); ++it) {
            if ((*it)->fileName() == fileName) {
                (*it)->setFocus();
                Py_Return;
            }
        }

        if (ext == QLatin1String("inp")) {
            Gui::TextEditor* editor = new Gui::TextEditor();
            editor->setWindowIcon(Gui::BitmapFactory().pixmap(":/icons/fem-inp-editor.svg"));
            Gui::EditorView* edit = new Gui::EditorView(editor, Gui::getMainWindow());
            editor->setSyntaxHighlighter(new FemGui::AbaqusHighlighter(editor));
            edit->setDisplayName(Gui::EditorView::FileName);
            edit->open(fileName);
            edit->resize(400, 300);
            Gui::getMainWindow()->addWindow(edit);

            QFont font = editor->font();
            font.setFamily(QString::fromLatin1("Arial"));
            editor->setFont(font);
        }
    } PY_CATCH;

    Py_Return;
}



/* registration table  */
struct PyMethodDef FemGui_Import_methods[] = {
    {"setActiveAnalysis"       ,setActiveAnalysis      ,METH_VARARGS,
     "setActiveAnalysis(AnalysisObject) -- Set the Analysis object in work."},
    {"getActiveAnalysis"       ,getActiveAnalysis      ,METH_VARARGS,
     "getActiveAnalysis() -- Returns the Analysis object in work."},
    {"open"                    ,open                   ,METH_VARARGS,
     "open(string) -- Opens an Abaqus file in a text editor."},
    {"insert"                  ,open                   ,METH_VARARGS,
     "insert(string,string) -- Opens an Abaqus file in a text editor."},
    {NULL, NULL}                   /* end of table marker */
};
