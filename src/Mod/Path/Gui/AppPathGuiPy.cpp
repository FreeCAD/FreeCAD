/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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
#endif

#include <QDir>
#include <QFileInfo>

#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/FileInfo.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>

#include <Gui/Command.h>
#include <Gui/WaitCursor.h>

#include "ViewProviderPath.h"
#include "DlgProcessorChooser.h"
#include "ui_DlgProcessorChooser.h"

using namespace PathGui;

static PyObject * open(PyObject *self, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    Base::FileInfo fi(EncodedName);
    if (!fi.exists())
        Py_Error(Base::BaseExceptionFreeCADError, "File not found");
    Gui::WaitCursor wc;
    wc.restoreCursor();

    PY_TRY {
        std::string path = App::GetApplication().getHomePath();
        path += "Mod/Path/PathScripts/";
        QDir dir1(QString::fromUtf8(path.c_str()), QString::fromAscii("*_pre.py"));
        std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
            ->GetASCII("MacroPath",App::Application::getUserAppDataDir().c_str());
        QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromAscii("*_pre.py"));
        QFileInfoList list = dir1.entryInfoList();
        list << dir2.entryInfoList();
        std::vector<std::string> scripts;
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            scripts.push_back(fileInfo.baseName().toStdString());
        }
        std::string selected;
        PathGui::DlgProcessorChooser Dlg(scripts);
        if (Dlg.exec() != QDialog::Accepted) {
            Py_Return;
        }
        selected = Dlg.getSelected();
    
        std::ostringstream pre;
        std::ostringstream cmd;
        if (selected.empty()) {
            App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
            Gui::Command::runCommand(Gui::Command::Gui,"import Path");
            cmd << "Path.read(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
            Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
        } else {
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                if (fileInfo.baseName().toStdString() == selected) {
                    if (fileInfo.absoluteFilePath().contains(QString::fromAscii("PathScripts"))) {
                        pre << "from PathScripts import " << selected;
                    } else {
                        pre << "import " << selected;
                    }
                    Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                    cmd << selected << ".open(\"" << EncodedName << "\")";
                    Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                }
            }
        }
    } PY_CATCH;
    Py_Return;
}

static PyObject * importer(PyObject *self, PyObject *args)
{
    char* Name;
    char* DocName=0;
    if (!PyArg_ParseTuple(args, "et|s","utf-8",&Name,&DocName))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    Base::FileInfo fi(EncodedName);
    if (!fi.exists())
        Py_Error(Base::BaseExceptionFreeCADError, "File not found");
    Gui::WaitCursor wc;
    wc.restoreCursor();

    PY_TRY {
        std::string path = App::GetApplication().getHomePath();
        path += "Mod/Path/PathScripts/";
        QDir dir1(QString::fromUtf8(path.c_str()), QString::fromAscii("*_pre.py"));
        std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
            ->GetASCII("MacroPath",App::Application::getUserAppDataDir().c_str());
        QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromAscii("*_pre.py"));
        QFileInfoList list = dir1.entryInfoList();
        list << dir2.entryInfoList();
        std::vector<std::string> scripts;
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            scripts.push_back(fileInfo.baseName().toStdString());
        }
        std::string selected;
        PathGui::DlgProcessorChooser Dlg(scripts);
        if (Dlg.exec() != QDialog::Accepted) {
            Py_Return;
        }
        selected = Dlg.getSelected();

        App::Document *pcDoc = 0;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();

        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        std::ostringstream pre;
        std::ostringstream cmd;
        if (selected.empty()) {
            Gui::Command::runCommand(Gui::Command::Gui,"import Path");
            cmd << "Path.read(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
            Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
        } else {
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                if (fileInfo.baseName().toStdString() == selected) {
                    if (fileInfo.absoluteFilePath().contains(QString::fromAscii("PathScripts"))) {
                        pre << "from PathScripts import " << selected;
                    } else {
                        pre << "import " << selected;
                    }
                    Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                    cmd << selected << ".insert(\"" << EncodedName << "\",\"" << pcDoc->getName() << "\")";
                    Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                }
            }
        }
    } PY_CATCH;
    Py_Return;
}

static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    char* Name;
    if (!PyArg_ParseTuple(args, "Oet",&object,"utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    Gui::WaitCursor wc;
    wc.restoreCursor();

    PY_TRY {
        Py::Sequence objlist(object);
        if (objlist.size() == 0)
            Py_Error(Base::BaseExceptionFreeCADError, "No object to export");
        std::string path = App::GetApplication().getHomePath();
        path += "Mod/Path/PathScripts/";
        QDir dir1(QString::fromUtf8(path.c_str()), QString::fromAscii("*_post.py"));
        std::string cMacroPath = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
            ->GetASCII("MacroPath",App::Application::getUserAppDataDir().c_str());
        QDir dir2(QString::fromUtf8(cMacroPath.c_str()), QString::fromAscii("*_post.py"));
        QFileInfoList list = dir1.entryInfoList();
        list << dir2.entryInfoList();
        std::vector<std::string> scripts;
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            scripts.push_back(fileInfo.baseName().toStdString());
        }
        std::string selected;
        PathGui::DlgProcessorChooser Dlg(scripts);
        if (Dlg.exec() != QDialog::Accepted) {
            Py_Return;
        }
        selected = Dlg.getSelected();
            
        std::ostringstream pre;
        std::ostringstream cmd;
        if (selected.empty()) {
            if (objlist.size() > 1) {
                Py_Error(Base::BaseExceptionFreeCADError, "Cannot export more than one object without using a post script");
            }
            PyObject* item = objlist[0].ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                App::Document* doc = obj->getDocument();
                Gui::Command::runCommand(Gui::Command::Gui,"import Path");
                cmd << "Path.write(FreeCAD.getDocument(\"" << doc->getName() << "\").getObject(\"" << obj->getNameInDocument() << "\"),\"" << EncodedName << "\")";            
                Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
            } else {
                Py_Return;
            }
        } else {
            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                if (fileInfo.baseName().toStdString() == selected) {
                    if (fileInfo.absoluteFilePath().contains(QString::fromAscii("PathScripts"))) {
                        pre << "from PathScripts import " << selected;
                    } else {
                        pre << "import " << selected;
                    }
                    Gui::Command::runCommand(Gui::Command::Gui,pre.str().c_str());
                    cmd << selected << ".export(__objs__,\"" << EncodedName << "\")";
                    Gui::Command::runCommand(Gui::Command::Gui,cmd.str().c_str());
                }
            }
        }
    } PY_CATCH;
    Py_Return;
}

/* registration table  */
struct PyMethodDef PathGui_methods[] = {
    {"open"        ,open       ,METH_VARARGS,
     "open(filename): Opens a GCode file as a new document"},
    {"insert"      ,importer   ,METH_VARARGS,
     "insert(filename,docname): Imports a given GCode file into the given document"},
    {"export"      ,exporter   ,METH_VARARGS,
     "export(objectslist,filename): Exports a given list of Path objects to a GCode file"},
    {NULL, NULL}        /* end of table marker */
};
