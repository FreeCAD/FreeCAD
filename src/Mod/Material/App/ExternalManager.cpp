/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QMutex>
#include <QMutexLocker>
#include <Python.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <CXX/Objects.hxx>

#include "ExternalManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

ExternalManager* ExternalManager::_manager = nullptr;
QMutex ExternalManager::_mutex;

ExternalManager::ExternalManager()
{
    getConfiguration();
    instantiate();

    // _hGrp = App::GetApplication().GetParameterGroupByPath(
    //     "User parameter:BaseApp/Preferences/Mod/Material/Database");
    // _useDatabase = _hGrp->GetBool("UseDatabase", false);
    // _hGrp->Attach(this);
}

ExternalManager::~ExternalManager()
{
    // _hGrp->Detach(this);
}

void ExternalManager::getConfiguration()
{
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface/Material DB");
    _moduleName = _hGrp->GetASCII("Module", "");
    _className = _hGrp->GetASCII("Class", "");
}

void ExternalManager::instantiate()
{
    _instantiated = false;
    Base::Console().Log("Loading external manager...\n");
    // std::string module, group;
    try {
        Base::PyGILStateLocker lock;
        Py::Module mod(PyImport_ImportModule(_moduleName.c_str()), true);
        // Py::Module mod(_moduleName);
        if (mod.isNull()) {
            PyErr_SetString(PyExc_ImportError, "Cannot load module");
            Base::Console().Log(" failed\n");
            return;
        }

        Py::Callable managerClass(mod.getAttr(_className));
        // _managerObject.set(Py::Object(managerClass.apply()));
        _managerObject = managerClass.apply();
        if (_managerObject.hasAttr("APIVersion")) {
            Base::Console().Log("\tFound APIVersion()\n");
            _instantiated = true;
        }

        // if (_managerObject.hasAttr("libraries")) {
        //     Base::Console().Log("\tFound libraries()\n");
        //     Py::Callable libraries(_managerObject.getAttr("libraries"));
        //     Py::List list(libraries.apply());
        //     for (auto library : list) {
        //         auto file = Py::Object(library).as_string();
        //         Base::Console().Log("\t'%s'\n", file.c_str());
        //     }
        // }
        // else {
        //     Base::Console().Log("\tlibraries() not found\n");
        // }
        if (_instantiated) {
            Base::Console().Log("done\n");
        }
        else {
            Base::Console().Log("failed\n");
        }
    }
    catch (Py::Exception& e) {
        Base::Console().Log("failed\n");
        e.clear();
    }
}

void ExternalManager::initManager()
{
    QMutexLocker locker(&_mutex);

    if (!_manager) {
        _manager = new ExternalManager();
    }
}

ExternalManager* ExternalManager::getManager()
{
    initManager();

    return _manager;
}

std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> ExternalManager::libraries()
{
    auto libList = std::make_shared<std::vector<std::tuple<QString, QString, bool>>>();

    if (_instantiated) {
        try
        {
            Base::PyGILStateLocker lock;
            if (_managerObject.hasAttr("libraries")) {
                Base::Console().Log("\tFound libraries()\n");
                Py::Callable libraries(_managerObject.getAttr("libraries"));
                Py::List list(libraries.apply());
                for (auto library : list) {
                    auto entry = Py::Tuple(library);

                    auto pyName = entry.getItem(0);
                    auto pyIcon = entry.getItem(1);
                    auto pyReadOnly = entry.getItem(2);
                    libList->push_back(std::tuple<QString, QString, bool>(
                        QString::fromStdString(Py::Object(pyName).as_string()),
                        QString::fromStdString(Py::Object(pyIcon).as_string()),
                        Py::Object(pyReadOnly).as_bool()));
                }
            }
            else {
                Base::Console().Log("\tlibraries() not found\n");
            }
        }
        catch (Py::Exception& e) {
            e.clear();
        }
    }

    return libList;
}
