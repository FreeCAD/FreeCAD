/***************************************************************************
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
#endif

#include "UnitTestPy.h"
#include "UnitTestImp.h"

#include <Gui/Language/Translator.h>
#include <Base/Console.h>

namespace TestGui {
class Module : public Py::ExtensionModule<Module>
{

public:
    Module() : Py::ExtensionModule<Module>("QtUnitGui")
    {
        TestGui::UnitTestDialogPy::init_type();
        add_varargs_method("UnitTest",&Module::new_UnitTest,"UnitTest");
        add_varargs_method("setTest",&Module::setTest,"setTest");
        add_varargs_method("addTest",&Module::addTest,"addTest");
        initialize("This module is the QtUnitGui module"); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object new_UnitTest(const Py::Tuple& args)
    {
        if (!PyArg_ParseTuple(args.ptr(), ""))
            throw Py::Exception();
        return Py::asObject(new TestGui::UnitTestDialogPy());
    }
    Py::Object setTest(const Py::Tuple& args)
    {
        char *pstr=0;
        if (!PyArg_ParseTuple(args.ptr(), "|s", &pstr))
            throw Py::Exception();

        TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
        if (pstr)
            dlg->setUnitTest(QString::fromLatin1(pstr));
        dlg->show();
        dlg->raise();
        return Py::None();
    }
    Py::Object addTest(const Py::Tuple& args)
    {
        char *pstr=0;
        if (!PyArg_ParseTuple(args.ptr(), "|s", &pstr))
            throw Py::Exception();

        TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
        if (pstr)
            dlg->addUnitTest(QString::fromLatin1(pstr));
        dlg->show();
        dlg->raise();
        return Py::None();
    }
};

PyObject* initModule()
{
    return (new Module())->module().ptr();
}

}

void loadTestResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Test);
    Gui::Translator::instance()->refresh();
}

/* Python entry */
PyMOD_INIT_FUNC(QtUnitGui)
{
    // the following constructor call registers our extension module
    // with the Python runtime system
    PyObject* mod = TestGui::initModule();

    Base::Console().Log("Loading GUI of Test module... done\n");

    // add resources and reloads the translators
    loadTestResource();
    PyMOD_Return(mod);
}
