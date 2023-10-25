/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
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
#include <Base/Parameter.h>

#include "Blending/BlendCurvePy.h"
#include "Blending/BlendPointPy.h"
#include "Blending/FeatureBlendCurve.h"

#include "FeatureCut.h"
#include "FeatureExtend.h"
#include "FeatureFilling.h"
#include "FeatureGeomFillSurface.h"
#include "FeatureSections.h"
#include "FeatureSewing.h"


namespace Surface
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Surface")
    {
        initialize("This module is the Surface module.");  // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Surface

/* Python entry */
PyMOD_INIT_FUNC(Surface)
{
    try {
        Base::Interpreter().runString("import Part");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = Surface::initModule();
    Base::Console().Log("Loading Surface module... done\n");
    Base::Interpreter().addType(&Surface::BlendPointPy::Type, mod, "BlendPoint");
    Base::Interpreter().addType(&Surface::BlendCurvePy::Type, mod, "BlendCurve");

    // clang-format off
    // Add types to module
    Surface::Filling           ::init();
    Surface::Sewing            ::init();
    Surface::Cut               ::init();
    Surface::GeomFillSurface   ::init();
    Surface::Extend            ::init();
    Surface::FeatureBlendCurve ::init();
    Surface::Sections          ::init();
    // clang-format on

    PyMOD_Return(mod);
}
