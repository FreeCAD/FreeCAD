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
#include <QDir>
#endif

#include "CustomFolderModel.h"
#include <App/Application.h>
#include <string>
#include <vector>

using namespace Start;


CustomFolderModel::CustomFolderModel(QObject* parent)
    : DisplayedFilesModel(parent)
{

    Base::Reference<ParameterGrp> parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");

    _customFolderDirectory =
        QDir(QString::fromStdString(parameterGroup->GetASCII("CustomFolder", "")));
}

/// If the custom folder path contains multiple paths separated by ';;', split them into individual
/// paths. This is used to allow the user to specify multiple paths in the preferences dialog.
/// We use ';;' as a separator because it is not a valid character in a file path.
std::vector<std::string> CustomFolderModel::_splitPaths()
{
    std::vector<std::string> paths;
    std::string pathspec = _customFolderDirectory.absolutePath().toStdString();
    std::string delimiter = ";;";
    size_t pos = 0;
    std::string path;

    while ((pos = pathspec.find(delimiter)) != std::string::npos) {
        path = pathspec.substr(0, pos);
        paths.push_back(path);
        pathspec.erase(0, pos + delimiter.length());
    }

    paths.push_back(pathspec);

    return paths;
}

void CustomFolderModel::loadCustomFolder()
{
    beginResetModel();
    clear();
    auto paths = _splitPaths();

    for (const auto& path : paths) {
        QDir customFolderDirectory(QString::fromStdString(path));
        if (!customFolderDirectory.exists()) {
            Base::Console().Warning(
                "BaseApp/Preferences/Mod/Start/CustomFolder: custom folder %s does not exist\n",
                customFolderDirectory.absolutePath().toStdString().c_str());
            continue;
        }
        if (!customFolderDirectory.isReadable()) {
            Base::Console().Warning(
                "BaseApp/Preferences/Mod/Start/CustomFolder: cannot read custom folder %s\n",
                customFolderDirectory.absolutePath().toStdString().c_str());
            continue;
        }
        auto entries = customFolderDirectory.entryList(QDir::Filter::Files | QDir::Filter::Readable,
                                                       QDir::SortFlag::Name);
        for (const auto& entry : entries) {
            addFile(customFolderDirectory.filePath(entry));
        }
    }

    endResetModel();
}
