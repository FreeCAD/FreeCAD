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

#include "Materials.h"


using namespace Material;

/* TRANSLATOR Material::Materials */

LibraryData::LibraryData()
{}

LibraryData::LibraryData(const std::string &libraryName, const fs::path &dir, const std::string &icon):
    name(libraryName), directory(dir), iconPath(icon)
{}

LibraryData::~LibraryData()
{
    // delete directory;
}

Materials::Materials()
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
Materials::~Materials()
{
    // no need to delete child widgets, Qt does it all for us
}

bool Materials::isCard(const fs::path &p)
{
    if (!fs::is_regular_file(p))
        return false;
    // check file extension
    if (p.extension() == ".FCMat")
        return true;
    return false;
}

std::list<LibraryData *> *Materials::getMaterialLibraries()
{
    auto param =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");
    bool useBuiltInMaterials = param->GetBool("UseBuiltInMaterials", true);
    bool useMatFromModules = param->GetBool("UseMaterialsFromWorkbenches", true);
    bool useMatFromConfigDir = param->GetBool("UseMaterialsFromConfigDir", true);
    bool useMatFromCustomDir = param->GetBool("UseMaterialsFromCustomDir", true);

    std::list<LibraryData *> *libraries = new std::list<LibraryData *>();
    if (useBuiltInMaterials)
    {
        fs::path resourceDir = App::Application::getResourceDir() + "/Mod/Material/Resources/Materials";
        Base::Console().Log(resourceDir.string().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryData("System", resourceDir, ":/icons/freecad.svg");
        libraries->push_back(libData);
    }

    if (useMatFromModules)
    {
        auto moduleParam = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules");
        for (auto &group : moduleParam->GetGroups()) {
            // auto module = moduleParam->GetGroup(group->GetGroupName());
            auto moduleName = group->GetGroupName();
            auto materialDir = group->GetASCII("ModuleDir", "");
            auto materialIcon = group->GetASCII("ModuleIcon", "");

            Base::Console().Log("Module ");
            Base::Console().Log(moduleName);
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleDir ");
            Base::Console().Log(materialDir.c_str());
            Base::Console().Log("\n");

            Base::Console().Log("\tModuleIcon ");
            Base::Console().Log(materialIcon.c_str());
            Base::Console().Log("\n");

            auto libData = new LibraryData(moduleName, materialDir, materialIcon);
            libraries->push_back(libData);
        }
    }

    if (useMatFromConfigDir)
    {
        fs::path resourceDir = App::Application::getUserAppDataDir() + "/Material";
        Base::Console().Log(resourceDir.string().c_str());
        Base::Console().Log("\n");
        auto libData = new LibraryData("User", resourceDir, ":/icons/preferences-general.svg");
        libraries->push_back(libData);
    }

    if (useMatFromCustomDir)
    {
        fs::path resourceDir = param->GetASCII("CustomMaterialsDir", "");
        if (fs::exists(resourceDir))
        {
            auto libData = new LibraryData("Custom", resourceDir, ":/icons/user.svg");
            libraries->push_back(libData);
        }
    }

    return libraries;
}


#include "moc_Materials.cpp"
