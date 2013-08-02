/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <assert.h>
# include <stdio.h>
# include <QApplication>
# include <QFile>
# include <QTextStream>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Macro.h"

#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>

#include "MainWindow.h"
#include "PythonConsole.h"
#include "PythonConsolePy.h"
#include "PythonDebugger.h"

using namespace Gui;


MacroManager::MacroManager()
  : openMacro(false),
    recordGui(true),
    guiAsComment(true),
    scriptToPyConsole(true),
    pyConsole(0),
    pyDebugger(new PythonDebugger())
{
    // Attach to the Parametergroup regarding macros
    this->params = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro");
    this->params->Attach(this);
    this->params->NotifyAll();
}

MacroManager::~MacroManager()
{
    delete pyDebugger;
    this->params->Detach(this);
}

void MacroManager::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    this->recordGui         = this->params->GetBool("RecordGui", true);
    this->guiAsComment      = this->params->GetBool("GuiAsComment", true);
    this->scriptToPyConsole = this->params->GetBool("ScriptToPyConsole", true);
    this->localEnv          = this->params->GetBool("LocalEnvironment", true);
}

void MacroManager::open(MacroType eType,const char *sName)
{
    // check 
    assert(!this->openMacro);
    assert(eType == File);

    // Convert from Utf-8
    this->macroName = QString::fromUtf8(sName);
    if (!this->macroName.endsWith(QLatin1String(".FCMacro")))
        this->macroName += QLatin1String(".FCMacro");

    this->macroInProgress.clear();
    this->openMacro = true;

    Base::Console().Log("CmdM: Open macro: %s\n", sName);
}

void MacroManager::commit(void)
{
    QFile file(this->macroName);
    if (file.open(QFile::WriteOnly)) 
    {
        // sort import lines and avoid duplicates
        QTextStream str(&file);
        QStringList import;
        import << QString::fromAscii("import FreeCAD");
        QStringList body;

        QStringList::Iterator it;
        for (it = this->macroInProgress.begin(); it != this->macroInProgress.end(); ++it )
        {
            if ((*it).startsWith(QLatin1String("import ")) ||
                (*it).startsWith(QLatin1String("#import ")))
            {
                if (import.indexOf(*it) == -1)
                    import.push_back(*it);
            }
            else
            {
                body.push_back(*it);
            }
        }

        QString header = QString::fromAscii("# Macro Begin: ");
        header += this->macroName;
        header += QString::fromAscii(" +++++++++++++++++++++++++++++++++++++++++++++++++\n");

        QString footer = QString::fromAscii("# Macro End: ");
        footer += this->macroName;
        footer += QString::fromAscii(" +++++++++++++++++++++++++++++++++++++++++++++++++\n");

        // write the data to the text file
        str << header;
        for (it = import.begin(); it != import.end(); ++it)
            str << (*it) << QLatin1Char('\n');
        str << QLatin1Char('\n');
        for (it = body.begin(); it != body.end(); ++it)
            str << (*it) << QLatin1Char('\n');
        str << footer;

        Base::Console().Log("Commit macro: %s\n",(const char*)this->macroName.toUtf8());

        this->macroInProgress.clear();
        this->macroName.clear();
        this->openMacro = false;
    }
    else {
        Base::Console().Error("Cannot open file to write macro: %s\n",
            (const char*)this->macroName.toUtf8());
        cancel();
    }
}

void MacroManager::cancel(void)
{
    Base::Console().Log("Cancel macro: %s\n",(const char*)this->macroName.toUtf8());

    this->macroInProgress.clear();
    this->macroName.clear();
    this->openMacro = false;
}

void MacroManager::addLine(LineType Type, const char* sLine)
{
    if (this->openMacro) {
        bool comment = false;
        if (Type == Gui) {
            if (this->recordGui && this->guiAsComment)
                comment = true;
            else if (!this->recordGui)
                return; // ignore Gui commands
        }
        else if (Type == Cmt) {
            comment = true;
        }

        QStringList lines = QString::fromAscii(sLine).split(QLatin1String("\n"));
        if (comment) {
            for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it)
                it->prepend(QLatin1String("#"));
        }
        this->macroInProgress.append(lines);
    }

    if (this->scriptToPyConsole) {
        // search for the Python console
        if (!this->pyConsole)
            this->pyConsole = Gui::getMainWindow()->findChild<Gui::PythonConsole*>();
        // Python console found?
        if (this->pyConsole)
            this->pyConsole->printStatement(QString::fromUtf8(sLine));
    }
}

void MacroManager::setModule(const char* sModule)
{
    if (this->openMacro && sModule && *sModule != '\0')
    {
        this->macroInProgress.append(QString::fromAscii("import %1").arg(QString::fromAscii(sModule)));
    }
}

namespace Gui {
    class PythonRedirector
    {
    public:
        PythonRedirector(const char* type, PyObject* obj) : std_out(type), out(obj)
        {
            if (out) {
                Base::PyGILStateLocker lock;
                old = PySys_GetObject(const_cast<char*>(std_out));
                PySys_SetObject(const_cast<char*>(std_out), out);
            }
        }
        ~PythonRedirector()
        {
            if (out) {
                Base::PyGILStateLocker lock;
                PySys_SetObject(const_cast<char*>(std_out), old);
                Py_DECREF(out);
            }
        }
    private:
        const char* std_out;
        PyObject* out;
        PyObject* old;
    };
}

void MacroManager::run(MacroType eType,const char *sName)
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("OutputWindow");
        PyObject* pyout = hGrp->GetBool("RedirectPythonOutput") ? new OutputStdout : 0;
        PyObject* pyerr = hGrp->GetBool("RedirectPythonErrors") ? new OutputStderr : 0;
        PythonRedirector std_out("stdout",pyout);
        PythonRedirector std_err("stderr",pyerr);
        //The given path name is expected to be Utf-8
        Base::Interpreter().runFile(sName, this->localEnv);
    }
    catch (const Base::SystemExitException&) {
        throw;
    }
    catch (const Base::PyException& e) {
        e.ReportException();
    }
    catch (const Base::Exception& e) {
        qWarning("%s",e.what());
    }
}

PythonDebugger* MacroManager::debugger() const
{
    return pyDebugger;
}
