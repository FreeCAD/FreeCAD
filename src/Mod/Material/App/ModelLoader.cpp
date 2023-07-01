/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#endif

#include <App/Application.h>
#include <Base/Interpreter.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManager.h"


using namespace Materials;

ModelEntry::ModelEntry(const ModelLibrary &library, const std::string &baseName, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData):
    _library(library), _base(baseName), _name(modelName), _directory(dir), _uuid(modelUuid), _model(modelData), _dereferenced(false)
{}

ModelEntry::~ModelEntry()
{}

std::map<std::string, ModelEntry*> *ModelLoader::_modelEntryMap = nullptr;

ModelLoader::ModelLoader(std::map<std::string, Model*> *modelMap, std::list<ModelLibrary*> *libraryList) :
    _modelMap(modelMap), _libraryList(libraryList)
{
    loadLibraries();
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelLoader::~ModelLoader()
{}

void ModelLoader::addLibrary(ModelLibrary *model)
{
    _libraryList->push_back(model);
}

const std::string ModelLoader::getUUIDFromPath(const std::string &path)
{
    QFile file(QString::fromStdString(path));
    if (!file.exists())
        throw ModelNotFound();

    try {
        YAML::Node yamlroot = YAML::LoadFile(path);
        std::string base = "Model";
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        const std::string uuid = yamlroot[base]["UUID"].as<std::string>();
        return uuid;
    } catch (YAML::Exception &ex) {
        throw ModelNotFound();
    }
}

ModelEntry *ModelLoader::getModelFromPath(const ModelLibrary &library, const std::string &path) const
{
    QFile file(QString::fromStdString(path));
    if (!file.exists())
        throw ModelNotFound();

    YAML::Node yamlroot;
    std::string base = "Model";
    std::string uuid;
    std::string name;
    try {
        yamlroot = YAML::LoadFile(path);
        if (yamlroot["AppearanceModel"]) {
            base = "AppearanceModel";
        }

        uuid = yamlroot[base]["UUID"].as<std::string>();
        name = yamlroot[base]["Name"].as<std::string>();
    } catch (YAML::Exception const &) {
        throw InvalidModel();
    }

    QDir modelDir(QString::fromStdString(path));
    ModelEntry *model = new ModelEntry(library, base, name, modelDir, uuid, yamlroot);

    return model;
}

void ModelLoader::showYaml(const YAML::Node &yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().Log("%s\n", logData.c_str());
}

void ModelLoader::dereference(const std::string &uuid, ModelEntry *parent, const ModelEntry *child, 
    std::map<std::pair<std::string, std::string>, std::string> *inheritances)
{
    auto parentPtr = parent->getModelPtr();
    auto parentBase = parent->getBase();
    auto childYaml = child->getModel();
    auto childBase = child->getBase();

    std::set<std::string> exclude;
    exclude.insert("Name");
    exclude.insert("UUID");
    exclude.insert("URL");
    exclude.insert("Description");
    exclude.insert("DOI");
    exclude.insert("Inherits");

    auto parentProperties = (*parentPtr)[parentBase];
    auto childProperties = childYaml[childBase];
    for(auto it = childProperties.begin(); it != childProperties.end(); it++) {
        std::string name = it->first.as<std::string>();
        if (exclude.count(name) == 0) {
            // showYaml(it->second);
            if (!parentProperties[name]) {
                parentProperties[name] = it->second;
                // parentProperties[name]["Inherits"] = childYaml[childBase]["UUID"];
                (*inheritances)[std::pair<std::string, std::string>(uuid, name)] = yamlValue(childYaml[childBase], "UUID", "");
            }
        }
    }
    // showYaml(*parentPtr);
}


void ModelLoader::dereference(ModelEntry *model, std::map<std::pair<std::string, std::string>, std::string> *inheritances)
{
    // Avoid recursion
    if (model->getDereferenced())
        return;

    auto yamlModel = model->getModel();
    auto base = model->getBase();
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for(auto it = inherits.begin(); it != inherits.end(); it++) {
            // auto nodeName = it->first.as<std::string>();
            std::string nodeName = (*it)["UUID"].as<std::string>();

            // This requires that all models have already been loaded undereferenced
            try {
                const ModelEntry *child = (*_modelEntryMap)[nodeName];
                dereference(model->getUUID(), model, child, inheritances);
            }
            catch (const std::out_of_range& oor) {
                Base::Console().Log("Unable to find '%s' in model map\n", nodeName.c_str());
            }
        }
    }

    model->markDereferenced();
}

std::string ModelLoader::yamlValue(const YAML::Node &node, const std::string &key, const std::string &defaultValue)
{
    if (node[key])
        return node[key].as<std::string>();
    return defaultValue;
}

void ModelLoader::addToTree(ModelEntry *model, std::map<std::pair<std::string, std::string>, std::string> *inheritances)
{
    std::set<std::string> exclude;
    exclude.insert("Name");
    exclude.insert("UUID");
    exclude.insert("URL");
    exclude.insert("Description");
    exclude.insert("DOI");
    exclude.insert("Inherits");

    auto yamlModel = model->getModel();
    auto library = model->getLibrary();
    auto base = model->getBase();
    auto name = model->getName();
    auto directory = model->getDirectory();
    auto uuid = model->getUUID();

    std::string version = yamlValue(yamlModel["General"], "Version", "");

    std::string description = yamlValue(yamlModel[base], "Description", "");
    std::string url = yamlValue(yamlModel[base], "URL", "");
    std::string doi = yamlValue(yamlModel[base], "DOI", "");

    Model::ModelType type = (base == "Model") ? Model::ModelType_Physical : Model::ModelType_Appearance;

    Model *finalModel = new Model(library, type, name, directory, uuid, description, url, doi);

    // Add inheritance list
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        for(auto it = inherits.begin(); it != inherits.end(); it++) {
            std::string nodeName = (*it)["UUID"].as<std::string>();

            finalModel->addInheritance(nodeName);
        }
    }

    // Add property list
    auto yamlProperties = yamlModel[base];
    for(auto it = yamlProperties.begin(); it != yamlProperties.end(); it++) {
        std::string propName = it->first.as<std::string>();
        if (exclude.count(propName) == 0) {
            // showYaml(it->second);
            auto yamlProp = yamlProperties[propName];
            auto propType = yamlValue(yamlProp, "Type", "");
            auto propUnits = yamlValue(yamlProp, "Units", "");
            auto propURL = yamlValue(yamlProp, "URL", "");
            auto propDescription = yamlValue(yamlProp, "Description", "");
            // auto inherits = yamlValue(yamlProp, "Inherits", "");

            ModelProperty property(propName, propType,
                           propUnits, propURL,
                           propDescription);

            if (propType == "2DArray" || propType == "3DArray")
            {
                Base::Console().Log("Reading columns\n");
                // Read the columns
                auto cols = yamlProp["Columns"];
                for (auto col: cols)
                {
                    std::string colName = col.first.as<std::string>();
                    Base::Console().Log("\tColumns '%s'\n", colName.c_str());

                    auto colProp = cols[colName];
                    auto colPropType = yamlValue(colProp, "Type", "");
                    auto colPropUnits = yamlValue(yamlProp, "Units", "");
                    auto colPropURL = yamlValue(yamlProp, "URL", "");
                    auto colPropDescription = yamlValue(yamlProp, "Description", "");
                    ModelProperty colProperty(colName, colPropType,
                                colPropUnits, colPropURL,
                                colPropDescription);

                    property.addColumn(colProperty);
                }
            }

            auto key = std::pair<std::string, std::string>(uuid, propName);
            if (inheritances->count(key) > 0)
                property.setInheritance((*inheritances)[key]);

            finalModel->addProperty(property);
        }
    }

    (*_modelMap)[uuid] = finalModel;
}

void ModelLoader::loadLibrary(const ModelLibrary &library)
{
    if (_modelEntryMap == nullptr)
        _modelEntryMap = new std::map<std::string, ModelEntry*>();

    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "yml") {
                std::string libraryName = file.baseName().toStdString();

                try {
                    auto model = getModelFromPath(library, file.canonicalFilePath().toStdString());
                    (*_modelEntryMap)[model->getUUID()] = model;
                    // showYaml(model->getModel());
                } catch (InvalidModel const&) {
                    Base::Console().Log("Invalid model '%s'\n", pathname.toStdString().c_str());
                }
            }
        }
    }

    std::map<std::pair<std::string, std::string>, std::string>* inheritances =
        new std::map<std::pair<std::string, std::string>, std::string>();
    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        dereference(it->second, inheritances);
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        addToTree(it->second, inheritances);
    }
    // delete inheritances;
}

void ModelLoader::loadLibraries(void)
{
    std::list<ModelLibrary*>* _libraryList = getModelLibraries();
    if (_libraryList) {
        for (auto it = _libraryList->begin(); it != _libraryList->end(); it++) {
            loadLibrary(**it);
        }
    }
}

std::list<ModelLibrary *> *ModelLoader::getModelLibraries()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    if (useBuiltInMaterials)
    {
        QString resourceDir = QString::fromStdString(App::Application::getResourceDir() + "/Mod/Material/Resources/Models");
        QDir materialDir(resourceDir);
        auto libData = new ModelLibrary("System", materialDir, ":/icons/freecad.svg");
        _libraryList->push_back(libData);
    }

    if (useMatFromModules)
    {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto &group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = group->GetGroupName();
            auto modelDir = group->GetASCII("ModuleModelDir", "");
            auto modelIcon = group->GetASCII("ModuleIcon", "");

            if (modelDir.length() > 0) {
                QDir dir(QString::fromStdString(modelDir));
                auto libData = new ModelLibrary(moduleName, dir, modelIcon);
                if (dir.exists()) {
                    _libraryList->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir)
    {
        QString resourceDir = QString::fromStdString(App::Application::getUserAppDataDir() + "/Models");
        QDir materialDir(resourceDir);
        auto libData = new ModelLibrary("User", materialDir, ":/icons/preferences-general.svg");
        if (materialDir.exists()) {
            _libraryList->push_back(libData);
        }
    }

    if (useMatFromCustomDir)
    {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        QDir materialDir(resourceDir);
        auto libData = new ModelLibrary("Custom", materialDir, ":/icons/user.svg");
        if (materialDir.exists()) {
            _libraryList->push_back(libData);
        }
    }

    return _libraryList;
}


#include "moc_ModelLoader.cpp"
