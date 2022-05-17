/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
#endif

#include "Command.h"
#include "Action.h"
#include "Application.h"
#include "MainWindow.h"
#include "Selection.h"
#include "Window.h"
#include "PythonWrapper.h"

// inclusion of the generated files (generated out of CommandPy.xml)
#include "CommandPy.h"
#include "CommandPy.cpp"


// returns a string which represents the object e.g. when printed in python
std::string CommandPy::representation(void) const
{
    return std::string("<Command object>");
}

PyObject* CommandPy::get(PyObject *args)
{
    char* pName;
    if (!PyArg_ParseTuple(args, "s", &pName))
        return nullptr;

    Command* cmd = Application::Instance->commandManager().getCommandByName(pName);
    if (cmd) {
        CommandPy* cmdPy = new CommandPy(cmd);
        return cmdPy;
    }

    Py_Return;
}

PyObject* CommandPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getMainWindow()->updateActions();
    Py_Return;
}

PyObject* CommandPy::listAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    std::vector <Command*> cmds = Application::Instance->commandManager().getAllCommands();
    PyObject* pyList = PyList_New(cmds.size());
    int i=0;
    for ( std::vector<Command*>::iterator it = cmds.begin(); it != cmds.end(); ++it ) {
        PyObject* str = PyUnicode_FromString((*it)->getName());
        PyList_SetItem(pyList, i++, str);
    }
    return pyList;
}

PyObject* CommandPy::listByShortcut(PyObject *args)
{
    char* shortcut_to_find;
    PyObject* bIsRegularExp = Py_False;
    if (!PyArg_ParseTuple(args, "s|O!", &shortcut_to_find, &PyBool_Type, &bIsRegularExp))
        return nullptr;

    std::vector <Command*> cmds = Application::Instance->commandManager().getAllCommands();
    std::vector <std::string> matches;
    for (Command* c : cmds) {
        Action* action = c->getAction();
        if (action) {
            QString spc = QString::fromLatin1(" ");
            if (PyObject_IsTrue(bIsRegularExp) ? true : false) {
               QRegExp re = QRegExp(QString::fromLatin1(shortcut_to_find));
               re.setCaseSensitivity(Qt::CaseInsensitive);
               if (!re.isValid()) {
                   std::stringstream str;
                   str << "Invalid regular expression:" << ' ' << shortcut_to_find;
                   throw Py::RuntimeError(str.str());
               }

               if (re.indexIn(action->shortcut().toString().remove(spc).toUpper()) != -1) {
                   matches.push_back(c->getName());
               }
            }
            else if (action->shortcut().toString().remove(spc).toUpper() ==
                     QString::fromLatin1(shortcut_to_find).remove(spc).toUpper()) {
                matches.push_back(c->getName());
            }
        }
    }

    PyObject* pyList = PyList_New(matches.size());
    int i=0;
    for (std::string match : matches) {
        PyObject* str = PyUnicode_FromString(match.c_str());
        PyList_SetItem(pyList, i++, str);
    }
    return pyList;
}

PyObject* CommandPy::run(PyObject *args)
{
    int item = 0;
    if (!PyArg_ParseTuple(args, "|i", &item))
        return nullptr;

    Gui::Command::LogDisabler d1;
    Gui::SelectionLogDisabler d2;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        cmd->invoke(item);
        Py_Return;
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::isActive(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        PY_TRY {
            return Py::new_reference_to(Py::Boolean(cmd->isActive()));
        }
        PY_CATCH;
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::getShortcut(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        PyObject* str = PyUnicode_FromString(cmd->getAction() ? cmd->getAction()->shortcut().toString().toStdString().c_str() : "");
        return str;
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::setShortcut(PyObject *args)
{
    char* pShortcut;
    if (!PyArg_ParseTuple(args, "s", &pShortcut))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        Action* action = cmd->getAction();
        if (action) {
            QKeySequence shortcut = QString::fromLatin1(pShortcut);
            QString nativeText = shortcut.toString(QKeySequence::NativeText);
            action->setShortcut(nativeText);
            bool success = action->shortcut() == nativeText;
            /**
             * avoid cluttering parameters unnecessarily by saving only
             * when new shortcut is not the default shortcut
             * remove spaces to handle cases such as shortcut = "C,L" or "C, L"
             */
            QString default_shortcut = QString::fromLatin1(cmd->getAccel());
            QString spc = QString::fromLatin1(" ");

            ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
            const char* pName = cmd->getName();
            if (success && default_shortcut.remove(spc).toUpper() != nativeText.remove(spc).toUpper()) {
                hGrp->SetASCII(pName, pShortcut);
            }
            else {
                hGrp->RemoveASCII(pName);
            }
            return Py::new_reference_to(Py::Boolean(success));
        }
        else {
            return Py::new_reference_to(Py::Boolean(false));
        }
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::resetShortcut(PyObject *args)
{

    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        Action* action = cmd->getAction();
        if (action){
            QString default_shortcut = QString::fromLatin1(cmd->getAccel());
            action->setShortcut(default_shortcut);
            ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
            hGrp->RemoveASCII(cmd->getName());
            /** test to see if we successfully reset the shortcut by loading it back and comparing */
            QString spc = QString::fromLatin1(" ");
            QString new_shortcut = action->shortcut().toString();
            if (default_shortcut.remove(spc).toUpper() == new_shortcut.remove(spc).toUpper()){
                return Py::new_reference_to(Py::Boolean(true));
            } else {
                return Py::new_reference_to(Py::Boolean(false));
            }
        } else {
            return Py::new_reference_to(Py::Boolean(false));
        }

    } else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::getInfo(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        Action* action = cmd->getAction();
        PyObject* pyDict = PyDict_New();
        const char* cmdName = cmd->getName();
        const char* menuTxt = cmd->getMenuText();
        const char* tooltipTxt = cmd->getToolTipText();
        const char* whatsThisTxt = cmd->getWhatsThis();
        const char* statustipTxt = cmd->getStatusTip();
        const char* pixMapTxt = cmd->getPixmap();
        std::string shortcutTxt = "";
        if (action)
            shortcutTxt = action->shortcut().toString().toStdString();

        PyObject* strCmdName = PyUnicode_FromString(cmdName);
        PyObject* strMenuTxt = PyUnicode_FromString(menuTxt ? menuTxt : "");
        PyObject* strTooltipTxt = PyUnicode_FromString(tooltipTxt ? tooltipTxt : "");
        PyObject* strWhatsThisTxt = PyUnicode_FromString(whatsThisTxt ? whatsThisTxt : "");
        PyObject* strStatustipTxt = PyUnicode_FromString(statustipTxt ? statustipTxt : "");
        PyObject* strPixMapTxt = PyUnicode_FromString(pixMapTxt ? pixMapTxt : "");
        PyObject* strShortcutTxt = PyUnicode_FromString(!shortcutTxt.empty() ? shortcutTxt.c_str() : "");
        PyDict_SetItemString(pyDict, "name", strCmdName);
        PyDict_SetItemString(pyDict, "menuText", strMenuTxt);
        PyDict_SetItemString(pyDict, "toolTip", strTooltipTxt);
        PyDict_SetItemString(pyDict, "whatsThis", strWhatsThisTxt);
        PyDict_SetItemString(pyDict, "statusTip", strStatustipTxt);
        PyDict_SetItemString(pyDict, "pixmap", strPixMapTxt);
        PyDict_SetItemString(pyDict, "shortcut", strShortcutTxt);
        return pyDict;
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}

PyObject* CommandPy::getAction(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Command* cmd = this->getCommandPtr();
    if (cmd) {
        Action* action = cmd->getAction();
        ActionGroup* group = qobject_cast<ActionGroup*>(action);

        PythonWrapper wrap;
        wrap.loadWidgetsModule();

        Py::List list;
        if (group) {
            for (auto a : group->actions())
                list.append(wrap.fromQObject(a));
        }
        else if (action) {
            list.append(wrap.fromQObject(action->action()));
        }

        return Py::new_reference_to(list);
    }
    else {
        PyErr_Format(Base::PyExc_FC_GeneralError, "No such command");
        return nullptr;
    }
}


PyObject* CommandPy::createCustomCommand(PyObject* args, PyObject* kw)
{
    const char* macroFile;
    const char* menuTxt = nullptr;
    const char* tooltipTxt = nullptr;
    const char* whatsthisTxt = nullptr;
    const char* statustipTxt = nullptr;
    const char* pixmapTxt = nullptr;
    const char* shortcutTxt = nullptr;
    static char* kwlist[] = {"macroFile", "menuText", "toolTip", "whatsThis","statusTip", "pixmap", "shortcut", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kw, "s|zzzzzz", kwlist, &macroFile, &menuTxt,
            &tooltipTxt, &whatsthisTxt, &statustipTxt, &pixmapTxt, &shortcutTxt))
        return nullptr;

    auto name = Application::Instance->commandManager().newMacroName();
    CommandManager& commandManager = Application::Instance->commandManager();
    MacroCommand* macro = new MacroCommand(name.c_str(), false);
    commandManager.addCommand(macro);

    macro->setScriptName(macroFile); 

    if (menuTxt)
        macro->setMenuText(menuTxt);

    if (tooltipTxt)
        macro->setToolTipText(tooltipTxt);

    if (whatsthisTxt)
        macro->setWhatsThis(whatsthisTxt);

    if (statustipTxt)
        macro->setStatusTip(statustipTxt);

    if (pixmapTxt)
        macro->setPixmap(pixmapTxt);

    if (shortcutTxt)
        macro->setAccel(shortcutTxt);

    return PyUnicode_FromString(name.c_str());
}

PyObject* CommandPy::removeCustomCommand(PyObject* args)
{
    const char* actionName = nullptr;
    if (!PyArg_ParseTuple(args, "s", &actionName))
        return nullptr;

    CommandManager& commandManager = Application::Instance->commandManager();
    std::vector<Command*> macros = commandManager.getGroupCommands("Macros");

    auto action = std::find_if(macros.begin(), macros.end(), [actionName](const Command* c) { 
        return std::string(c->getName()) == std::string(actionName); 
    });

    if (action != macros.end()) {
        commandManager.removeCommand(*action);
        return Py::new_reference_to(Py::Boolean(true));
    }
    else {
        return Py::new_reference_to(Py::Boolean(false));
    }
}

PyObject* CommandPy::findCustomCommand(PyObject* args)
{
    const char* macroScriptName = nullptr;
    if (!PyArg_ParseTuple(args, "s", &macroScriptName))
        return nullptr;

    CommandManager& commandManager = Application::Instance->commandManager();
    std::vector<Command*> macros = commandManager.getGroupCommands("Macros");

    auto action = std::find_if(macros.begin(), macros.end(), [macroScriptName](const Command* c) {
        if (auto mc = dynamic_cast<const MacroCommand*>(c))
            if (std::string(mc->getScriptName()) == std::string(macroScriptName))
                return true;
        return false;
        });

    if (action != macros.end())
        return PyUnicode_FromString((*action)->getName());
    else
        Py_Return;
}

PyObject *CommandPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CommandPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

