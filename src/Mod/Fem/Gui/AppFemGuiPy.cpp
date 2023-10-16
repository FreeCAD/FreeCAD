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
#include <QFileInfo>
#endif

#include <App/DocumentObjectPy.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/EditorView.h>
#include <Gui/MainWindow.h>
#include <Gui/TextEdit.h>
#include <Mod/Fem/App/FemAnalysis.h>

#include "AbaqusHighlighter.h"
#include "ActiveAnalysisObserver.h"


namespace FemGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("FemGui")
    {
        add_varargs_method("setActiveAnalysis",
                           &Module::setActiveAnalysis,
                           "setActiveAnalysis(AnalysisObject) -- Set the Analysis object in work.");
        add_varargs_method("getActiveAnalysis",
                           &Module::getActiveAnalysis,
                           "getActiveAnalysis() -- Returns the Analysis object in work.");
        add_varargs_method("open",
                           &Module::open,
                           "open(string) -- Opens an Abaqus file in a text editor.");
        add_varargs_method("insert",
                           &Module::open,
                           "insert(string,string) -- Opens an Abaqus file in a text editor.");
        initialize("This module is the FemGui module.");  // register with Python
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
    Py::Object setActiveAnalysis(const Py::Tuple& args)
    {
        if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
            FemGui::ActiveAnalysisObserver::instance()->highlightActiveObject(
                Gui::HighlightMode::Blue,
                false);
            FemGui::ActiveAnalysisObserver::instance()->setActiveObject(nullptr);
        }

        PyObject* object = nullptr;
        if (PyArg_ParseTuple(args.ptr(), "|O!", &(App::DocumentObjectPy::Type), &object)
            && object) {
            App::DocumentObject* obj =
                static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
            if (!obj || !obj->getTypeId().isDerivedFrom(Fem::FemAnalysis::getClassTypeId())) {
                throw Py::Exception(Base::PyExc_FC_GeneralError,
                                    "Active Analysis object have to be of type Fem::FemAnalysis!");
            }

            // get the gui document of the Analysis Item
            FemGui::ActiveAnalysisObserver::instance()->setActiveObject(
                static_cast<Fem::FemAnalysis*>(obj));
            FemGui::ActiveAnalysisObserver::instance()->highlightActiveObject(
                Gui::HighlightMode::Blue,
                true);
        }

        return Py::None();
    }
    Py::Object getActiveAnalysis(const Py::Tuple& args)
    {
        if (!PyArg_ParseTuple(args.ptr(), "")) {
            throw Py::Exception();
        }
        if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
            return Py::asObject(
                FemGui::ActiveAnalysisObserver::instance()->getActiveObject()->getPyObject());
        }
        return Py::None();
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName;
        if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        QString fileName = QString::fromUtf8(EncodedName.c_str());
        QFileInfo fi;
        fi.setFile(fileName);
        QString ext = fi.completeSuffix().toLower();
        QList<Gui::EditorView*> views = Gui::getMainWindow()->findChildren<Gui::EditorView*>();
        for (auto view : views) {
            if (view->fileName() == fileName) {
                view->setFocus();
                return Py::None();
            }
        }

        if ((ext == QLatin1String("inp")) || (ext == QLatin1String("sif"))
            || (ext == QLatin1String("txt"))) {
            Gui::TextEditor* editor = new Gui::TextEditor();
            editor->setWindowIcon(Gui::BitmapFactory().pixmap(":/icons/fem-solver-inp-editor.svg"));
            Gui::EditorView* edit = new Gui::EditorView(editor, Gui::getMainWindow());
            if (ext == QLatin1String("inp")) {
                editor->setSyntaxHighlighter(new FemGui::AbaqusHighlighter(editor));
            }
            edit->setDisplayName(Gui::EditorView::FileName);
            edit->open(fileName);
            edit->resize(400, 300);
            Gui::getMainWindow()->addWindow(edit);

            QFont font = editor->font();
            font.setFamily(QString::fromLatin1("Arial"));
            editor->setFont(font);
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace FemGui
