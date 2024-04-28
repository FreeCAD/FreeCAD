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

#ifndef FREECAD_START_EXAMPLESMODEL_H
#define FREECAD_START_EXAMPLESMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <Base/Parameter.h>

#include "DisplayedFilesModel.h"
#include "../StartGlobal.h"

namespace Start
{

/// A model for displaying a list of files including a thumbnail or icon, plus various file
/// statistics.
class StartExport ExamplesModel: public DisplayedFilesModel
{
    Q_OBJECT
public:
    explicit ExamplesModel(QObject* parent = nullptr);

    void loadExamples();

private:
    QDir _examplesDirectory;
};

}  // namespace Start

#endif  // FREECAD_START_EXAMPLESMODEL_H
