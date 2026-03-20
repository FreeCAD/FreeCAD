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

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QRunnable>
#include <QString>
#include <QStringList>

#include <memory>

namespace Start
{

class ThumbnailSourceSignals: public QObject
{
    Q_OBJECT
public:
Q_SIGNALS:

    void thumbnailAvailable(const QString& file, const QByteArray& data);
};

class ThumbnailSource: public QRunnable
{
public:
    explicit ThumbnailSource(QString file);

    // Don't make copies of a ThumbnailSource (it's probably running a process, what would it mean
    // to copy it?):
    ThumbnailSource(ThumbnailSource&) = delete;
    ThumbnailSource(ThumbnailSource&&) = delete;
    ThumbnailSource operator=(const ThumbnailSource&) = delete;
    ThumbnailSource operator=(ThumbnailSource&&) = delete;

    void run() override;

    ThumbnailSourceSignals* signals();

private:
    static void setupF3D();

    QString _file;
    QString _thumbnailPath;
    ThumbnailSourceSignals _signals;

    /// Gather together all of the f3d information protected by the mutex: data in this struct
    /// should be accessed only after a call to setupF3D() to ensure synchronization.
    static struct F3DInstallation
    {
        bool initialized {false};
        int major {0};
        int minor {0};
        QStringList baseArgs;
    } _f3d;
    static QMutex _mutex;
};

}  // namespace Start
