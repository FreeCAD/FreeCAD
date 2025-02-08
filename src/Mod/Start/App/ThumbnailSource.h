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

#ifndef FREECAD_THUMBNAIL_SOURCE_H
#define FREECAD_THUMBNAIL_SOURCE_H

#include <QRunnable>
#include <QString>
#include <QObject>
#include <QMutex>
#include <QStringList>

#include <memory>

class QProcess;

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
    ~ThumbnailSource() override;

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
    ThumbnailSourceSignals _signals;
    std::unique_ptr<QProcess> _process;

    static bool _f3dInitialized;
    static int _f3dMajor;
    static int _f3dMinor;
    static QStringList _f3dBaseArgs;
    static QMutex _mutex;
};

}  // namespace Start

#endif  // FREECAD_THUMBNAIL_SOURCE_H
