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
#include <yaml-cpp/yaml.h>

#include "Model.h"


using namespace Material;

Model::Model()
{
    loadLibraries();
}

/*
 *  Destroys the object and frees any allocated resources
 */
Model::~Model()
{
    // no need to delete child widgets, Qt does it all for us
}

void Model::loadLibraries(void)
{
    Base::Console().Log("Model::loadLibraries()\n");

    Base::Console().Log("Model::loadLibraries() - finished\n");
}

std::list<std::string*>* Model::getModelLibraries()
// std::list<LibraryData *> *MaterialsEditor::getMaterialLibraries()
{
    // auto param =
    //     App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");
    // bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    // bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    // bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    // bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    // std::list<LibraryData *> *libraries = new std::list<LibraryData *>();
    // if (useBuiltInMaterials)
    // {
    //     QString resourceDir = QString::fromStdString(App::Application::getResourceDir() + "/Mod/Material/Resources/Materials");
    //     QDir *materialDir = new QDir(resourceDir);
    //     Base::Console().Log(materialDir->absolutePath().toStdString().c_str());
    //     Base::Console().Log("\n");
    //     auto libData = new LibraryData("System", materialDir, ":/icons/freecad.svg");
    //     libraries->push_back(libData);
    // }

    // if (useMatFromModules)
    // {
    //     auto moduleParam = App::GetApplication().GetParameterGroupByPath(
    //         "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
    //     for (auto &group : moduleParam->GetGroups()) {
    //         // auto module = moduleParam->GetGroup(group->GetGroupName());
    //         auto moduleName = group->GetGroupName();
    //         auto materialDir = group->GetASCII("ModuleDir", "");
    //         auto materialIcon = group->GetASCII("ModuleIcon", "");

    //         Base::Console().Log("Module ");
    //         Base::Console().Log(moduleName);
    //         Base::Console().Log("\n");

    //         Base::Console().Log("\tModuleDir ");
    //         Base::Console().Log(materialDir.c_str());
    //         Base::Console().Log("\n");

    //         Base::Console().Log("\tModuleIcon ");
    //         Base::Console().Log(materialIcon.c_str());
    //         Base::Console().Log("\n");

    //         QDir *dir = new QDir(QString::fromStdString(materialDir));
    //         auto libData = new LibraryData(moduleName, dir, materialIcon);
    //         libraries->push_back(libData);
    //     }
    // }

    // if (useMatFromConfigDir)
    // {
    //     QString resourceDir = QString::fromStdString(App::Application::getUserAppDataDir() + "/Material");
    //     QDir *materialDir = new QDir(resourceDir);
    //     Base::Console().Log(materialDir->absolutePath().toStdString().c_str());
    //     Base::Console().Log("\n");
    //     auto libData = new LibraryData("User", materialDir, ":/icons/preferences-general.svg");
    //     libraries->push_back(libData);
    // }

    // if (useMatFromCustomDir)
    // {
    //     QString resourceDir = QString::fromStdString(param->GetASCII("CustomMaterialsDir", ""));
    //     QDir *materialDir = new QDir(resourceDir);
    //     if (materialDir->exists())
    //     {
    //         auto libData = new LibraryData("Custom", materialDir, ":/icons/user.svg");
    //         libraries->push_back(libData);
    //     }
    // }

    // return libraries;

    return nullptr;
}


#include "moc_Model.cpp"
