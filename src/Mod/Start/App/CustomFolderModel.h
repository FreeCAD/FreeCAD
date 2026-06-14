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

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <Base/Parameter.h>

#include "DisplayedFilesModel.h"
#include "../StartGlobal.h"

namespace Start
{

/// A model for displaying a list of files including a thumbnail or icon, plus various file
/// statistics.
class StartExport CustomFolderModel: public DisplayedFilesModel
{
    Q_OBJECT
public:
    explicit CustomFolderModel(QObject* parent = nullptr);

    void loadCustomFolder();

private:
    QString _customFolderPathSpec;
    bool _showOnlyFCStd;  // Show only FreeCAD files
};

}  // namespace Start
