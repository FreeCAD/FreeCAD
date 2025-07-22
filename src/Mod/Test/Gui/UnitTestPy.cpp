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
#include <QApplication>
#include <QEventLoop>
#endif

#include <Base/Exception.h>

#include "UnitTestImp.h"
#include "UnitTestPy.h"


using namespace TestGui;


void UnitTestDialogPy::init_type()
{
    behaviors().name("TestGui.UnitTest");
    behaviors().doc("About TestGui.UnitTest");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("clearErrorList", &UnitTestDialogPy::clearErrorList, "clearErrorList");
    add_varargs_method("insertError", &UnitTestDialogPy::insertError, "insertError");
    add_varargs_method("setUnitTest", &UnitTestDialogPy::setUnitTest, "setUnitTest");
    add_varargs_method("getUnitTest", &UnitTestDialogPy::getUnitTest, "getUnitTest");
    add_varargs_method("setStatusText", &UnitTestDialogPy::setStatusText, "setStatusText");
    add_varargs_method("setProgressFraction",
                       &UnitTestDialogPy::setProgressFrac,
                       "setProgressFraction");
    add_varargs_method("errorDialog", &UnitTestDialogPy::errorDialog, "errorDialog");
    add_varargs_method("setRunCount", &UnitTestDialogPy::setRunCount, "setRunCount");
    add_varargs_method("setFailCount", &UnitTestDialogPy::setFailCount, "setFailCount");
    add_varargs_method("setErrorCount", &UnitTestDialogPy::setErrorCount, "setErrorCount");
    add_varargs_method("setRemainCount", &UnitTestDialogPy::setRemainCount, "setRemainCount");
    add_varargs_method("updateGUI", &UnitTestDialogPy::updateGUI, "updateGUI");
    add_varargs_method("addUnitTest", &UnitTestDialogPy::addUnitTest, "addUnitTest");
    add_varargs_method("clearUnitTests", &UnitTestDialogPy::clearUnitTests, "clearUnitTests");
}

UnitTestDialogPy::UnitTestDialogPy() = default;

UnitTestDialogPy::~UnitTestDialogPy() = default;

Py::Object UnitTestDialogPy::repr()
{
    return Py::String("UnitTest");
}

Py::Object UnitTestDialogPy::getattr(const char* attr)
{
    return Py::PythonExtension<UnitTestDialogPy>::getattr(attr);
}

int UnitTestDialogPy::setattr(const char* attr, const Py::Object& value)
{
    return Py::PythonExtension<UnitTestDialogPy>::setattr(attr, value);
}

Py::Object UnitTestDialogPy::clearErrorList(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->clearErrorList();
    return Py::None();
}

Py::Object UnitTestDialogPy::insertError(const Py::Tuple& args)
{
    char* failure = nullptr;
    char* details = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &failure, &details)) {
        throw Py::Exception();
    }

    UnitTestDialog::instance()->insertError(QString::fromLatin1(failure),
                                            QString::fromLatin1(details));
    return Py::None();
}

Py::Object UnitTestDialogPy::setUnitTest(const Py::Tuple& args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr)) {
        throw Py::Exception();
    }

    UnitTestDialog::instance()->setUnitTest(QString::fromLatin1(pstr));
    return Py::None();
}

Py::Object UnitTestDialogPy::getUnitTest(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    return Py::String((const char*)UnitTestDialog::instance()->getUnitTest().toLatin1());
}

Py::Object UnitTestDialogPy::setStatusText(const Py::Tuple& args)
{
    char* pstr = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr)) {
        throw Py::Exception();
    }

    UnitTestDialog::instance()->setStatusText(QString::fromLatin1(pstr));
    return Py::None();
}

Py::Object UnitTestDialogPy::setProgressFrac(const Py::Tuple& args)
{
    float fraction;
    char* pColor = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "f|s", &fraction, &pColor)) {
        throw Py::Exception();
    }

    if (pColor) {
        UnitTestDialog::instance()->setProgressFraction(fraction, QString::fromLatin1(pColor));
    }
    else {
        UnitTestDialog::instance()->setProgressFraction(fraction);
    }
    return Py::None();
}

Py::Object UnitTestDialogPy::errorDialog(const Py::Tuple& args)
{
    char* title = nullptr;
    char* message = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &title, &message)) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->showErrorDialog(title, message);
    return Py::None();
}

Py::Object UnitTestDialogPy::setRunCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count)) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->setRunCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setFailCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count)) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->setFailCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setErrorCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count)) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->setErrorCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::setRemainCount(const Py::Tuple& args)
{
    int count;
    if (!PyArg_ParseTuple(args.ptr(), "i", &count)) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->setRemainCount(count);
    return Py::None();
}

Py::Object UnitTestDialogPy::updateGUI(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    return Py::None();
}

Py::Object UnitTestDialogPy::addUnitTest(const Py::Tuple& args)
{
    char* pstr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &pstr)) {
        throw Py::Exception();
    }

    TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
    dlg->addUnitTest(QString::fromLatin1(pstr));
    return Py::None();
}

Py::Object UnitTestDialogPy::clearUnitTests(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }
    UnitTestDialog::instance()->clearUnitTests();
    return Py::None();
}
