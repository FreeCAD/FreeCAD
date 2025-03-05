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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <QUrl>
#endif

#include "FileUtilities.h"


QString Start::getThumbnailsImage()
{
    return QLatin1String("thumbnails/Thumbnail.png");
}

QString Start::getThumbnailsName()
{
#if defined(Q_OS_LINUX)
    return QLatin1String("thumbnails/normal");
#else
    return QLatin1String("FreeCADStartThumbnails");
#endif
}

QDir Start::getThumbnailsParentDir()
{
    return {QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)};
}

QString Start::getThumbnailsDir()
{
    const QDir dir = getThumbnailsParentDir();
    return dir.absoluteFilePath(getThumbnailsName());
}

void Start::createThumbnailsDir()
{
    const QString name = getThumbnailsName();
    if (const QDir dir(getThumbnailsParentDir()); !dir.exists(name)) {
        [[maybe_unused]] bool result = dir.mkpath(name);
    }
}

QString Start::getMD5Hash(const QString& path)
{
    // Use MD5 hash as specified here:
    // https://specifications.freedesktop.org/thumbnail-spec/0.8.0/thumbsave.html
    QUrl url(path);
    url.setScheme(QStringLiteral("file"));
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(url.toEncoded());
    const QByteArray ba = hash.result().toHex();
    return QString::fromLatin1(ba);
}

QString Start::getUniquePNG(const QString& path)
{
    const QDir dir = getThumbnailsDir();
    const QString md5 = getMD5Hash(path) + QLatin1String(".png");
    return dir.absoluteFilePath(md5);
}

bool Start::useCachedPNG(const QString& image, const QString& project)
{
    const QFileInfo f1(image);
    const QFileInfo f2(project);
    if (!f1.exists()) {
        return false;
    }
    if (!f2.exists()) {
        return false;
    }

    return f1.lastModified() > f2.lastModified();
}
