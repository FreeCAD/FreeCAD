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

#include <Python.h>
#include <QMutex>
#include <QMutexLocker>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <CXX/Objects.hxx>

#include "Exceptions.h"
#include "ExternalManager.h"
#include "MaterialLibrary.h"
#include "MaterialLibraryPy.h"
#include "MaterialPy.h"
#include "ModelLibrary.h"
#include "ModelPy.h"
#include "MaterialFilterPy.h"
#include "MaterialFilterOptionsPy.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

ExternalManager* ExternalManager::_manager = nullptr;
QMutex ExternalManager::_mutex;

ExternalManager::ExternalManager()
    : _instantiated(false)
{
    _hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    _hGrp->Attach(this);

    getConfiguration();
}

ExternalManager::~ExternalManager()
{
    _hGrp->Detach(this);
}

void ExternalManager::OnChange(ParameterGrp::SubjectType& /*rCaller*/, ParameterGrp::MessageType Reason)
{
    if (std::strncmp(Reason, "Current", 7) == 0) {
        if (_instantiated) {
            // The old manager object will be deleted when reconnecting
            _instantiated = false;
        }
        getConfiguration();
    }
}

void ExternalManager::getConfiguration()
{
    // _hGrp = App::GetApplication().GetParameterGroupByPath(
    //     "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface");
    auto current = _hGrp->GetASCII("Current", "None");
    if (current == "None") {
        _moduleName = "";
        _className = "";
    }
    else {
        auto groupName =
            "User parameter:BaseApp/Preferences/Mod/Material/ExternalInterface/Interfaces/"
            + current;
        auto hGrp = App::GetApplication().GetParameterGroupByPath(groupName.c_str());
        _moduleName = hGrp->GetASCII("Module", "");
        _className = hGrp->GetASCII("Class", "");
    }
}

void ExternalManager::instantiate()
{
    _instantiated = false;
    Base::Console().Log("Loading external manager...\n");

    if (_moduleName.empty() || _className.empty()) {
        Base::Console().Log("External module not defined\n");
        return;
    }

    try {
        Base::PyGILStateLocker lock;
        Py::Module mod(PyImport_ImportModule(_moduleName.c_str()), true);

        if (mod.isNull()) {
            Base::Console().Log(" failed\n");
            return;
        }

        Py::Callable managerClass(mod.getAttr(_className));
        _managerObject = managerClass.apply();
        if (_managerObject.hasAttr("APIVersion")) {
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

bool ExternalManager::checkMaterialLibraryType(const Py::Object& entry)
{
    return entry.hasAttr("name") && entry.hasAttr("icon") && entry.hasAttr("readOnly")
        && entry.hasAttr("timestamp");
}

std::shared_ptr<Library>
ExternalManager::libraryFromObject(const Py::Object& entry)
{
    if (!checkMaterialLibraryType(entry)) {
        throw InvalidLibrary();
    }

    Py::String pyName(entry.getAttr("name"));
    Py::Bytes pyIcon(entry.getAttr("icon"));
    Py::Boolean pyReadOnly(entry.getAttr("readOnly"));
    Py::String pyTimestamp(entry.getAttr("timestamp"));

    QString libraryName;
    if (!pyName.isNone()) {
        libraryName = QString::fromStdString(pyName.as_string());
    }

    QString icon;
    if (!pyIcon.isNone()) {
        icon = QString::fromStdString(pyIcon.as_string());
    }

    bool readOnly = pyReadOnly.as_bool();

    QString timestamp;
    if (!pyTimestamp.isNone()) {
        timestamp = QString::fromStdString(pyTimestamp.as_string());
    }

    auto library = std::make_shared<Library>(libraryName, icon, readOnly, timestamp);
    return library;
}

bool ExternalManager::checkMaterialObjectType(const Py::Object& entry)
{
    return entry.hasAttr("UUID") && entry.hasAttr("path") && entry.hasAttr("name");
}

std::tuple<QString, QString, QString>
ExternalManager::materialObjectTypeFromObject(const Py::Object& entry)
{
    QString uuid;
    auto pyUUID = entry.getAttr("UUID");
    if (!pyUUID.isNone()) {
        uuid = QString::fromStdString(pyUUID.as_string());
    }

    QString path;
    auto pyPath = entry.getAttr("path");
    if (!pyPath.isNone()) {
        path = QString::fromStdString(pyPath.as_string());
    }

    QString name;
    auto pyName = entry.getAttr("name");
    if (!pyName.isNone()) {
        name = QString::fromStdString(pyName.as_string());
    }

    return std::tuple<QString, QString, QString>(uuid, path, name);
}

std::shared_ptr<std::vector<std::shared_ptr<Library>>>
ExternalManager::libraries()
{
    auto libList = std::make_shared<std::vector<std::shared_ptr<Library>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("libraries")) {
            Py::Callable libraries(_managerObject.getAttr("libraries"));
            Py::List list(libraries.apply());
            for (auto lib : list) {
                auto library = libraryFromObject(Py::Object(lib));
                libList->push_back(library);
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

std::shared_ptr<std::vector<std::shared_ptr<Library>>> ExternalManager::modelLibraries()
{
    auto libList = std::make_shared<std::vector<std::shared_ptr<Library>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("modelLibraries")) {
            Py::Callable libraries(_managerObject.getAttr("modelLibraries"));
            Py::List list(libraries.apply());
            for (auto lib : list) {
                auto library = libraryFromObject(Py::Tuple(lib));
                libList->push_back(library);
            }
        }
        else {
            Base::Console().Log("\tmodelLibraries() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return libList;
}

std::shared_ptr<std::vector<std::shared_ptr<Library>>> ExternalManager::materialLibraries()
{
    auto libList = std::make_shared<std::vector<std::shared_ptr<Library>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("materialLibraries")) {
            Py::Callable libraries(_managerObject.getAttr("materialLibraries"));
            Py::List list(libraries.apply());
            for (auto lib : list) {
                auto library = libraryFromObject(Py::Tuple(lib));
                libList->push_back(library);
            }
        }
        else {
            Base::Console().Log("\tmaterialLibraries() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return libList;
}

std::shared_ptr<Library> ExternalManager::getLibrary(const QString& name)
{
    // throw LibraryNotFound("Not yet implemented");
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("getLibrary")) {
            Py::Callable libraries(_managerObject.getAttr("getLibrary"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(name.toStdString()));
            Py::Tuple result(libraries.apply(args));

            Py::Object libObject = result.getItem(0);
            auto lib = libraryFromObject(Py::Tuple(libObject));
            return std::make_shared<Library>(*lib);
        }
        else {
            Base::Console().Log("\tgetLibrary() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw CreationError(e1.what());
    }
}

void ExternalManager::createLibrary(const QString& libraryName, const QString& icon, bool readOnly)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("createLibrary")) {
            Py::Callable libraries(_managerObject.getAttr("createLibrary"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(icon.toStdString()));
            args.setItem(2, Py::Boolean(readOnly));
            libraries.apply(args);  // No return expected
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

void ExternalManager::renameLibrary(const QString& libraryName, const QString& newName)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("renameLibrary")) {
            Py::Callable libraries(_managerObject.getAttr("renameLibrary"));
            Py::Tuple args(2);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(newName.toStdString()));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\trenameLibrary() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw RenameError(e1.what());
    }
}

void ExternalManager::changeIcon(const QString& libraryName, const QString& icon)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("changeIcon")) {
            Py::Callable libraries(_managerObject.getAttr("changeIcon"));
            Py::Tuple args(2);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(icon.toStdString()));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\tchangeIcon() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw ReplacementError(e1.what());
    }
}

void ExternalManager::removeLibrary(const QString& libraryName)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("removeLibrary")) {
            Py::Callable libraries(_managerObject.getAttr("removeLibrary"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(libraryName.toStdString()));
            libraries.apply(args);  // No return expected
        }
        else {
            Base::Console().Log("\tremoveLibrary() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw DeleteError(e1.what());
    }
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
ExternalManager::libraryModels(const QString& libraryName)
{
    auto modelList = std::make_shared<std::vector<std::tuple<QString, QString, QString>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("libraryModels")) {
            Py::Callable libraries(_managerObject.getAttr("libraryModels"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(libraryName.toStdString()));
            Py::List list(libraries.apply(args));
            for (auto library : list) {
                auto entry = Py::Object(library);
                if (!checkMaterialObjectType(entry)) {
                    throw InvalidModel();
                }

                modelList->push_back(materialObjectTypeFromObject(entry));
            }
        }
        else {
            Base::Console().Log("\tlibraryModels() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return modelList;
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
ExternalManager::libraryMaterials(const QString& libraryName)
{
    auto materialList = std::make_shared<std::vector<std::tuple<QString, QString, QString>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("libraryMaterials")) {
            Py::Callable libraries(_managerObject.getAttr("libraryMaterials"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(libraryName.toStdString()));
            Py::List list(libraries.apply(args));
            for (auto library : list) {
                auto entry = Py::Object(library);
                if (!checkMaterialObjectType(entry)) {
                    throw InvalidMaterial();
                }

                materialList->push_back(materialObjectTypeFromObject(entry));
            }
        }
        else {
            Base::Console().Log("\tlibraryMaterials() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return materialList;
}

std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
ExternalManager::libraryMaterials(const QString& libraryName,
                                  const std::shared_ptr<MaterialFilter>& filter,
                                  const MaterialFilterOptions& options)
{
    auto materialList = std::make_shared<std::vector<std::tuple<QString, QString, QString>>>();

    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("libraryMaterials")) {
            Py::Callable libraries(_managerObject.getAttr("libraryMaterials"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            if (filter) {
                args.setItem(1,
                             Py::Object(new MaterialFilterPy(new MaterialFilter(*filter)), true));
            }
            else {
                args.setItem(1, Py::None());
            }
            args.setItem(
                2,
                Py::Object(new MaterialFilterOptionsPy(new MaterialFilterOptions(options)), true));
            Py::List list(libraries.apply(args));
            for (auto library : list) {
                auto entry = Py::Object(library);
                if (!checkMaterialObjectType(entry)) {
                    throw InvalidMaterial();
                }

                materialList->push_back(materialObjectTypeFromObject(entry));
            }
        }
        else {
            Base::Console().Log("\tlibraryMaterials() not found\n");
            throw ConnectionError();
        }
    }
    catch (Py::Exception& e) {
        Base::PyException e1;  // extract the Python error text
        throw LibraryNotFound(e1.what());
    }

    return materialList;
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
            Py::Callable libraries(_managerObject.getAttr("getModel"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(uuid.toStdString()));
            Py::Tuple result(libraries.apply(args));  // ignore return for now

            Py::Object uuidObject = result.getItem(0);
            Py::Tuple libraryObject(result.getItem(1));
            Py::Object modelObject = result.getItem(2);

            Py::Object pyName = libraryObject.getItem(0);
            Py::Object pyIcon = libraryObject.getItem(1);
            Py::Object readOnly = libraryObject.getItem(2);

            QString name;
            if (!pyName.isNone()) {
                name = QString::fromStdString(pyName.as_string());
            }
            QString icon;
            if (!pyIcon.isNone()) {
                icon = QString::fromStdString(pyIcon.as_string());
            }
            auto library =
                std::make_shared<ModelLibrary>(name, QString(), icon, readOnly.as_bool());

            Model* model = static_cast<ModelPy*>(*modelObject)->getModelPtr();
            model->setUUID(uuid);
            model->setLibrary(library);
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

std::shared_ptr<Material> ExternalManager::getMaterial(const QString& uuid)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("getMaterial")) {
            Py::Callable libraries(_managerObject.getAttr("getMaterial"));
            Py::Tuple args(1);
            args.setItem(0, Py::String(uuid.toStdString()));
            Py::Tuple result(libraries.apply(args));

            Py::Object uuidObject = result.getItem(0);
            Py::Tuple libraryObject(result.getItem(1));
            Py::Object materialObject = result.getItem(2);

            Py::Object pyName = libraryObject.getItem(0);
            Py::Object pyIcon = libraryObject.getItem(1);
            Py::Object readOnly = libraryObject.getItem(2);

            QString name;
            if (!pyName.isNone()) {
                name = QString::fromStdString(pyName.as_string());
            }
            QString icon;
            if (!pyIcon.isNone()) {
                icon = QString::fromStdString(pyIcon.as_string());
            }
            auto library =
                std::make_shared<MaterialLibrary>(name, QString(), icon, readOnly.as_bool());

            Material* material = static_cast<MaterialPy*>(*materialObject)->getMaterialPtr();
            material->setUUID(uuid);
            material->setLibrary(library);
            auto shared = std::make_shared<Material>(*material);

            return shared;
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

void ExternalManager::addMaterial(const QString& libraryName,
                                  const QString& path,
                                  const std::shared_ptr<Material>& material)
{
    connect();

    Base::PyGILStateLocker lock;
    try {
        if (_managerObject.hasAttr("addMaterial")) {
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
            Py::Callable libraries(_managerObject.getAttr("migrateMaterial"));
            Py::Tuple args(3);
            args.setItem(0, Py::String(libraryName.toStdString()));
            args.setItem(1, Py::String(path.toStdString()));
            auto mat = new Material(*material);
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
