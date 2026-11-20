// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Benjamin Nauck
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


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
        auto const path = temp_base + "/" + d + "/";
        auto const qpath = QDir::cleanPath(QString::fromStdString(path));
        QDir().mkpath(qpath);
        config[d] = QDir::toNativeSeparators(qpath).toStdString();
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
