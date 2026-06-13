// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 The FreeCAD Project Association AISBL               *
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

#include "FcstdInfoSource.h"

#ifdef __cpp_lib_spanstream
# include <spanstream>
#else
# include <ostream>
#endif

#include <Base/Console.h>
#include <Base/Stream.h>

#include <App/ProjectFile.h>

#include "FileUtilities.h"


using namespace Start;

/// Load the thumbnail image data (if any) that is stored in an FCStd file.
/// \returns The image bytes, or an empty QByteArray (if no thumbnail was stored)
static QByteArray loadFCStdThumbnail(const App::ProjectFile& proj, const QString& filePath)
{
    try {
        const QString pathToCachedThumbnail = getPathToCachedThumbnail(filePath);
        if (useCachedThumbnail(pathToCachedThumbnail, filePath)) {
            if (auto inputFile = QFile(pathToCachedThumbnail);
                inputFile.exists() && inputFile.open(QIODevice::OpenModeFlag::ReadOnly)) {
                return inputFile.readAll();
            }
        }
        else {
            const auto pathToThumbnail = QString(defaultThumbnailPath).toStdString();
            if (proj.containsFile(pathToThumbnail)) {
                createThumbnailsDir();

                // Read the thumbnail into a buffer
                const auto dataSize = proj.sizeOfFile(pathToThumbnail);
                QByteArray data(dataSize, Qt::Uninitialized);
#ifdef __cpp_lib_spanstream
                std::spanstream dataStream({data.data(), size_t(data.size())});
#else
                Base::BufferStreambuf dataStreambuf({data.data(), size_t(data.size())});
                std::ostream dataStream(&dataStreambuf);
#endif
                proj.readInputFileDirect(pathToThumbnail, dataStream);

                // Save that buffer to the thumbnail cache
                const Base::FileInfo thumbnailFileInfo(pathToCachedThumbnail.toStdString());
                Base::ofstream thumbnailFileStream(thumbnailFileInfo, std::ios::out | std::ios::binary);
                thumbnailFileStream.write(data.data(), data.size());
                thumbnailFileStream.close();

                return data;
            }
        }
    }
    catch (...) {
        Base::Console().log("Failed to load thumbnail for %s", filePath.toStdString());
    }
    return {};
}

static FileStats getProjectFileInfo(const App::ProjectFile& proj)
{
    FileStats result;
    auto metadata = proj.getMetadata();
    result.insert(std::make_pair(DisplayedFilesModelRoles::author, metadata.createdBy));
    result.insert(std::make_pair(DisplayedFilesModelRoles::modifiedTime, metadata.lastModifiedDate));
    result.insert(std::make_pair(DisplayedFilesModelRoles::creationTime, metadata.creationDate));
    result.insert(std::make_pair(DisplayedFilesModelRoles::company, metadata.company));
    result.insert(std::make_pair(DisplayedFilesModelRoles::license, metadata.license));
    result.insert(std::make_pair(DisplayedFilesModelRoles::description, metadata.comment));
    return result;
}

FcstdInfoSource::FcstdInfoSource(QString filePath)
    : _filePath(std::move(filePath))
{}

void FcstdInfoSource::run()
{
    const std::string stdFilePath(_filePath.toStdString());
    App::ProjectFile proj(stdFilePath);
    proj.loadDocument();
    auto fileStats = getProjectFileInfo(proj);
    auto thumbnail = loadFCStdThumbnail(proj, _filePath);
    Q_EMIT _signals.infoAvailable(_filePath, fileStats, thumbnail);
}
