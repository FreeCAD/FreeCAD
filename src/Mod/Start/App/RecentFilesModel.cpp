// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "RecentFilesModel.h"
#include <App/Application.h>
#include <App/ProjectFile.h>

using namespace Start;

RecentFilesModel::RecentFilesModel(QObject* parent)
    : DisplayedFilesModel(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/RecentFiles");
}

void RecentFilesModel::loadRecentFiles()
{
    beginResetModel();
    clear();
    auto numRows {_parameterGroup->GetInt("RecentFiles", 0)};
    for (int i = 0; i < numRows; ++i) {
        auto entry = fmt::format("MRU{}", i);
        auto path = _parameterGroup->GetASCII(entry.c_str(), "");
        addFile(QString::fromStdString(path));
    }
    endResetModel();
}
