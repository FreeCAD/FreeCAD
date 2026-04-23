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

#pragma once

#include <QtClassHelperMacros>
#include <QObject>
#include <QRunnable>
#include <QString>

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
    // Don't make copies of a ThumbnailSource (it's probably running a process, what would it mean
    // to copy it?):
    Q_DISABLE_COPY_MOVE(ThumbnailSource)

public:
    using Signals = ThumbnailSourceSignals;

    explicit ThumbnailSource(QString file);
    ~ThumbnailSource() override = default;

    void run() override;

    const Signals* signals() const
    {
        return &_signals;
    }

private:
    // Having a signal QObject as part of the QRunnable ensures signal connections
    // are properly cleaned up when the QRunnable gets destroyed as it finishes its work.
    Signals _signals;
    QString _file;
    QString _thumbnailPath;
};

}  // namespace Start
