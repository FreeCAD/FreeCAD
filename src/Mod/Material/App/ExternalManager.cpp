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

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialPy.h"
#include "ModelPy.h"


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

void ExternalManager::connect()
{
    if (!_instantiated) {
        instantiate();

        if (!_instantiated) {
            throw ConnectionError();
        }
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

//=====
//
// Library management
//
//=====

std::shared_ptr<std::vector<std::tuple<QString, QString, bool>>> ExternalManager::libraries()
{
    auto libList = std::make_shared<std::vector<std::tuple<QString, QString, bool>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("libraries")) {
            Base::Console().Log("\tFound libraries()\n");
            Py::Callable libraries(_managerObject.getAttr("libraries"));
            Py::List list(libraries.apply());
            for (auto library : list) {
                auto entry = Py::Tuple(library);

                auto pyName = entry.getItem(0);
                QString libraryName;
                if (!pyName.isNone()) {
                    libraryName = QString::fromStdString(pyName.as_string());
                }
                auto pyIcon = entry.getItem(1);
                QString icon;
                if (!pyIcon.isNone()) {
                    icon = QString::fromStdString(pyIcon.as_string());
                }
                auto pyReadOnly = entry.getItem(2);
                bool readOnly = pyReadOnly.as_bool();

                libList->push_back(
                    std::tuple<QString, QString, bool>(libraryName, icon, readOnly));
            }
        }
        else {
            Base::Console().Log("\tlibraries() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return libList;
}

void ExternalManager::createLibrary(const QString& libraryName,
                                    const QString& icon,
                                    bool readOnly)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("createLibrary")) {
            Base::Console().Log("\tcreateLibrary()\n");
            Py::Callable libraries(_managerObject.getAttr("createLibrary"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(icon.toStdString()));
            args.setItem(2, Py::Boolean(readOnly));
            libraries.apply(args); // No return expected
        }
        else {
            Base::Console().Log("\tcreateLibrary() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

//=====
//
// Model management
//
//=====

std::shared_ptr<Model> ExternalManager::getModel(const QString& uuid)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("getModel")) {
            Base::Console().Log("\tgetModel()\n");
            Py::Callable libraries(_managerObject.getAttr("getModel"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(uuid.toStdString()));
            Py::Tuple result(libraries.apply(args));  // ignore return for now

            Py::Object uuidObject = result.getItem(0);
            Py::Object libraryObject = result.getItem(1);
            Py::Object modelObject = result.getItem(2);

            Model* model = static_cast<ModelPy*>(*modelObject)->getModelPtr();
            model->setUUID(uuid);
            auto shared = std::make_shared<Model>(*model);

            return shared;
        }
        else {
            Base::Console().Log("\tgetModel() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw ModelNotFound(e1.what());
    }
}

void ExternalManager::addModel(const QString& libraryName,
                               const QString& path,
                               const std::shared_ptr<Model>& model)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("addModel")) {
            Base::Console().Log("\taddModel()\n");
            Py::Callable libraries(_managerObject.getAttr("addModel"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(path.toStdString()));
            args.setItem(2, Py::Object(new ModelPy(new Model(*model)), true));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\taddModel() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

void ExternalManager::migrateModel(const QString& libraryName,
                               const QString& path,
                               const std::shared_ptr<Model>& model)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("migrateModel")) {
            Base::Console().Log("\tmigrateModel()\n");
            Py::Callable libraries(_managerObject.getAttr("migrateModel"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(path.toStdString()));
            args.setItem(2, Py::Object(new ModelPy(new Model(*model)), true));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\tmigrateModel() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

//=====
//
// Material management
//
//=====

void ExternalManager::addMaterial(const QString& libraryName,
                                  const QString& path,
                                  const std::shared_ptr<Material>& material)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("addMaterial")) {
            Base::Console().Log("\taddMaterial()\n");
            Py::Callable libraries(_managerObject.getAttr("addMaterial"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(path.toStdString()));
            args.setItem(2, Py::Object(new MaterialPy(new Material(*material)), true));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\taddMaterial() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

void ExternalManager::migrateMaterial(const QString& libraryName,
                                      const QString& path,
                                      const std::shared_ptr<Material>& material)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("migrateMaterial")) {
            Base::Console().Log("\tmigrateMaterial()\n");
            Py::Callable libraries(_managerObject.getAttr("migrateMaterial"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(path.toStdString()));
            auto mat = new Material(*material);
            Base::Console().Log("Physical model count %d\n", mat->getPhysicalModels()->size());
            Base::Console().Log("Appearance model count %d\n", mat->getAppearanceModels()->size());
            args.setItem(2, Py::Object(new MaterialPy(mat), true));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\tmigrateMaterial() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

std::shared_ptr<Material> ExternalManager::getMaterial(const QString& uuid)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("getMaterial")) {
            Base::Console().Log("\tgetMaterial()\n");
            Py::Callable libraries(_managerObject.getAttr("getMaterial"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(uuid.toStdString()));
            Py::Tuple result(libraries.apply(args));  // ignore return for now

            Py::Object uuidObject = result.getItem(0);
            Py::Object libraryObject = result.getItem(1);
            Py::Object modelObject = result.getItem(2);

            // Model* model = static_cast<ModelPy*>(*modelObject)->getModelPtr();
            // model->setUUID(uuid);
            // auto shared = std::make_shared<Model>(*model);

            return nullptr;
        }
        else {
            Base::Console().Log("\tgetMaterial() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw MaterialNotFound(e1.what());
    }
}
