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
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#endif

#include "DisplayedFilesModel.h"
#include <App/Application.h>
#include <App/ProjectFile.h>

using namespace Start;


namespace
{

std::string humanReadableSize(unsigned int bytes)
{
    static const std::vector<std::string> siPrefix {
        "b",
        "kb",
        "Mb",
        "Gb",
        "Tb",
        "Pb",
        "Eb"  // I think it's safe to stop here (for the time being)...
    };
    size_t base = 0;
    double inUnits = bytes;
    constexpr double siFactor {1000.0};
    while (inUnits > siFactor && base < siPrefix.size() - 1) {
        ++base;
        inUnits /= siFactor;
    }
    if (base == 0) {
        // Don't include a decimal point for bytes
        return fmt::format("{:.0f} {}", inUnits, siPrefix[base]);
    }
    // For all others, include one digit after the decimal place
    return fmt::format("{:.1f} {}", inUnits, siPrefix[base]);
}

FileStats fileInfoFromFreeCADFile(const std::string& path)
{
    App::ProjectFile proj(path);
    proj.loadDocument();
    auto metadata = proj.getMetadata();
    FileStats result;
    result.insert(std::make_pair(DisplayedFilesModelRoles::author, metadata.createdBy));
    result.insert(
        std::make_pair(DisplayedFilesModelRoles::modifiedTime, metadata.lastModifiedDate));
    result.insert(std::make_pair(DisplayedFilesModelRoles::creationTime, metadata.creationDate));
    result.insert(std::make_pair(DisplayedFilesModelRoles::company, metadata.company));
    result.insert(std::make_pair(DisplayedFilesModelRoles::license, metadata.license));
    result.insert(std::make_pair(DisplayedFilesModelRoles::description, metadata.comment));
    return result;
}

/// Load the thumbnail image data (if any) that is stored in an FCStd file.
/// \returns The image bytes, or an empty QByteArray (if no thumbnail was stored)
QByteArray loadFCStdThumbnail(const std::string& pathToFCStdFile)
{
    App::ProjectFile proj(pathToFCStdFile);
    if (proj.loadDocument()) {
        try {
            std::string thumbnailFile = proj.extractInputFile("thumbnails/Thumbnail.png");
            if (!thumbnailFile.empty()) {
                auto inputFile = QFile(QString::fromStdString(thumbnailFile));
                inputFile.open(QIODevice::OpenModeFlag::ReadOnly);
                return inputFile.readAll();
            }
        }
        catch (...) {
        }
    }
    return {};
}

FileStats getFileInfo(const std::string& path)
{
    FileStats result;
    Base::FileInfo file(path);
    if (file.hasExtension("FCStd")) {
        result = fileInfoFromFreeCADFile(path);
    }
    else {
        file.lastModified();
    }
    result.insert(std::make_pair(DisplayedFilesModelRoles::path, path));
    result.insert(std::make_pair(DisplayedFilesModelRoles::size, humanReadableSize(file.size())));
    result.insert(std::make_pair(DisplayedFilesModelRoles::baseName, file.fileName()));
    return result;
}
}  // namespace

DisplayedFilesModel::DisplayedFilesModel(QObject* parent)
    : QAbstractListModel(parent)
{}


int DisplayedFilesModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(_fileInfoCache.size());
}

QVariant DisplayedFilesModel::data(const QModelIndex& index, int roleAsInt) const
{
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(_fileInfoCache.size())) {
        return {};
    }
    auto mapEntry = _fileInfoCache.at(row);
    auto role = static_cast<DisplayedFilesModelRoles>(roleAsInt);
    switch (role) {
        case DisplayedFilesModelRoles::author:  // NOLINT(bugprone-branch-clone)
            [[fallthrough]];
        case DisplayedFilesModelRoles::baseName:
            [[fallthrough]];
        case DisplayedFilesModelRoles::company:
            [[fallthrough]];
        case DisplayedFilesModelRoles::creationTime:
            [[fallthrough]];
        case DisplayedFilesModelRoles::description:
            [[fallthrough]];
        case DisplayedFilesModelRoles::license:
            [[fallthrough]];
        case DisplayedFilesModelRoles::modifiedTime:
            [[fallthrough]];
        case DisplayedFilesModelRoles::path:
            [[fallthrough]];
        case DisplayedFilesModelRoles::size:
            if (mapEntry.find(role) != mapEntry.end()) {
                return QString::fromStdString(mapEntry.at(role));
            }
            else {
                return {};
            }
        case DisplayedFilesModelRoles::image: {
            auto path = QString::fromStdString(mapEntry.at(DisplayedFilesModelRoles::path));
            if (_imageCache.contains(path)) {
                return _imageCache[path];
            }
            break;
        }
        default:
            break;
    }
    switch (roleAsInt) {
        case Qt::ItemDataRole::ToolTipRole:
            return QString::fromStdString(mapEntry.at(DisplayedFilesModelRoles::path));
    }
    return {};
}

bool freecadCanOpen(const QString& extension)
{
    auto importTypes = App::GetApplication().getImportTypes();
    return std::find(importTypes.begin(), importTypes.end(), extension.toStdString())
        != importTypes.end();
}

void DisplayedFilesModel::addFile(const QString& filePath)
{
    QFileInfo qfi(filePath);
    if (!qfi.isReadable()) {
        return;
    }
    if (!freecadCanOpen(qfi.suffix())) {
        return;
    }
    _fileInfoCache.emplace_back(getFileInfo(filePath.toStdString()));
    if (qfi.suffix() == QLatin1String("FCStd")) {
        auto thumbnail = loadFCStdThumbnail(filePath.toStdString());
        if (!thumbnail.isEmpty()) {
            _imageCache.insert(filePath, thumbnail);
        }
    }
}

void DisplayedFilesModel::clear()
{
    _fileInfoCache.clear();
}

QHash<int, QByteArray> DisplayedFilesModel::roleNames() const
{
    static QHash<int, QByteArray> nameMap {
        std::make_pair(int(DisplayedFilesModelRoles::author), "author"),
        std::make_pair(int(DisplayedFilesModelRoles::baseName), "baseName"),
        std::make_pair(int(DisplayedFilesModelRoles::company), "company"),
        std::make_pair(int(DisplayedFilesModelRoles::creationTime), "creationTime"),
        std::make_pair(int(DisplayedFilesModelRoles::description), "description"),
        std::make_pair(int(DisplayedFilesModelRoles::image), "image"),
        std::make_pair(int(DisplayedFilesModelRoles::license), "license"),
        std::make_pair(int(DisplayedFilesModelRoles::modifiedTime), "modifiedTime"),
        std::make_pair(int(DisplayedFilesModelRoles::path), "path"),
        std::make_pair(int(DisplayedFilesModelRoles::size), "size"),
    };
    return nameMap;
}
