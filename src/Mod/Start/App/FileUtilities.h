// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 The FreeCAD Project Association AISBL               *
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

#include "Base/FileInfo.h"
#include "Mod/Start/StartGlobal.h"

#include <qglobal.h>
#include <QDir>
#include <QStandardPaths>

class QString;

namespace Start
{

const QLatin1String defaultThumbnailPath("thumbnails/Thumbnail.png");

const QLatin1String defaultThumbnailName
#if defined(Q_OS_LINUX)
    ("thumbnails/normal");
#else
    ("FreeCADStartThumbnails");
#endif

const QDir thumbnailsParentDir {QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)};

const QDir thumbnailsDir {thumbnailsParentDir.absoluteFilePath(defaultThumbnailName)};

StartExport void createThumbnailsDir();

StartExport QString getMD5Hash(const QString& path);

StartExport QString getPathToCachedThumbnail(const QString& path);

StartExport bool useCachedThumbnail(const QString& image, const QString& project);

StartExport std::string humanReadableSize(std::uint64_t bytes);

StartExport std::string getLastModifiedAsString(const Base::FileInfo& file);

}  // namespace Start
