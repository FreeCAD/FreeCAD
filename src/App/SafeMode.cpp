/***************************************************************************
 *   Copyright (c) 2024 Benjamin Nauck <benjamin@nauck.se>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <QTemporaryDir>

#include "Application.h"
#include "FCConfig.h"

#include "SafeMode.h"

static QTemporaryDir* tempDir = nullptr;

static bool _createTemporaryBaseDir()
{
    tempDir = new QTemporaryDir();
    if (!tempDir->isValid()) {
        delete tempDir;
        tempDir = nullptr;
    }
    return tempDir;
}

static void _replaceDirs()
{
    auto& config = App::GetApplication().Config();

    auto const temp_base = tempDir->path().toStdString();
    auto const dirs = {
        "UserAppData",
        "UserConfigPath",
        "UserCachePath",
        "AppTempPath",
        "UserMacroPath",
        "UserHomePath",
    };

    for (auto const d : dirs) {
        auto const path = temp_base + PATHSEP + d + PATHSEP;
        auto const qpath = QString::fromStdString(path);
        QDir().mkpath(qpath);
        config[d] = path;
    }
}

void SafeMode::StartSafeMode()
{
    if (_createTemporaryBaseDir()) {
        _replaceDirs();
    }
}

bool SafeMode::SafeModeEnabled()
{
    return tempDir;
}

void SafeMode::Destruct()
{
    delete tempDir;
}
