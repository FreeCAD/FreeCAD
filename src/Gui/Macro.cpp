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
# include <cassert>
# include <QFile>
# include <QTextStream>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "Macro.h"
#include "MainWindow.h"
#include "PythonConsole.h"
#include "PythonConsolePy.h"
#include "PythonDebugger.h"


using namespace Gui;

MacroFile::MacroFile() = default;

void MacroFile::open(const char *sName)
{
    // check
#if _DEBUG
    Q_ASSERT(!this->openMacro);
#endif

    // Convert from Utf-8
    this->macroName = QString::fromUtf8(sName);
    if (!this->macroName.endsWith(QLatin1String(".FCMacro")))
        this->macroName += QLatin1String(".FCMacro");

    this->macroInProgress.clear();
    this->openMacro = true;
}

void MacroFile::append(const QString& line)
{
    this->macroInProgress.append(line);
}

void MacroFile::append(const QStringList& lines)
{
    this->macroInProgress.append(lines);
}

bool MacroFile::commit()
{
    QFile file(this->macroName);
    if (!file.open(QFile::WriteOnly)) {
        return false;
    }

    // sort import lines and avoid duplicates
    QTextStream str(&file);
    QStringList import;
    import << QString::fromLatin1("import FreeCAD");
    QStringList body;

    for (const auto& it : qAsConst(this->macroInProgress)) {
        if (it.startsWith(QLatin1String("import ")) ||
            it.startsWith(QLatin1String("#import "))) {
            if (import.indexOf(it) == -1)
                import.push_back(it);
        }
        else {
            body.push_back(it);
        }
    }

    QString header;
    header += QString::fromLatin1("# -*- coding: utf-8 -*-\n\n");
    header += QString::fromLatin1("# Macro Begin: ");
    header += this->macroName;
    header += QString::fromLatin1(" +++++++++++++++++++++++++++++++++++++++++++++++++\n");

    QString footer = QString::fromLatin1("# Macro End: ");
    footer += this->macroName;
    footer += QString::fromLatin1(" +++++++++++++++++++++++++++++++++++++++++++++++++\n");

    // write the data to the text file
    str << header;
    for (const auto& it : qAsConst(import)) {
        str << it << QLatin1Char('\n');
    }
    str << QLatin1Char('\n');
    for (const auto& it : body) {
        str << it << QLatin1Char('\n');
    }
    str << footer;

    this->macroInProgress.clear();
    this->macroName.clear();
    this->openMacro = false;
    file.close();
    return true;
}

void MacroFile::cancel()
{
    this->macroInProgress.clear();
    this->macroName.clear();
    this->openMacro = false;
}

// ----------------------------------------------------------------------------

MacroOutputBuffer::MacroOutputBuffer() = default;

void MacroOutputBuffer::addPendingLine(int type, const char* line)
{
    if (!line) {
        pendingLine.clear();
    }
    else {
        pendingLine.emplace_back(type, line);
    }
}

bool MacroOutputBuffer::addPendingLineIfComment(int type, const char* line)
{
    if (MacroOutputOption::isComment(type)) {
        pendingLine.emplace_back(type, line);
        return true;
    }

    return false;
}

void MacroOutputBuffer::incrementIfNoComment(int type)
{
    if (!MacroOutputOption::isComment(type)) {
        ++totalLines;
    }
}

// ----------------------------------------------------------------------------

MacroOutputOption::MacroOutputOption() = default;

std::tuple<bool, bool> MacroOutputOption::values(int type) const
{
    bool comment = isComment(type);
    bool record = true;

    if (isGuiCommand(type)) {
        if (recordGui && guiAsComment) {
            comment = true;
        }
        else if (!recordGui) {
            comment = true;
            record = false;
        }
    }

    return std::make_tuple(comment, record);
}

bool MacroOutputOption::isComment(int type)
{
    return type == MacroManager::Cmt;
}

bool MacroOutputOption::isGuiCommand(int type)
{
    return type == MacroManager::Gui;
}

bool MacroOutputOption::isAppCommand(int type)
{
    return type == MacroManager::App;
}

// ----------------------------------------------------------------------------

MacroManager::MacroManager()
  : pyDebugger(new PythonDebugger())
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
    (void)rCaller;
    (void)sReason;
    option.recordGui         = this->params->GetBool("RecordGui", true);
    option.guiAsComment      = this->params->GetBool("GuiAsComment", true);
    option.scriptToPyConsole = this->params->GetBool("ScriptToPyConsole", true);
    this->localEnv           = this->params->GetBool("LocalEnvironment", true);
}

void MacroManager::open(MacroType eType, const char *sName)
{
    // check
#if _DEBUG
    assert(eType == File);
#else
    Q_UNUSED(eType);
#endif

    macroFile.open(sName);
    Base::Console().Log("CmdM: Open macro: %s\n", sName);
}

void MacroManager::commit()
{
    QString macroName = macroFile.fileName();
    if (macroFile.commit()) {
        Base::Console().Log("Commit macro: %s\n", (const char*)macroName.toUtf8());
    }
    else {
        Base::Console().Error("Cannot open file to write macro: %s\n",
            (const char*)macroName.toUtf8());
        cancel();
    }
}

void MacroManager::cancel()
{
    QString macroName = macroFile.fileName();
    Base::Console().Log("Cancel macro: %s\n",(const char*)macroName.toUtf8());
    macroFile.cancel();
}

void MacroManager::addPendingLine(LineType type, const char* line)
{
    buffer.addPendingLine(type, line);
}

void MacroManager::addLine(LineType Type, const char* sLine)
{
    if (!sLine)
        return;

    if (buffer.hasPendingLines()) {
        if (buffer.addPendingLineIfComment(Type, sLine)) {
            return;
        }

        processPendingLines();
    }

    buffer.incrementIfNoComment(Type);

    addToOutput(Type, sLine);
}

void MacroManager::processPendingLines()
{
    decltype(buffer.pendingLine) lines;
    lines.swap(buffer.pendingLine);
    for (auto &v : lines) {
        addLine(static_cast<LineType>(v.first), v.second.c_str());
    }
}

void MacroManager::makeComment(QStringList& lines) const
{
    for (auto &line : lines) {
        if (!line.startsWith(QLatin1String("#"))) {
            line.prepend(QLatin1String("# "));
        }
    }
}

void MacroManager::addToOutput(LineType type, const char* line)
{
    auto [comment, record] = option.values(type);

    QStringList lines = QString::fromUtf8(line).split(QLatin1String("\n"));
    if (comment) {
        makeComment(lines);
    }

    if (record && macroFile.isOpen()) {
        macroFile.append(lines);
    }

    if (option.scriptToPyConsole) {
        // search for the Python console
        auto console = getPythonConsole();
        if (console) {
            for(auto &line : lines) {
                console->printStatement(line);
            }
        }
    }
}

void MacroManager::setModule(const char* sModule)
{
    if (macroFile.isOpen() && sModule && *sModule != '\0')  {
        macroFile.append(QString::fromLatin1("import %1").arg(QString::fromLatin1(sModule)));
    }
}

PythonConsole* MacroManager::getPythonConsole() const
{
    // search for the Python console
    if (!this->pyConsole) {
        this->pyConsole = Gui::getMainWindow()->findChild<Gui::PythonConsole*>();
    }

    return this->pyConsole;
}

namespace Gui {
    class PythonRedirector
    {
    public:
        PythonRedirector(const char* type, PyObject* obj) : std_out(type), out(obj)
        {
            if (out) {
                Base::PyGILStateLocker lock;
                old = PySys_GetObject(std_out);
                PySys_SetObject(std_out, out);
            }
        }
        ~PythonRedirector()
        {
            if (out) {
                Base::PyGILStateLocker lock;
                PySys_SetObject(std_out, old);
                Py_DECREF(out);
            }
        }
    private:
        const char* std_out;
        PyObject* out;
        PyObject* old{nullptr};
    };
}

void MacroManager::run(MacroType eType, const char *sName)
{
    Q_UNUSED(eType);

    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("OutputWindow");
        PyObject* pyout = hGrp->GetBool("RedirectPythonOutput",true) ? new OutputStdout : nullptr;
        PyObject* pyerr = hGrp->GetBool("RedirectPythonErrors",true) ? new OutputStderr : nullptr;
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
