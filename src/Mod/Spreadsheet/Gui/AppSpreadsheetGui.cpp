/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/MainWindow.h>
#include <Gui/WidgetFactory.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "DlgSettingsImp.h"
#include "SheetTableViewAccessibleInterface.h"
#include "ViewProviderSpreadsheet.h"
#include "Workbench.h"

// use a different name to CreateCommand()
void CreateSpreadsheetCommands();

void loadSpreadsheetResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Spreadsheet);
    Q_INIT_RESOURCE(Spreadsheet_translation);
    Gui::Translator::instance()->refresh();
}

namespace SpreadsheetGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("SpreadsheetGui")
    {
        add_varargs_method("open", &Module::open);
        add_varargs_method("insert", &Module::insert);
        initialize("This module is the SpreadsheetGui module.");  // register with Python
    }

private:
    void load(App::Document* pcDoc, const std::string& Name)
    {
        try {
            Base::FileInfo file(Name);
            auto filename = file.fileNamePure();
            auto* pcSheet = pcDoc->addObject<Spreadsheet::Sheet>(filename.c_str());

            pcSheet->importFromFile(Name, '\t', '"', '\\');
            pcSheet->execute();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        App::Document* pcDoc =
            App::GetApplication().newDocument(DocName ? DocName : QT_TR_NOOP("Unnamed"));
        load(pcDoc, EncodedName);

        return Py::None();
    }

    Py::Object insert(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "et|s", "utf-8", &Name, &DocName)) {
            throw Py::Exception();
        }
        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        App::Document* pcDoc = App::GetApplication().getDocument(DocName);
        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName ? DocName : QT_TR_NOOP("Unnamed"));
        }
        load(pcDoc, EncodedName);

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace SpreadsheetGui

/* Python entry */
PyMOD_INIT_FUNC(SpreadsheetGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // instantiating the commands
    CreateSpreadsheetCommands();

#if defined(Q_OS_WIN32)
    QAccessible::installFactory(SpreadsheetGui::SheetTableViewAccessibleInterface::ifactory);
#endif

    SpreadsheetGui::ViewProviderSheet::init();
    SpreadsheetGui::ViewProviderSheetPython::init();
    SpreadsheetGui::Workbench::init();
    SpreadsheetGui::SheetView::init();
    SpreadsheetGui::SheetViewPy::init_type();

    // register preference page
    new Gui::PrefPageProducer<SpreadsheetGui::DlgSettingsImp>(
        QT_TRANSLATE_NOOP("QObject", "Spreadsheet"));

    // add resources and reloads the translators
    loadSpreadsheetResource();

    PyObject* mod = SpreadsheetGui::initModule();
    Base::Console().log("Loading GUI of Spreadsheet module… done\n");
    PyMOD_Return(mod);
}
