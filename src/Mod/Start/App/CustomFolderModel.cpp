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

using namespace Start;


CustomFolderModel::CustomFolderModel(QObject* parent)
    : DisplayedFilesModel(parent)
{
    std::string defaultPath = App::GetApplication().Config()["UserHomePath"];
    Base::Reference<ParameterGrp> parameterGroup;

    parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");

    _customFolderDirectory = QDir(
        QString::fromUtf8(parameterGroup->GetASCII("CustomFolder", defaultPath.c_str()).c_str()));
}

void CustomFolderModel::loadAdditional()
{
    beginResetModel();
    clear();
    if (!_customFolderDirectory.isReadable()) {
        Base::Console().Warning(
            "BaseApp/Preferences/Mod/Start/CustomFolder: cannot read custom folder %s\n",
            _customFolderDirectory.absolutePath().toStdString().c_str());
    }
    auto entries = _customFolderDirectory.entryList(QDir::Filter::Files | QDir::Filter::Readable,
                                                    QDir::SortFlag::Name);
    for (const auto& entry : entries) {
        addFile(_customFolderDirectory.filePath(entry));
    }
    endResetModel();
}
