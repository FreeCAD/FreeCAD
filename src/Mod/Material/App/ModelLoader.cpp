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


using namespace Material;

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

ModelEntry *ModelLoader::getModelFromPath(const ModelLibrary &library, const std::string &path) const
{
    YAML::Node yamlroot = YAML::LoadFile(path);
    std::string base = "Model";
    if (yamlroot["AppearanceModel"]) {
        base = "AppearanceModel";
    }

    const std::string uuid = yamlroot[base]["UUID"].as<std::string>();
    const std::string name = yamlroot[base]["Name"].as<std::string>();

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

void ModelLoader::dereference(ModelEntry *parent, const ModelEntry *child)
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
            if (!parentProperties[name])
                parentProperties[name] = it->second;
        }
    }
    showYaml(*parentPtr);
}


void ModelLoader::dereference(ModelEntry *model)
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
                dereference(model, child);
            }
            catch (const std::out_of_range& oor) {
                Base::Console().Log("Unable to find '%s' in model map\n", nodeName.c_str());
            }
        }
    }

    model->markDereferenced();
}

void ModelLoader::addToTree(ModelEntry *model)
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

    std::string description = yamlModel[base]["Description"].as<std::string>();
    std::string url = yamlModel[base]["URL"].as<std::string>();
    std::string doi = yamlModel[base]["DOI"].as<std::string>();

    Model::ModelType type = (base == "Model") ? Model::MODEL : Model::APPEARANCE_MODEL;

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
            auto propType = yamlProp["Type"].as<std::string>();
            auto propUnits = yamlProp["Units"].as<std::string>();
            auto propURL = yamlProp["URL"].as<std::string>();
            auto propDescription = yamlProp["Description"].as<std::string>();

            ModelProperty property(propName, propType,
                           propUnits, propURL,
                           propDescription);

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

                auto model = getModelFromPath(library, file.canonicalFilePath().toStdString());
                (*_modelEntryMap)[model->getUUID()] = model;
                showYaml(model->getModel());
            }
        }
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        dereference(it->second);
    }

    for (auto it = _modelEntryMap->begin(); it != _modelEntryMap->end(); it++) {
        addToTree(it->second);
    }

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
