// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#include <App/Application.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <Base/Stream.h>


#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

ModelEntry::ModelEntry(const std::shared_ptr<ModelLibraryLocal>& library,
                       const std::string& baseName,
                       const std::string& modelName,
                       const std::string& dir,
                       const std::string& modelUuid,
                       const YAML::Node& modelData)
    : _library(library)
    , _base(baseName)
    , _name(modelName)
    , _directory(dir)
    , _uuid(modelUuid)
    , _model(modelData)
    , _dereferenced(false)
{}

std::unique_ptr<std::map<std::string, std::shared_ptr<ModelEntry>>> ModelLoader::_modelEntryMap =
    nullptr;

ModelLoader::ModelLoader(std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> modelMap,
                         std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> libraryList)
    : _modelMap(modelMap)
    , _libraryList(libraryList)
{
    loadLibraries();
}

void ModelLoader::addLibrary(std::shared_ptr<ModelLibraryLocal> model)
{
    _libraryList->push_back(model);
}

std::string ModelLoader::getUUIDFromPath(const std::string& path)
{
    Base::FileInfo fi(path);
    if (!fi.exists()) {
        throw ModelNotFound();
    }

    try {
        Base::ifstream str(fi);
        YAML::Node yamlroot = YAML::Load(str);
        std::string base = "Model";
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        return yamlroot[base]["UUID"].as<std::string>();
    }
    catch (YAML::Exception&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<ModelEntry> ModelLoader::getModelFromPath(std::shared_ptr<ModelLibrary> library,
                                                          const std::string& path) const
{
    Base::FileInfo fi(path);
    if (!fi.exists()) {
        throw ModelNotFound();
    }

    YAML::Node yamlroot;
    std::string base = "Model";
    std::string uuid;
    std::string name;
    try {
        Base::ifstream str(fi);
        yamlroot = YAML::Load(str);
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        uuid = yamlroot[base]["UUID"].as<std::string>();
        name = yamlroot[base]["Name"].as<std::string>();
    }
    catch (YAML::Exception const&) {
        throw InvalidModel();
    }

    auto localLibrary = std::static_pointer_cast<ModelLibraryLocal>(library);
    std::shared_ptr<ModelEntry> model =
        std::make_shared<ModelEntry>(localLibrary, base, name, path, uuid, yamlroot);

    return model;
}

void ModelLoader::showYaml(const YAML::Node& yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().log("{}\n", logData);
}

void ModelLoader::dereference(const std::string& uuid,
                              std::shared_ptr<ModelEntry> parent,
                              std::shared_ptr<ModelEntry> child,
                              std::map<std::pair<std::string, std::string>, std::string>* inheritances)
{
    auto parentPtr = parent->getModelPtr();
    const auto& parentBase = parent->getBase();
    auto childYaml = child->getModel();
    const auto& childBase = child->getBase();

    std::set<std::string> exclude;
    exclude.insert("Name");
    exclude.insert("UUID");
    exclude.insert("URL");
    exclude.insert("Description");
    exclude.insert("DOI");
    exclude.insert("Inherits");

    auto parentProperties = (*parentPtr)[parentBase];
    auto childProperties = childYaml[childBase];
    for (auto it = childProperties.begin(); it != childProperties.end(); it++) {
        std::string name = it->first.as<std::string>();
        if (!exclude.contains(name)) {
            // showYaml(it->second);
            if (!parentProperties[name]) {
                parentProperties[name] = it->second;
                // parentProperties[name]["Inherits"] = childYaml[childBase]["UUID"];
                (*inheritances)[std::pair<std::string, std::string>(uuid, name)] =
                    yamlValue(childYaml[childBase], "UUID", "");
            }
        }
    }
    // showYaml(*parentPtr);
}


void ModelLoader::dereference(std::shared_ptr<ModelEntry> model,
                              std::map<std::pair<std::string, std::string>, std::string>* inheritances)
{
    // Avoid recursion
    if (model->getDereferenced()) {
        return;
    }

    auto yamlModel = model->getModel();
    const auto& base = model->getBase();
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            const auto nodeName = (*it)["UUID"].as<std::string>();

            // This requires that all models have already been loaded undereferenced
            try {
                std::shared_ptr<ModelEntry> child = (*_modelEntryMap)[nodeName];
                dereference(model->getUUID(), model, child, inheritances);
            }
            catch (const std::out_of_range&) {
                Base::Console().log("Unable to find '{}' in model map\n",
                                    nodeName);
            }
        }
    }

    model->markDereferenced();
}

std::string ModelLoader::yamlValue(const YAML::Node& node,
                               const std::string& key,
                               const std::string& defaultValue)
{
    if (node[key]) {
        return node[key].as<std::string>();
    }
    return defaultValue;
}

void ModelLoader::addToTree(std::shared_ptr<ModelEntry> model,
                            std::map<std::pair<std::string, std::string>, std::string>* inheritances)
{
    std::set<std::string> exclude;
    exclude.insert("Name");
    exclude.insert("UUID");
    exclude.insert("URL");
    exclude.insert("Description");
    exclude.insert("DOI");
    exclude.insert("Inherits");

    auto yamlModel = model->getModel();
    if (!model->getLibrary()->isLocal()) {
        throw InvalidLibrary();
    }
    auto library = model->getLibrary();
    const auto& base = model->getBase();
    auto name = model->getName();
    auto directory = model->getDirectory();
    auto uuid = model->getUUID();

    std::string description = yamlValue(yamlModel[base], "Description", "");
    std::string url = yamlValue(yamlModel[base], "URL", "");
    std::string doi = yamlValue(yamlModel[base], "DOI", "");

    Model::ModelType type =
        (base == "Model") ? Model::ModelType_Physical : Model::ModelType_Appearance;

    Model finalModel(library, type, name, directory, uuid, description, url, doi);

    // Add inheritance list
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for (auto it = inherits.begin(); it != inherits.end(); it++) {
            const auto nodeName = (*it)["UUID"].as<std::string>();

            finalModel.addInheritance(nodeName);
        }
    }

    // Add property list
    auto yamlProperties = yamlModel[base];
    for (auto it = yamlProperties.begin(); it != yamlProperties.end(); it++) {
        std::string propName = it->first.as<std::string>();
        if (!exclude.contains(propName)) {
            // showYaml(it->second);
            auto yamlProp = yamlProperties[propName];
            auto propDisplayName = yamlValue(yamlProp, "DisplayName", "");
            auto propType = yamlValue(yamlProp, "Type", "");
            auto propUnits = yamlValue(yamlProp, "Units", "");
            auto propURL = yamlValue(yamlProp, "URL", "");
            auto propDescription = yamlValue(yamlProp, "Description", "");
            // auto inherits = yamlValue(yamlProp, "Inherits", "");

            ModelProperty property(propName,
                                   propDisplayName,
                                   propType,
                                   propUnits,
                                   propURL,
                                   propDescription);

            if (propType == "2DArray" || propType == "3DArray") {
                // Base::Console().Log("Reading columns\n");
                // Read the columns
                auto cols = yamlProp["Columns"];
                for (const auto& col : cols) {
                    std::string colName = col.first.as<std::string>();
                    // Base::Console().Log("\tColumns '{}'\n", colName);

                    auto colProp = cols[colName];
                    auto colPropDisplayName = yamlValue(colProp, "DisplayName", "");
                    auto colPropType = yamlValue(colProp, "Type", "");
                    auto colPropUnits = yamlValue(colProp, "Units", "");
                    auto colPropURL = yamlValue(colProp, "URL", "");
                    auto colPropDescription = yamlValue(colProp, "Description", "");
                    ModelProperty colProperty(colName,
                                              colPropDisplayName,
                                              colPropType,
                                              colPropUnits,
                                              colPropURL,
                                              colPropDescription);

                    property.addColumn(colProperty);
                }
            }

            auto key = std::pair<std::string, std::string>(uuid, propName);
            if (inheritances->contains(key)) {
                property.setInheritance((*inheritances)[key]);
            }

            finalModel.addProperty(property);
        }
    }

    (*_modelMap)[uuid] = library->addModel(finalModel, directory);
}

void ModelLoader::loadLibrary(std::shared_ptr<ModelLibraryLocal> library)
{
    if (_modelEntryMap == nullptr) {
        _modelEntryMap = std::make_unique<std::map<std::string, std::shared_ptr<ModelEntry>>>();
    }

    QDirIterator it(QString::fromStdString(library->getDirectory()),
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix() == QLatin1String("yml")) {
                try {
                    auto model =
                        getModelFromPath(library, file.canonicalFilePath().toStdString());
                    (*_modelEntryMap)[model->getUUID()] = model;
                    // showYaml(model->getModel());
                }
                catch (InvalidModel const&) {
                    Base::Console().log("Invalid model '{}'\n", pathname.toStdString());
                }
            }
        }
    }

    std::map<std::pair<std::string, std::string>, std::string> inheritances;
    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        dereference(it->second, &inheritances);
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        addToTree(it->second, &inheritances);
    }
}

void ModelLoader::loadLibraries()
{
    getModelLibraries();
    if (_libraryList) {
        for (auto& it : *_libraryList) {
            if (it->isLocal()) {
                auto modelLibrary = std::dynamic_pointer_cast<Materials::ModelLibraryLocal>(it);
                if (modelLibrary) {
                    loadLibrary(modelLibrary);
                }
            }
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
        const std::string resourceDir =
            App::Application::getResourceDir() + "/Mod/Material/Resources/Models";
        auto libData =
            std::make_shared<ModelLibraryLocal>("System", resourceDir, ":/icons/freecad.svg");
        _libraryList->push_back(libData);
    }

    if (useMatFromModules) {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto& group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = group->GetGroupName();
            auto modelDir = group->GetASCII("ModuleModelDir", "");
            auto modelIcon = group->GetASCII("ModuleIcon", "");

            if (!modelDir.empty()) {
                if (QDir(QString::fromStdString(modelDir)).exists()) {
                    auto libData = std::make_shared<ModelLibraryLocal>(moduleName, modelDir, modelIcon);
                    _libraryList->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir) {
        const std::string resourceDir = App::Application::getUserAppDataDir() + "/Models";
        if (!resourceDir.empty()) {
            if (QDir(QString::fromStdString(resourceDir)).exists()) {
                auto libData = std::make_shared<ModelLibraryLocal>(
                    "User",
                    resourceDir,
                    ":/icons/preferences-general.svg");
                _libraryList->push_back(libData);
            }
        }
    }

    if (useMatFromCustomDir) {
        const std::string resourceDir = param->GetASCII("CustomMaterialsDir", "");
        if (!resourceDir.empty()) {
            if (QDir(QString::fromStdString(resourceDir)).exists()) {
                auto libData = std::make_shared<ModelLibraryLocal>("Custom",
                                                              resourceDir,
                                                              ":/icons/user.svg");
                _libraryList->push_back(libData);
            }
        }
    }
}
