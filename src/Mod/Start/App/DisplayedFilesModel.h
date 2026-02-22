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
#include <Base/Parameter.h>

#include "../StartGlobal.h"

namespace Start
{

enum class DisplayedFilesModelRoles
{
    baseName = Qt::UserRole + 1,
    image,
    size,
    author,
    creationTime,
    modifiedTime,
    description,
    company,
    license,
    path
};

using FileStats = std::map<DisplayedFilesModelRoles, std::string>;

/// A model for displaying a list of files including a thumbnail or icon, plus various file
/// statistics.
class StartExport DisplayedFilesModel: public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DisplayedFilesModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void addFile(const QString& filePath);

    void clear();

protected:
    /// For communication with QML, define the text version of each role name defined in the
    /// DisplayedFilesModelRoles enumeration
    QHash<int, QByteArray> roleNames() const override;

    /// Process a new thumbnail produces by some sort of worker thread
    void processNewThumbnail(const QString& file, const QByteArray& thumbnail);

private:
    std::vector<FileStats> _fileInfoCache;
    QMap<QString, QByteArray> _imageCache;
};

}  // namespace Start
