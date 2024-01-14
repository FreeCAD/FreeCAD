/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
 *   Copyright (c) 2014 Balázs Bámer                                       *
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

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgPrefsMeasureAppearanceImp.h"
#include "ViewProviderMeasureAngle.h"
#include "ViewProviderMeasureDistance.h"
#include "ViewProviderMeasureBase.h"


// use a different name to CreateCommand()
void CreateMeasureCommands();


namespace MeasureGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MeasureGui")
    {
        initialize("This module is the MeasureGui module.");// register with Python
    }

    ~Module() override = default;

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}// namespace MeasureGui

/* Python entry */
PyMOD_INIT_FUNC(MeasureGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }
    
    // load dependent module
    try {
        Base::Interpreter().loadModule("Measure");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = MeasureGui::initModule();
    Base::Console().Log("Loading GUI of Measure module... done\n");

    // instantiating the commands
    CreateMeasureCommands();

    MeasureGui::ViewProviderMeasureBase                ::init();
    MeasureGui::ViewProviderMeasure        ::init();
    MeasureGui::ViewProviderMeasureAngle               ::init();
    MeasureGui::ViewProviderMeasureDistance            ::init();

    // register preferences pages
    new Gui::PrefPageProducer<MeasureGui::DlgPrefsMeasureAppearanceImp>(QT_TRANSLATE_NOOP("QObject", "Measure"));

//    Q_INIT_RESOURCE(Measure);

    PyMOD_Return(mod);
}
