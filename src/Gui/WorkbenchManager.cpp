/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "WorkbenchManager.h"
#include "Workbench.h"
#include "DockWindowManager.h"
#include "MenuManager.h"
#include "ToolBarManager.h"

using namespace Gui;

WorkbenchManager* WorkbenchManager::_instance = nullptr;

WorkbenchManager* WorkbenchManager::instance()
{
    if (!_instance)
        _instance = new WorkbenchManager;
    return _instance;
}

void WorkbenchManager::destruct()
{
    delete _instance;
    _instance = nullptr;
}

WorkbenchManager::WorkbenchManager() = default;

WorkbenchManager::~WorkbenchManager()
{
    for (auto & it : _workbenches) {
        Workbench* wb = it.second;
        delete wb;
    }

    MenuManager::destruct();
    ToolBarManager::destruct();
    //ToolBoxManager::destruct();
    DockWindowManager::destruct();
}

Workbench* WorkbenchManager::createWorkbench (const std::string& name, const std::string& className)
{
    Workbench* wb = getWorkbench(name);

    if (!wb) {
        // try to create an instance now
        Base::Type type = Base::Type::getTypeIfDerivedFrom(className.c_str(), Workbench::getClassTypeId(), false);
        wb = static_cast<Workbench*>(type.createInstance());
        // createInstance could return a null pointer
        if (!wb) {
            std::stringstream str;
            str << "'" << className << "' not a workbench type" << std::ends;
            throw Base::TypeError(str.str());
        }

        wb->setName(name);
        _workbenches[name] = wb;
    }

    return wb;
}

void WorkbenchManager::removeWorkbench(const std::string& name)
{
    std::map<std::string, Workbench*>::iterator it = _workbenches.find(name);
    if (it != _workbenches.end()) {
        Workbench* wb = it->second;
        _workbenches.erase(it);
        if (_activeWorkbench == wb)
            _activeWorkbench = nullptr;
        delete wb;
    }
}

Workbench* WorkbenchManager::getWorkbench (const std::string& name) const
{
    Workbench* wb=nullptr;

    std::map<std::string, Workbench*>::const_iterator it = _workbenches.find(name);
    if (it != _workbenches.end()) {
        // returns the already created object
        wb = it->second;
    }

    return wb;
}

bool WorkbenchManager::activate(const std::string& name, const std::string& className)
{
    Workbench* wb = createWorkbench(name, className);
    if (wb) {
        _activeWorkbench = wb;
        wb->activate();
        return true;
    }

    return false;
}

Workbench* WorkbenchManager::active() const
{
    return _activeWorkbench;
}

std::list<std::string> WorkbenchManager::workbenches() const
{
    std::list<std::string> wb;
    for (const auto & it : _workbenches)
        wb.push_back(it.first);
    return wb;
}
