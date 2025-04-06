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
#include <QStringList>

using namespace Start;


CustomFolderModel::CustomFolderModel(QObject* parent)
    : DisplayedFilesModel(parent)
{

    Base::Reference<ParameterGrp> parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");

    _customFolderPathSpec = QString::fromStdString(parameterGroup->GetASCII("CustomFolder", ""));

    _showOnlyFCStd = parameterGroup->GetBool("ShowOnlyFCStd", false);
}

/// If the custom folder path contains multiple paths separated by ';;', split them into individual
/// paths. This is used to allow the user to specify multiple paths in the preferences dialog.
/// We use ';;' as a separator because ';' is a valid character in a file path (e.g. NTFS on
/// Windows).
void CustomFolderModel::loadCustomFolder()
{
    beginResetModel();
    clear();
    auto pathDelimiter = QStringLiteral(";;");
    auto paths = _customFolderPathSpec.split(pathDelimiter, Qt::SkipEmptyParts);

    for (const auto& path : paths) {
        QDir customFolderDirectory(path);
        if (!customFolderDirectory.exists()) {
            Base::Console().warning(
                "BaseApp/Preferences/Mod/Start/CustomFolder: custom folder %s does not exist\n",
                customFolderDirectory.absolutePath().toStdString().c_str());
            continue;
        }
        if (!customFolderDirectory.isReadable()) {
            Base::Console().warning(
                "BaseApp/Preferences/Mod/Start/CustomFolder: cannot read custom folder %s\n",
                customFolderDirectory.absolutePath().toStdString().c_str());
            continue;
        }
        if (_showOnlyFCStd) {
            customFolderDirectory.setNameFilters(QStringList() << QStringLiteral("*.FCStd"));
        }

        auto entries = customFolderDirectory.entryList(QDir::Filter::Files | QDir::Filter::Readable,
                                                       QDir::SortFlag::Name);
        for (const auto& entry : entries) {
            addFile(customFolderDirectory.filePath(entry));
        }
    }

    endResetModel();
}
