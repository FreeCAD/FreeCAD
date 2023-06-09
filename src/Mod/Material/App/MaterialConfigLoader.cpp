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

#include <fstream>
#include <App/Application.h>
#include <Base/Interpreter.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QString>

#include "Model.h"
#include "MaterialConfigLoader.h"


using namespace Materials;

MaterialConfigEntry::MaterialConfigEntry(const MaterialLibrary &library, const std::string &modelName, const QDir &dir, 
        const std::string &modelUuid, const YAML::Node &modelData):
    _library(library), _name(modelName), _directory(dir), _uuid(modelUuid), _model(modelData), _dereferenced(false)
{}

MaterialConfigEntry::~MaterialConfigEntry()
{}

MaterialConfigLoader::MaterialConfigLoader()
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialConfigLoader::~MaterialConfigLoader()
{}

bool MaterialConfigLoader::isConfigStyle(const std::string& path)
{
    std::ifstream infile(path);

    // Check the first 2 lines for a ";"
    for (int i = 0; i < 2; i++) {
        std::string line;
        if (!std::getline(infile, line))
            return false;
        if (line.at(0) != ';')
            return false;
    }

    return true;
}



#include "moc_MaterialConfigLoader.cpp"
