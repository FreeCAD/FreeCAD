/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>
#include "Workbench.h"
#include "ViewProviderSketch.h"
#include "ViewProviderPython.h"
#include "SoDatumLabel.h"
#include "SoZoomTranslation.h"
#include "SketcherSettings.h"
#include "PropertyConstraintListItem.h"
#include "ViewProviderSketchGeometryExtension.h"


// create the commands
void CreateSketcherCommands(void);
void CreateSketcherCommandsCreateGeo(void);
void CreateSketcherCommandsConstraints(void);
void CreateSketcherCommandsConstraintAccel(void);
void CreateSketcherCommandsAlterGeo(void);
void CreateSketcherCommandsBSpline(void);
void CreateSketcherCommandsVirtualSpace(void);

void loadSketcherResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Sketcher);
    Gui::Translator::instance()->refresh();
}


namespace SketcherGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("SketcherGui")
    {
        initialize("This module is the SketcherGui module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace SketcherGui

/* Python entry */
PyMOD_INIT_FUNC(SketcherGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }
    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import Sketcher");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    PyObject* mod = SketcherGui::initModule();
    Base::Console().Log("Loading GUI of Sketcher module... done\n");

    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/constraints"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/elements"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/general"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/geometry"));
  //Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/obsolete"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/pointers"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/splines"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/tools"));

    // instantiating the commands
    CreateSketcherCommands();
    CreateSketcherCommandsCreateGeo();
    CreateSketcherCommandsConstraints();
    CreateSketcherCommandsAlterGeo();
    CreateSketcherCommandsConstraintAccel();
    CreateSketcherCommandsBSpline();
    CreateSketcherCommandsVirtualSpace();

    SketcherGui::Workbench::init();

    // init objects
    SketcherGui::ViewProviderSketch         		  ::init();
    SketcherGui::ViewProviderPython         		  ::init();
    SketcherGui::ViewProviderCustom         		  ::init();
    SketcherGui::ViewProviderCustomPython   		  ::init();
    SketcherGui::SoDatumLabel               		  ::initClass();
    SketcherGui::SoZoomTranslation          		  ::initClass();
    SketcherGui::PropertyConstraintListItem 		  ::init();
    SketcherGui::ViewProviderSketchGeometryExtension  ::init();

    (void)new Gui::PrefPageProducer<SketcherGui::SketcherSettings>        ( QT_TRANSLATE_NOOP("QObject","Sketcher") );
    (void)new Gui::PrefPageProducer<SketcherGui::SketcherSettingsDisplay> ( QT_TRANSLATE_NOOP("QObject","Sketcher") );
    (void)new Gui::PrefPageProducer<SketcherGui::SketcherSettingsColors>  ( QT_TRANSLATE_NOOP("QObject","Sketcher") );

     // add resources and reloads the translators
    loadSketcherResource();

    PyMOD_Return(mod);
}
