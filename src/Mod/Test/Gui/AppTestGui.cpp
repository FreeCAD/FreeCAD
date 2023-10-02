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

#include <Base/Console.h>
#include <Base/ConsoleObserver.h>
#include <Base/Interpreter.h>
#include <Gui/Language/Translator.h>

#include "UnitTestImp.h"
#include "UnitTestPy.h"


class ILoggerBlockerTest: public Base::ILogger
{
public:
    ~ILoggerBlockerTest() override
    {
        Base::Console().DetachObserver(this);
    }

    const char* Name() override
    {
        return "ILoggerBlockerTest";
    }

    void flush()
    {
        buffer.str("");
        buffer.clear();
    }

    void SendLog(const std::string& notifiername,
                 const std::string& msg,
                 Base::LogStyle level,
                 Base::IntendedRecipient recipient,
                 Base::ContentType content) override
    {
        (void)notifiername;
        (void)msg;
        (void)recipient;
        (void)content;

        switch (level) {
            case Base::LogStyle::Warning:
                buffer << "WRN";
                break;
            case Base::LogStyle::Message:
                buffer << "MSG";
                break;
            case Base::LogStyle::Error:
                buffer << "ERR";
                break;
            case Base::LogStyle::Log:
                buffer << "LOG";
                break;
            case Base::LogStyle::Critical:
                buffer << "CMS";
                break;
            default:
                break;
        }
    }

    void runSingleTest(const char* comment, std::string expectedResult)
    {
        Base::Console().Log(comment);
        flush();
        Base::Console().Log("LOG");
        Base::Console().Message("MSG");
        Base::Console().Warning("WRN");
        Base::Console().Error("ERR");
        Base::Console().Critical("CMS");
        if (buffer.str() != expectedResult) {
            throw Py::RuntimeError("ILoggerTest: " + buffer.str() + " different from "
                                   + expectedResult);
        }
    }

    void runTest()
    {
        runSingleTest("Print all message types", "LOGMSGWRNERRCMS");
        {
            Base::ILoggerBlocker blocker("ILoggerBlockerTest");
            runSingleTest("All types blocked", "");
        }
        runSingleTest("Print all", "LOGMSGWRNERRCMS");
        {
            Base::ILoggerBlocker blocker("ILoggerBlockerTest",
                                         Base::ConsoleSingleton::MsgType_Err
                                             | Base::ConsoleSingleton::MsgType_Wrn);
            runSingleTest("Error & Warning blocked", "LOGMSGCMS");
        }
        runSingleTest("Print all", "LOGMSGWRNERRCMS");
        {
            Base::ILoggerBlocker blocker("ILoggerBlockerTest",
                                         Base::ConsoleSingleton::MsgType_Log
                                             | Base::ConsoleSingleton::MsgType_Txt);
            runSingleTest("Log & Message blocked", "WRNERRCMS");
        }
        runSingleTest("Print all", "LOGMSGWRNERRCMS");
        {
            Base::ILoggerBlocker blocker("ILoggerBlockerTest", Base::ConsoleSingleton::MsgType_Err);
            runSingleTest("Nested : Error blocked", "LOGMSGWRNCMS");
            {
                Base::ILoggerBlocker blocker2("ILoggerBlockerTest",
                                              Base::ConsoleSingleton::MsgType_Err
                                                  | Base::ConsoleSingleton::MsgType_Wrn);
                runSingleTest(
                    "Nested : Warning blocked + Error (from nesting) + Error (redundancy)",
                    "LOGMSGCMS");
            }
            runSingleTest("Nested : Error still blocked", "LOGMSGWRNCMS");
        }
        runSingleTest("Print all", "LOGMSGWRNERRCMS");
        {
            Base::ILoggerBlocker blocker("ILoggerBlockerTest");
            Base::Console().SetEnabledMsgType("ILoggerBlockerTest",
                                              Base::ConsoleSingleton::MsgType_Log,
                                              true);
            runSingleTest("Log is enabled but a warning is triggered in debug mode", "LOG");
        }
        runSingleTest("Print all", "LOGMSGWRNERRCMS");
    }

private:
    std::ostringstream buffer;
};


namespace TestGui
{
class Module: public Py::ExtensionModule<Module>
{

public:
    Module()
        : Py::ExtensionModule<Module>("QtUnitGui")
    {
        TestGui::UnitTestDialogPy::init_type();
        add_varargs_method("UnitTest", &Module::new_UnitTest, "UnitTest");
        add_varargs_method("setTest", &Module::setTest, "setTest");
        add_varargs_method("addTest", &Module::addTest, "addTest");
        add_varargs_method("runTest", &Module::runTest, "runTest");
        add_varargs_method("testILoggerBlocker", &Module::testILoggerBlocker, "testILoggerBlocker");
        initialize("This module is the QtUnitGui module");  // register with Python
    }

private:
    Py::Object new_UnitTest(const Py::Tuple& args)
    {
        if (!PyArg_ParseTuple(args.ptr(), "")) {
            throw Py::Exception();
        }
        return Py::asObject(new TestGui::UnitTestDialogPy());
    }
    Py::Object setTest(const Py::Tuple& args)
    {
        char* pstr = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "|s", &pstr)) {
            throw Py::Exception();
        }

        TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
        if (pstr) {
            dlg->setUnitTest(QString::fromLatin1(pstr));
        }
        dlg->show();
        dlg->raise();
        return Py::None();
    }
    Py::Object addTest(const Py::Tuple& args)
    {
        char* pstr = nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "|s", &pstr)) {
            throw Py::Exception();
        }

        TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
        if (pstr) {
            dlg->addUnitTest(QString::fromLatin1(pstr));
        }
        dlg->show();
        dlg->raise();
        return Py::None();
    }
    Py::Object runTest(const Py::Tuple& args)
    {
        if (!PyArg_ParseTuple(args.ptr(), "")) {
            throw Py::Exception();
        }

        TestGui::UnitTestDialog* dlg = TestGui::UnitTestDialog::instance();
        bool success = dlg->runCurrentTest();
        return Py::Boolean(success);
    }
    Py::Object testILoggerBlocker(const Py::Tuple& args)
    {
        (void)args;
        ILoggerBlockerTest iltest;
        Base::Console().AttachObserver(static_cast<Base::ILogger*>(&iltest));
        Base::Console().SetConnectionMode(Base::ConsoleSingleton::Direct);
        iltest.runTest();
        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace TestGui

void loadTestResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Test);
    Q_INIT_RESOURCE(Test_translation);
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
