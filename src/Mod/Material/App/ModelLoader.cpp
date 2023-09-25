/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#include <QString>
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>

#include <QDirIterator>
#include <QFileInfo>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

ModelEntry::ModelEntry(const ModelLibrary& library,
                       const QString& baseName,
                       const QString& modelName,
                       const QString& dir,
                       const QString& modelUuid,
                       const YAML::Node& modelData)
    : _library(library)
    , _base(baseName)
    , _name(modelName)
    , _directory(dir)
    , _uuid(modelUuid)
    , _model(modelData)
    , _dereferenced(false)
{}

std::unique_ptr<std::map<QString, ModelEntry*>> ModelLoader::_modelEntryMap = nullptr;

ModelLoader::ModelLoader(std::shared_ptr<std::map<QString, Model*>> modelMap,
                         std::shared_ptr<std::list<ModelLibrary*>> libraryList)
    : _modelMap(modelMap)
    , _libraryList(libraryList)
{
    loadLibraries();
}

void ModelLoader::addLibrary(ModelLibrary* model)
{
    _libraryList->push_back(model);
}

const QString ModelLoader::getUUIDFromPath(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        throw ModelNotFound();
    }

    try {
        YAML::Node yamlroot = YAML::LoadFile(path.toStdString());
        std::string base = "Model";
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        const QString uuid = QString::fromStdString(yamlroot[base]["UUID"].as<std::string>());
        return uuid;
    }
    catch (YAML::Exception& ex) {
        throw ModelNotFound();
    }
}

ModelEntry* ModelLoader::getModelFromPath(const ModelLibrary& library, const QString& path) const
{
    QFile file(path);
    if (!file.exists()) {
        throw ModelNotFound();
    }

    YAML::Node yamlroot;
    std::string base = "Model";
    std::string uuid;
    std::string name;
    try {
        yamlroot = YAML::LoadFile(path.toStdString());
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        uuid = yamlroot[base]["UUID"].as<std::string>();
        name = yamlroot[base]["Name"].as<std::string>();
    }
    catch (YAML::Exception const&) {
        throw InvalidModel();
    }

    ModelEntry* model = new ModelEntry(library,
                                       QString::fromStdString(base),
                                       QString::fromStdString(name),
                                       path,
                                       QString::fromStdString(uuid),
                                       yamlroot);

    return model;
}

void ModelLoader::showYaml(const YAML::Node& yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().Log("%s\n", logData.c_str());
}

void ModelLoader::dereference(const QString& uuid,
                              ModelEntry* parent,
                              const ModelEntry* child,
                              std::map<std::pair<QString, QString>, QString>* inheritances)
{
    auto parentPtr = parent->getModelPtr();
    auto parentBase = parent->getBase().toStdString();
    auto childYaml = child->getModel();
    auto childBase = child->getBase().toStdString();

    std::set<QString> exclude;
    exclude.insert(QString::fromStdString("Name"));
    exclude.insert(QString::fromStdString("UUID"));
    exclude.insert(QString::fromStdString("URL"));
    exclude.insert(QString::fromStdString("Description"));
    exclude.insert(QString::fromStdString("DOI"));
    exclude.insert(QString::fromStdString("Inherits"));

    auto parentProperties = (*parentPtr)[parentBase];
    auto childProperties = childYaml[childBase];
    for (auto it = childProperties.begin(); it != childProperties.end(); it++) {
        std::string name = it->first.as<std::string>();
        if (exclude.count(QString::fromStdString(name)) == 0) {
            // showYaml(it->second);
            if (!parentProperties[name]) {
                parentProperties[name] = it->second;
                // parentProperties[name]["Inherits"] = childYaml[childBase]["UUID"];
                (*inheritances)[std::pair<QString, QString>(uuid, QString::fromStdString(name))] =
                    yamlValue(childYaml[childBase], "UUID", "");
            }
        }
    }
    // showYaml(*parentPtr);
}


void ModelLoader::dereference(ModelEntry* model,
                              std::map<std::pair<QString, QString>, QString>* inheritances)
{
    // Avoid recursion
    if (model->getDereferenced()) {
        return;
    }

    auto yamlModel = model->getModel();
    auto base = model->getBase().toStdString();
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            QString nodeName = QString::fromStdString((*it)["UUID"].as<std::string>());

            // This requires that all models have already been loaded undereferenced
            try {
                const ModelEntry* child = (*_modelEntryMap)[nodeName];
                dereference(model->getUUID(), model, child, inheritances);
            }
            catch (const std::out_of_range& oor) {
                Base::Console().Log("Unable to find '%s' in model map\n",
                                    nodeName.toStdString().c_str());
            }
        }
    }

    model->markDereferenced();
}

QString ModelLoader::yamlValue(const YAML::Node& node,
                               const std::string& key,
                               const std::string& defaultValue)
{
    if (node[key]) {
        return QString::fromStdString(node[key].as<std::string>());
    }
    return QString::fromStdString(defaultValue);
}

void ModelLoader::addToTree(ModelEntry* model,
                            std::map<std::pair<QString, QString>, QString>* inheritances)
{
    std::set<QString> exclude;
    exclude.insert(QString::fromStdString("Name"));
    exclude.insert(QString::fromStdString("UUID"));
    exclude.insert(QString::fromStdString("URL"));
    exclude.insert(QString::fromStdString("Description"));
    exclude.insert(QString::fromStdString("DOI"));
    exclude.insert(QString::fromStdString("Inherits"));

    auto yamlModel = model->getModel();
    auto library = model->getLibrary();
    auto base = model->getBase().toStdString();
    auto name = model->getName();
    auto directory = model->getDirectory();
    auto uuid = model->getUUID();

    QString version = yamlValue(yamlModel["General"], "Version", "");

    QString description = yamlValue(yamlModel[base], "Description", "");
    QString url = yamlValue(yamlModel[base], "URL", "");
    QString doi = yamlValue(yamlModel[base], "DOI", "");

    Model::ModelType type =
        (base == "Model") ? Model::ModelType_Physical : Model::ModelType_Appearance;

    Model* finalModel = new Model(library, type, name, directory, uuid, description, url, doi);

    // Add inheritance list
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            QString nodeName = QString::fromStdString((*it)["UUID"].as<std::string>());

            finalModel->addInheritance(nodeName);
        }
    }

    // Add property list
    auto yamlProperties = yamlModel[base];
    for (auto it = yamlProperties.begin(); it != yamlProperties.end(); it++) {
        std::string propName = it->first.as<std::string>();
        if (exclude.count(QString::fromStdString(propName)) == 0) {
            // showYaml(it->second);
            auto yamlProp = yamlProperties[propName];
            auto propType = yamlValue(yamlProp, "Type", "");
            auto propUnits = yamlValue(yamlProp, "Units", "");
            auto propURL = yamlValue(yamlProp, "URL", "");
            auto propDescription = yamlValue(yamlProp, "Description", "");
            // auto inherits = yamlValue(yamlProp, "Inherits", "");

            ModelProperty property(QString::fromStdString(propName),
                                   propType,
                                   propUnits,
                                   propURL,
                                   propDescription);

            if (propType == QString::fromStdString("2DArray")
                || propType == QString::fromStdString("3DArray")) {
                Base::Console().Log("Reading columns\n");
                // Read the columns
                auto cols = yamlProp["Columns"];
                for (auto col : cols) {
                    std::string colName = col.first.as<std::string>();
                    Base::Console().Log("\tColumns '%s'\n", colName.c_str());

                    auto colProp = cols[colName];
                    auto colPropType = yamlValue(colProp, "Type", "");
                    auto colPropUnits = yamlValue(colProp, "Units", "");
                    auto colPropURL = yamlValue(colProp, "URL", "");
                    auto colPropDescription = yamlValue(colProp, "Description", "");
                    ModelProperty colProperty(QString::fromStdString(colName),
                                              colPropType,
                                              colPropUnits,
                                              colPropURL,
                                              colPropDescription);

                    property.addColumn(colProperty);
                }
            }

            auto key = std::pair<QString, QString>(uuid, QString::fromStdString(propName));
            if (inheritances->count(key) > 0) {
                property.setInheritance((*inheritances)[key]);
            }

            finalModel->addProperty(property);
        }
    }

    (*_modelMap)[uuid] = library.addModel(*finalModel, directory);
}

void ModelLoader::loadLibrary(const ModelLibrary& library)
{
    if (_modelEntryMap == nullptr) {
        _modelEntryMap = std::make_unique<std::map<QString, ModelEntry*>>();
    }

    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "yml") {
                QString libraryName = file.baseName();

                try {
                    auto model = getModelFromPath(library, file.canonicalFilePath());
                    (*_modelEntryMap)[model->getUUID()] = model;
                    // showYaml(model->getModel());
                }
                catch (InvalidModel const&) {
                    Base::Console().Log("Invalid model '%s'\n", pathname.toStdString().c_str());
                }
            }
        }
    }

    std::map<std::pair<QString, QString>, QString>* inheritances =
        new std::map<std::pair<QString, QString>, QString>();
    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        dereference(it->second, inheritances);
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        addToTree(it->second, inheritances);
    }
    // delete inheritances;
}

void ModelLoader::loadLibraries()
{
    getModelLibraries();
    if (_libraryList) {
        for (auto it = _libraryList->begin(); it != _libraryList->end(); it++) {
            loadLibrary(**it);
        }
    }
}

void ModelLoader::getModelLibraries()
{
    auto param = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    if (useBuiltInMaterials) {
        QString resourceDir = QString::fromStdString(App::Application::getResourceDir()
                                                     + "/Mod/Material/Resources/Models");
        auto libData = new ModelLibrary(QString::fromStdString("System"),
                                        resourceDir,
                                        QString::fromStdString(":/icons/freecad.svg"));
        _libraryList->push_back(libData);
    }

    if (useMatFromModules) {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto& group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = QString::fromStdString(group->GetGroupName());
            auto modelDir = QString::fromStdString(group->GetASCII("ModuleModelDir", ""));
            auto modelIcon = QString::fromStdString(group->GetASCII("ModuleIcon", ""));

            if (modelDir.length() > 0) {
                QDir dir(modelDir);
                if (dir.exists()) {
                    auto libData = new ModelLibrary(moduleName, modelDir, modelIcon);
                    _libraryList->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir) {
        QString resourceDir =
            QString::fromStdString(App::Application::getUserAppDataDir() + "/Models");
        if (!resourceDir.isEmpty()) {
            QDir materialDir(resourceDir);
            if (materialDir.exists()) {
                auto libData =
                    new ModelLibrary(QString::fromStdString("User"),
                                     resourceDir,
                                     QString::fromStdString(":/icons/preferences-general.svg"));
                _libraryList->push_back(libData);
            }
        }
    }

    if (useMatFromCustomDir) {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        if (!resourceDir.isEmpty()) {
            QDir materialDir(resourceDir);
            if (materialDir.exists()) {
                auto libData = new ModelLibrary(QString::fromStdString("Custom"),
                                                resourceDir,
                                                QString::fromStdString(":/icons/user.svg"));
                _libraryList->push_back(libData);
            }
        }
    }
}
