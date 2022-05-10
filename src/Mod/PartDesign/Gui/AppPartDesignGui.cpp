/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>

#include "Workbench.h"
#include "ViewProviderPocket.h"
#include "ViewProviderHole.h"
#include "ViewProviderBody.h"
#include "ViewProviderSketchBased.h"
#include "ViewProviderPad.h"
#include "ViewProviderChamfer.h"
#include "ViewProviderFillet.h"
#include "ViewProviderDraft.h"
#include "ViewProviderDressUp.h"
#include "ViewProviderRevolution.h"
#include "ViewProviderGroove.h"
#include "ViewProviderMirrored.h"
#include "ViewProviderLinearPattern.h"
#include "ViewProviderPolarPattern.h"
#include "ViewProviderScaled.h"
#include "ViewProviderMultiTransform.h"
#include "ViewProviderTransformed.h"
#include "ViewProviderDatumPoint.h"
#include "ViewProviderDatumLine.h"
#include "ViewProviderDatumPlane.h"
#include "ViewProviderBoolean.h"
#include "ViewProviderPrimitive.h"
#include "ViewProviderDatumCS.h"
#include "ViewProviderThickness.h"
#include "ViewProviderPipe.h"
#include "ViewProviderLoft.h"
#include "ViewProviderHelix.h"
#include "ViewProviderShapeBinder.h"
#include "ViewProviderBase.h"


// use a different name to CreateCommand()
void CreatePartDesignCommands(void);
void CreatePartDesignBodyCommands(void);
void CreatePartDesignPrimitiveCommands(void);

void loadPartDesignResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(PartDesign);
    Gui::Translator::instance()->refresh();
}

namespace PartDesignGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("PartDesignGui")
    {
        initialize("This module is the PartDesignGui module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace PartDesignGui


/* Python entry */
PyMOD_INIT_FUNC(PartDesignGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import SketcherGui");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = PartDesignGui::initModule();
    Base::Console().Log("Loading GUI of PartDesign module... done\n");

    // instantiating the commands
    CreatePartDesignCommands();
    CreatePartDesignBodyCommands();
    CreatePartDesignPrimitiveCommands();

    PartDesignGui::Workbench                 ::init();
    PartDesignGui::ViewProvider              ::init();
    PartDesignGui::ViewProviderPython        ::init();
    PartDesignGui::ViewProviderBody          ::init();
    PartDesignGui::ViewProviderSketchBased   ::init();
    PartDesignGui::ViewProviderPocket        ::init();
    PartDesignGui::ViewProviderHole          ::init();
    PartDesignGui::ViewProviderPad           ::init();
    PartDesignGui::ViewProviderRevolution    ::init();
    PartDesignGui::ViewProviderDressUp       ::init();
    PartDesignGui::ViewProviderGroove        ::init();
    PartDesignGui::ViewProviderChamfer       ::init();
    PartDesignGui::ViewProviderFillet        ::init();
    PartDesignGui::ViewProviderDraft         ::init();
    PartDesignGui::ViewProviderThickness     ::init();
    PartDesignGui::ViewProviderTransformed   ::init();
    PartDesignGui::ViewProviderMirrored      ::init();
    PartDesignGui::ViewProviderLinearPattern ::init();
    PartDesignGui::ViewProviderPolarPattern  ::init();
    PartDesignGui::ViewProviderScaled        ::init();
    PartDesignGui::ViewProviderMultiTransform::init();
    PartDesignGui::ViewProviderDatum         ::init();
    PartDesignGui::ViewProviderDatumPoint    ::init();
    PartDesignGui::ViewProviderDatumLine     ::init();
    PartDesignGui::ViewProviderDatumPlane    ::init();
    PartDesignGui::ViewProviderDatumCoordinateSystem::init();
    PartDesignGui::ViewProviderShapeBinder   ::init();
    PartDesignGui::ViewProviderSubShapeBinder::init();
    PartDesignGui::ViewProviderSubShapeBinderPython::init();
    PartDesignGui::ViewProviderBoolean       ::init();
    PartDesignGui::ViewProviderAddSub        ::init();
    PartDesignGui::ViewProviderPrimitive     ::init();
    PartDesignGui::ViewProviderPipe          ::init();
    PartDesignGui::ViewProviderLoft          ::init();
    PartDesignGui::ViewProviderHelix         ::init();
    PartDesignGui::ViewProviderBase          ::init();

     // add resources and reloads the translators
    loadPartDesignResource();

    PyMOD_Return(mod);
}
