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
#include <yaml-cpp/yaml.h>

#include "Model.h"


using namespace Material;

Model::Model(const std::string baseName, const std::string &modelName, const QDir &dir, const std::string &modelUuid, const std::string &modelData) :
    base(baseName), name(modelName), directory(dir), uuid(modelUuid), model(modelData), dereferenced(false)
{}

Model::~Model()
{}


LibraryEntry::LibraryEntry(const std::string &libraryName, const QDir &dir, const std::string &icon)
{}

LibraryEntry::~LibraryEntry()
{}

ModelManager *ModelManager::manager = nullptr;
std::list<LibraryEntry*> *ModelManager::libraries = nullptr;

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

void ModelManager::loadLibrary(const LibraryEntry &library)
{
    Base::Console().Log("ModelManager::loadLibrary(");
    Base::Console().Log(library.getName().c_str());
    Base::Console().Log(",");
    Base::Console().Log(library.getDirectory().absolutePath().toStdString().c_str());
    Base::Console().Log(",");
    Base::Console().Log(library.getIconPath().c_str());
    Base::Console().Log(")\n");

    QDirIterator it(library.getDirectory(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        auto pathname = it.next();
        QFileInfo file(pathname);
        if (file.isFile()) {
            if (file.suffix().toStdString() == "yml") {
                Base::Console().Log("\t");
                Base::Console().Log(file.canonicalFilePath().toStdString().c_str());
                Base::Console().Log("\n");

                std::string libraryName = file.baseName().toStdString();

                // auto model = new LibraryEntry(
                //     libraryName, file.canonicalFilePath().toStdString(), ":/icons/freecad.svg");
                // addModel(model);
            }
        }
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
                if (dir.exists())
                    libraries->push_back(libData);
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
        if (materialDir.exists())
            libraries->push_back(libData);
    }

    if (useMatFromCustomDir)
    {
        QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
        QDir materialDir(resourceDir);
        Base::Console().Log(materialDir.absolutePath().toStdString().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryEntry("Custom", materialDir, ":/icons/user.svg");
        if (materialDir.exists()) 
            libraries->push_back(libData);
    }

    return libraries;
}


#include "moc_Model.cpp"
