/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
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
# include <Python.h>
# include <QIcon>
# include <QImage>
# include <QFileInfo>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Language/Translator.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include "Workbench.h"
#include "ViewProviderSpreadsheet.h"
#include "SpreadsheetView.h"

// use a different name to CreateCommand()
void CreateSpreadsheetCommands(void);

void loadSpreadsheetResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Spreadsheet);
    Gui::Translator::instance()->refresh();
}

namespace SpreadsheetGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("SpreadsheetGui")
    {
        add_varargs_method("open",&Module::open
        );
        initialize("This module is the SpreadsheetGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        const char* Name;
        const char* DocName=0;
        if (!PyArg_ParseTuple(args.ptr(), "s|s",&Name,&DocName))
            throw Py::Exception();

        try {
            Base::FileInfo file(Name);
            App::Document *pcDoc = App::GetApplication().newDocument(DocName ? DocName : QT_TR_NOOP("Unnamed"));
            Spreadsheet::Sheet *pcSheet = static_cast<Spreadsheet::Sheet *>(pcDoc->addObject("Spreadsheet::Sheet", file.fileNamePure().c_str()));

            pcSheet->importFromFile(Name, '\t', '"', '\\');
            pcSheet->execute();
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }

        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace SpreadsheetGui


/* Python entry */
PyMOD_INIT_FUNC(SpreadsheetGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }

    // instantiating the commands
    CreateSpreadsheetCommands();

    SpreadsheetGui::ViewProviderSheet::init();
    SpreadsheetGui::Workbench::init();
//    SpreadsheetGui::SheetView::init();

    // add resources and reloads the translators
    loadSpreadsheetResource();

    PyObject* mod = SpreadsheetGui::initModule();
    Base::Console().Log("Loading GUI of Spreadsheet module... done\n");
    PyMOD_Return(mod);
}
