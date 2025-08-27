/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#endif
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgPrefsMeasureAppearanceImp.h"
#include "QuickMeasure.h"
#include "QuickMeasurePy.h"
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
        initialize("This module is the MeasureGui module.");  // register with Python
    }

    ~Module() override = default;

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MeasureGui

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
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = MeasureGui::initModule();
    Base::Console().log("Loading GUI of Measure module… done\n");

    // instantiating the commands
    CreateMeasureCommands();

    // clang-format off
    MeasureGui::DimensionLinear::initClass();

    MeasureGui::ViewProviderMeasureGroup               ::init();
    MeasureGui::ViewProviderMeasureBase                ::init();
    MeasureGui::ViewProviderMeasure                    ::init();
    MeasureGui::ViewProviderMeasureAngle               ::init();
    MeasureGui::ViewProviderMeasureDistance            ::init();

    MeasureGui::ViewProviderMeasureArea                ::init();
    MeasureGui::ViewProviderMeasureLength              ::init();
    MeasureGui::ViewProviderMeasurePosition            ::init();
    MeasureGui::ViewProviderMeasureRadius              ::init();
    // clang-format on

    // register preferences pages
    new Gui::PrefPageProducer<MeasureGui::DlgPrefsMeasureAppearanceImp>(
        QT_TRANSLATE_NOOP("QObject", "Measure"));

    //    Q_INIT_RESOURCE(Measure);

    Base::Interpreter().addType(&MeasureGui::QuickMeasurePy::Type, mod, "QuickMeasure");

    // Create a QuickMeasure instance
    auto measure = new MeasureGui::QuickMeasure(QApplication::instance());
    Q_UNUSED(measure)

    PyMOD_Return(mod);
}
