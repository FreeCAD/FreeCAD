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

#include "RecentFilesModel.h"
#include <App/Application.h>
#include <App/ProjectFile.h>

using namespace Start;

RecentFilesModel::RecentFilesModel(QObject* parent)
    : DisplayedFilesModel(parent)
{
    _parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/RecentFiles"
    );
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

void RecentFilesModel::recentFileAdded(const QString& filename)
{
    Q_UNUSED(filename)
    loadRecentFiles();
}

void RecentFilesModel::removeFile(const QString& filename)
{
    auto numRows {static_cast<std::size_t>(_parameterGroup->GetInt("RecentFiles", 0))};

    // Collect the entries that are being kept, in their current order, skipping the one
    // being removed.
    std::vector<std::string> keptPaths;
    keptPaths.reserve(numRows);
    for (std::size_t i = 0; i < numRows; ++i) {
        auto entry = fmt::format("MRU{}", i);
        auto path = _parameterGroup->GetASCII(entry.c_str(), "");
        if (QString::fromStdString(path) != filename) {
            keptPaths.push_back(path);
        }
    }

    if (keptPaths.size() == numRows) {
        // The requested file wasn't found in the list; nothing to do.
        return;
    }

    // Rewrite the MRU entries so that they are contiguous again, then update the count.
    for (std::size_t i = 0; i < numRows; ++i) {
        auto entry = fmt::format("MRU{}", i);
        if (i < keptPaths.size()) {
            _parameterGroup->SetASCII(entry.c_str(), keptPaths[i].c_str());
        }
        else {
            _parameterGroup->RemoveASCII(entry.c_str());
        }
    }
    _parameterGroup->SetInt("RecentFiles", static_cast<long>(keptPaths.size()));

    loadRecentFiles();
}
