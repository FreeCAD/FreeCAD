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
#include <boost/algorithm/string/predicate.hpp>
#include <QByteArray>
#include <QFileInfo>
#include <QProcess>
#include <QTimeZone>
#include <QThreadPool>
#include <QUrl>
#endif

#include "DisplayedFilesModel.h"


#include "FileUtilities.h"
#include "ThumbnailSource.h"
#include <App/Application.h>
#include <App/ProjectFile.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Stream.h>


using namespace Start;


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
QByteArray loadFCStdThumbnail(const QString& pathToFCStdFile)
{
    if (App::ProjectFile proj(pathToFCStdFile.toStdString()); proj.loadDocument()) {
        try {
            const QString pathToCachedThumbnail = getPathToCachedThumbnail(pathToFCStdFile);
            if (!useCachedThumbnail(pathToCachedThumbnail, pathToFCStdFile)) {
                static const QString pathToThumbnail = defaultThumbnailPath;
                if (proj.containsFile(pathToThumbnail.toStdString())) {
                    createThumbnailsDir();
                    const Base::FileInfo fi(pathToCachedThumbnail.toStdString());
                    Base::ofstream stream(fi, std::ios::out | std::ios::binary);
                    proj.readInputFileDirect(pathToThumbnail.toStdString(), stream);
                    stream.close();
                }
            }

            if (auto inputFile = QFile(pathToCachedThumbnail); inputFile.exists()) {
                inputFile.open(QIODevice::OpenModeFlag::ReadOnly);
                return inputFile.readAll();
            }
        }
        catch (...) {
            Base::Console().log("Failed to load thumbnail for %s\n", pathToFCStdFile.toStdString());
        }
    }
    return {};
}

FileStats getFileInfo(const std::string& path)
{
    FileStats result;
    const Base::FileInfo file(path);
    if (file.hasExtension("FCStd")) {
        result = fileInfoFromFreeCADFile(path);
    }
    else {
        result.insert(
            std::make_pair(DisplayedFilesModelRoles::modifiedTime, getLastModifiedAsString(file)));
    }
    result.insert(std::make_pair(DisplayedFilesModelRoles::path, path));
    result.insert(std::make_pair(DisplayedFilesModelRoles::size, humanReadableSize(file.size())));
    result.insert(std::make_pair(DisplayedFilesModelRoles::baseName, file.fileName()));
    return result;
}

bool freecadCanOpen(const QString& extension)
{
    std::string ext = extension.toStdString();
    auto importTypes = App::GetApplication().getImportTypes();
    return std::ranges::find_if(importTypes,
                                [&ext](const auto& item) {
                                    return boost::iequals(item, ext);
                                })
        != importTypes.end();
}

DisplayedFilesModel::DisplayedFilesModel(QObject* parent)
    : QAbstractListModel(parent)
{}


int DisplayedFilesModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(_fileInfoCache.size());
}

QVariant DisplayedFilesModel::data(const QModelIndex& index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= static_cast<int>(_fileInfoCache.size())) {
        return {};
    }
    const auto mapEntry = _fileInfoCache.at(row);
    switch (const auto roleAsType = static_cast<DisplayedFilesModelRoles>(role)) {
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
            if (mapEntry.contains(roleAsType)) {
                return QString::fromStdString(mapEntry.at(roleAsType));
            }
            break;
        case DisplayedFilesModelRoles::image: {
            if (const auto path =
                    QString::fromStdString(mapEntry.at(DisplayedFilesModelRoles::path));
                _imageCache.contains(path)) {
                return _imageCache[path];
            }
            break;
        }
        default:
            break;
    }
    switch (role) {
        case Qt::ItemDataRole::ToolTipRole:
            return QString::fromStdString(mapEntry.at(DisplayedFilesModelRoles::path));
        default:
            // No other role gets handled
            break;
    }
    return {};
}

void DisplayedFilesModel::addFile(const QString& filePath)
{
    const QFileInfo qfi(filePath);
    if (!qfi.isReadable()) {
        return;
    }

    if (!freecadCanOpen(qfi.suffix())) {
        return;
    }

    _fileInfoCache.emplace_back(getFileInfo(filePath.toStdString()));
    const auto lowercaseExtension = qfi.suffix().toLower();
    const QStringList ignoredExtensions {QLatin1String("fcmacro"),
                                         QLatin1String("py"),
                                         QLatin1String("pyi"),
                                         QLatin1String("csv"),
                                         QLatin1String("txt")};
    if (lowercaseExtension == QLatin1String("fcstd")) {
        if (const auto thumbnail = loadFCStdThumbnail(filePath); !thumbnail.isEmpty()) {
            _imageCache.insert(filePath, thumbnail);
        }
    }
    else if (ignoredExtensions.contains(lowercaseExtension)) {
        // Don't try to generate a thumbnail for things like this: FreeCAD can read them, but
        // there's not much point in showing anything besides a generic icon
    }
    else {
        const auto runner = new ThumbnailSource(filePath);
        connect(runner->signals(),
                &ThumbnailSourceSignals::thumbnailAvailable,
                this,
                &DisplayedFilesModel::processNewThumbnail);
        QThreadPool::globalInstance()->start(runner);
    }
}

void DisplayedFilesModel::clear()
{
    _fileInfoCache.clear();
}

QHash<int, QByteArray> DisplayedFilesModel::roleNames() const
{
    static QHash<int, QByteArray> nameMap {
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::author), "author"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::baseName), "baseName"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::company), "company"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::creationTime), "creationTime"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::description), "description"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::image), "image"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::license), "license"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::modifiedTime), "modifiedTime"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::path), "path"),
        std::make_pair(static_cast<int>(DisplayedFilesModelRoles::size), "size"),
    };
    return nameMap;
}

void DisplayedFilesModel::processNewThumbnail(const QString& file, const QByteArray& thumbnail)
{
    if (!thumbnail.isEmpty()) {
        _imageCache.insert(file, thumbnail);

        // Figure out the index of this file...
        auto it = std::ranges::find_if(_fileInfoCache, [file](const FileStats& row) {
            auto pathIt = row.find(DisplayedFilesModelRoles::path);
            return pathIt != row.end() && pathIt->second == file.toStdString();
        });
        if (it != _fileInfoCache.end()) {
            std::size_t index = std::distance(_fileInfoCache.begin(), it);
            QModelIndex qmi = createIndex(index, 0);
            Q_EMIT(dataChanged(qmi, qmi, {static_cast<int>(DisplayedFilesModelRoles::image)}));
        }
        else {
            Base::Console().log("Unrecognized path %s\n", file.toStdString());
        }
    }
}
