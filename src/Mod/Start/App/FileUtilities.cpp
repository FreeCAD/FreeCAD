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
#include <QFileInfo>
#include <QString>
#include <QTimeZone>
#include <QUrl>
#include <fmt/format.h>
#endif

#include "FileUtilities.h"
#include <Base/TimeInfo.h>


void Start::createThumbnailsDir()
{
    if (!thumbnailsParentDir.exists(defaultThumbnailName)) {
        thumbnailsParentDir.mkpath(defaultThumbnailName);
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

QString Start::getPathToCachedThumbnail(const QString& path)
{
    const QString md5 = getMD5Hash(path) + QLatin1String(".png");
    return thumbnailsDir.absoluteFilePath(md5);
}

bool Start::useCachedThumbnail(const QString& image, const QString& project)
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

std::string Start::humanReadableSize(uint64_t bytes)
{
    if (bytes == 0) {
        return "0 B";
    }

    // uint64_t can't express numbers higher than the EB range (< 20 EB)
    constexpr std::array units {"B", "kB", "MB", "GB", "TB", "PB", "EB"};
    constexpr double unitFactor = 1000.0;

    const double logBaseFactor = std::log10(unitFactor);  // change to constexpr on c++26
    const auto unitIndex = static_cast<size_t>(std::log10(bytes) / logBaseFactor);

    // unitIndex can't be out of range because uint64_t can't express numbers higher than the EB
    // range
    const auto unit = units.at(unitIndex);

    const double scaledValue = static_cast<double>(bytes) / std::pow(unitFactor, unitIndex);

    const bool isByteUnit = unitIndex == 0;
    const size_t precision = isByteUnit ? 0 : 1;
    return fmt::format("{:.{}f} {}", scaledValue, precision, unit);
}

std::string Start::getLastModifiedAsString(const Base::FileInfo& file)
{
    Base::TimeInfo lastModified = file.lastModified();
    return QDateTime::fromSecsSinceEpoch(lastModified.getTime_t())
        .toTimeZone(QTimeZone::utc())
        .toString(Qt::ISODate)
        .toStdString();
}
