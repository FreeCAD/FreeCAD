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
#include "ModelManager.h"


using namespace Material;

LibraryEntry::LibraryEntry(const std::string &libraryName, const QDir &dir, const std::string &icon):
    name(libraryName), directory(dir), iconPath(icon)
{}

LibraryEntry::~LibraryEntry()
{}

ModelManager *ModelManager::manager = nullptr;
std::list<LibraryEntry*> *ModelManager::libraries = nullptr;
std::map<std::string, Model*>* ModelManager::modelMap = nullptr;

ModelManager::ModelManager()
{
    loadLibraries();
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelManager::~ModelManager()
{
    // // Delete the library
    // while(!libraries->empty()) {
    //     delete libraries->back();
    //     libraries->pop_back();
    // }
    // libraries = nullptr;
}

ModelManager *ModelManager::getManager()
{
    if (manager == nullptr)
        manager = new ModelManager();
    return manager;
}

void ModelManager::addModel(LibraryEntry *model)
{
    if (libraries == nullptr)
        libraries = new std::list<LibraryEntry*>();
    libraries->push_back(model);
}

Model *ModelManager::getModelFromPath(const std::string &path) const
{
    YAML::Node yamlroot = YAML::LoadFile(path);
    std::string base = "Model";
    if (yamlroot["Model"]) {
        Base::Console().Log("Model type: Model\n");
    }
    if (yamlroot["AppearanceModel"]) {
        base = "AppearanceModel";
        Base::Console().Log("Model type: AppearanceModel\n");
    }

    const std::string uuid = yamlroot[base]["UUID"].as<std::string>();
    const std::string name = yamlroot[base]["Name"].as<std::string>();

    QDir modelDir(QString::fromStdString(path));
    Model *model = new Model(base, name, modelDir, uuid, yamlroot);

    return model;
}

void ModelManager::showYaml(const YAML::Node &yaml) const
{
    std::stringstream out;

    out << yaml;
    std::string logData = out.str();
    Base::Console().Log("%s\n", logData.c_str());
}

void ModelManager::dereference(Model *parent, const Model *child)
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


void ModelManager::dereference(Model *model)
{
    // Avoid recursion
    if (model->getDereferenced())
        return;

    auto yamlModel = model->getModel();
    auto base = model->getBase();
    if (yamlModel[base]["Inherits"]) {
        auto inherits = yamlModel[base]["Inherits"];
        Base::Console().Log("Model '%s' inherits from:\n", model->getName().c_str());
        for(auto it = inherits.begin(); it != inherits.end(); it++) {
            // auto nodeName = it->first.as<std::string>();
            std::string nodeName = (*it)["UUID"].as<std::string>();
            Base::Console().Log("\t'%s'\n", nodeName.c_str());

            // This requires that all models have already been loaded undereferenced
            Base::Console().Log("\tModel map size %d\n", modelMap->size());
            try {
                const Model *child = (*modelMap)[nodeName];
                dereference(model, child);
            }
            catch (const std::out_of_range& oor) {
                Base::Console().Log("Unable to find '%s' in model map\n", nodeName.c_str());
            }
        }
    }

    model->markDereferenced();
}

void ModelManager::loadLibrary(const LibraryEntry &library)
{
    Base::Console().Log("ModelManager::loadLibrary(%s)\n", library.getIconPath().c_str());

    if (modelMap == nullptr)
        modelMap = new std::map<std::string, Model*>();

    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "yml") {
                Base::Console().Log("\t%s\n", file.canonicalFilePath().toStdString().c_str());

                std::string libraryName = file.baseName().toStdString();

                auto model = getModelFromPath(file.canonicalFilePath().toStdString());
                (*modelMap)[model->getUUID()] = model;
                showYaml(model->getModel());
            }
        }
    }

    Base::Console().Log("ModelManager::loadLibrary() - dereference\n");
    for (auto it = modelMap->begin(); it != modelMap->end(); it++) {
        dereference(it->second);
    }

    Base::Console().Log("ModelManager::loadLibrary() - finished\n");
}

void ModelManager::loadLibraries(void)
{
    Base::Console().Log("ModelManager::loadLibraries()\n");

    std::list<LibraryEntry*>* libraries = getModelLibraries();
    if (libraries) {
        for (auto it = libraries->begin(); it != libraries->end(); it++) {
            loadLibrary(**it);
        }
    }

    Base::Console().Log("ModelManager::loadLibraries() - finished\n");
}

void ModelManager::showLibEntry(const std::string &checkpoint, const QDir &dir, const LibraryEntry &entry) const
{
    Base::Console().Log("ModelManager::showLibEntry(");
    Base::Console().Log(checkpoint.c_str());
    Base::Console().Log(": (");
    Base::Console().Log(entry.getName().c_str());
    Base::Console().Log(",");
    Base::Console().Log(entry.getDirectory().absolutePath().toStdString().c_str());
    Base::Console().Log(",");
    Base::Console().Log(entry.getIconPath().c_str());
    Base::Console().Log("))\n");
    Base::Console().Log("\t");
    Base::Console().Log(dir.absolutePath().toStdString().c_str());
    Base::Console().Log("\n");
}

std::list<LibraryEntry *> *ModelManager::getModelLibraries()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    std::list<LibraryEntry*> *libraries = new std::list<LibraryEntry*>();
    if (useBuiltInMaterials)
    {
        QString resourceDir = QString::fromStdString(App::Application::getResourceDir() + "/Mod/Material/Resources/Models");
        QDir materialDir(resourceDir);
        Base::Console().Log(materialDir.absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryEntry("System", materialDir, ":/icons/freecad.svg");
        // showLibEntry("System", materialDir, *libData);
        Base::Console().Log(materialDir.absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        libraries->push_back(libData);
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

            Base::Console().Log("Module ");
            Base::Console().Log(moduleName);
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleModelDir ");
            Base::Console().Log(modelDir.c_str());
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleIcon ");
            Base::Console().Log(modelIcon.c_str());
            Base::Console().Log("\n");

            if (modelDir.length() > 0) {
                QDir dir(QString::fromStdString(modelDir));
                auto libData = new LibraryEntry(moduleName, dir, modelIcon);
                if (dir.exists()) {
                    // showLibEntry("Module", dir, *libData);
                    libraries->push_back(libData);
                }
            }
        }
    }

    if (useMatFromConfigDir)
    {
        QString resourceDir = QString::fromStdString(App::Application::getUserAppDataDir() + "/Models");
        QDir materialDir(resourceDir);
        Base::Console().Log(materialDir.absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryEntry("User", materialDir, ":/icons/preferences-general.svg");
        if (materialDir.exists()) {
            // showLibEntry("User", materialDir, *libData);
            libraries->push_back(libData);
        }
    }

    if (useMatFromCustomDir)
    {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        QDir materialDir(resourceDir);
        Base::Console().Log(materialDir.absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryEntry("Custom", materialDir, ":/icons/user.svg");
        if (materialDir.exists()) {
            // showLibEntry("Custom", materialDir, *libData);
            libraries->push_back(libData);
        }
    }

    return libraries;
}


#include "moc_ModelManager.cpp"
