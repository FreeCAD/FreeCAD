/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
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

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include "FeatureFilling.h"
#include "FeatureSewing.h"
#include "FeatureCut.h"
#include "FeatureGeomFillSurface.h"
#include "FeatureExtend.h"
#include "FeatureSections.h"

#include <Base/Interpreter.h>
#include <Base/Parameter.h>


namespace Surface {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Surface")
    {
        initialize("This module is the Surface module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Surface


/* Python entry */
PyMOD_INIT_FUNC(Surface)
{
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    PyObject* mod = Surface::initModule();
    Base::Console().Log("Loading Surface module... done\n");

    // Add types to module
    Surface::Filling         ::init();
    Surface::Sewing          ::init();
    Surface::Cut             ::init();
    Surface::GeomFillSurface ::init();
    Surface::Extend          ::init();
    Surface::Sections        ::init();

    PyMOD_Return(mod);
}
