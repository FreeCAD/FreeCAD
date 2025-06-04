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
#include <QFile>
#include <QMetaObject>
#include <QMutexLocker>
#include <QObject>
#include <QProcess>
#include <QTimer>
#endif

#include "ThumbnailSource.h"

#include <QThread>

#include "FileUtilities.h"

#include <App/Application.h>

using namespace Start;

ThumbnailSource::F3DInstallation ThumbnailSource::_f3d {};
QMutex ThumbnailSource::_mutex;

ThumbnailSource::ThumbnailSource(QString file)
    : _file(std::move(file))
{}

ThumbnailSourceSignals* ThumbnailSource::signals()
{
    return &_signals;
}

void ThumbnailSource::run()
{
    _thumbnailPath = getPathToCachedThumbnail(_file);
    if (!useCachedThumbnail(_thumbnailPath, _file)) {
        // Go through the mutex to ensure data is not stale.
        // Contention on the lock is diminished because of first checking the cache.
        setupF3D();
        if (_f3d.major < 2) {
            return;
        }
        const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Start");
        const auto f3d = QString::fromUtf8(hGrp->GetASCII("f3d", "f3d").c_str());
        QStringList args(_f3d.baseArgs);
        args << QLatin1String("--output=") + _thumbnailPath << _file;

        QProcess process;
        Base::Console().log("Creating thumbnail for %s...\n", _file.toStdString());
        process.start(f3d, args);
        if (!process.waitForFinished()) {
            process.kill();
            Base::Console().log("Creating thumbnail for %s timed out\n", _file.toStdString());
            return;
        }
        if (process.exitStatus() == QProcess::CrashExit) {
            Base::Console().log("Creating thumbnail for %s crashed\n", _file.toStdString());
            return;
        }
        if (process.exitCode() != 0) {
            Base::Console().log("Creating thumbnail for %s failed\n", _file.toStdString());
            return;
        }
        Base::Console().log("Creating thumbnail for %s succeeded, wrote to %s\n",
                            _file.toStdString(),
                            _thumbnailPath.toStdString());
    }
    if (QFile thumbnailFile(_thumbnailPath); thumbnailFile.exists()) {
        thumbnailFile.open(QIODevice::OpenModeFlag::ReadOnly);
        Q_EMIT _signals.thumbnailAvailable(_file, thumbnailFile.readAll());
    }
}

std::tuple<int, int, int> extractF3DVersion(const QString& stdoutString)
{
    int major {0};
    int minor {0};
    int patch {0};
    for (auto lines = stdoutString.split(QLatin1Char('\n')); const auto& line : lines) {
        if (line.startsWith(QLatin1String("Version: "))) {
            const auto substring = line.mid(8);
            if (auto split = substring.split(QLatin1Char('.')); split.size() >= 3) {
                try {
                    major = split[0].toInt();
                    minor = split[1].toInt();
                    patch = split[2].toInt();
                }
                catch (...) {
                    Base::Console().log(
                        "Could not determine F3D version, disabling thumbnail generation\n");
                }
            }
            break;
        }
    }
    return std::make_tuple(major, minor, patch);
}

QStringList getF3DOptions(const QString& f3d)
{
    // F3D is under active development, and the available options change with some regularity.
    // Rather than hardcode per version, just check the ones we care about.
    QStringList optionsToTest {
        QStringLiteral("--load-plugins=occt"),
        QStringLiteral("--config=thumbnail"),
        QStringLiteral("--verbose=quiet"),
        QStringLiteral("--resolution=256,256"),
        QStringLiteral("--filename=0"),
        QStringLiteral("--grid=0"),
        QStringLiteral("--axis=0"),
        QStringLiteral("--no-background"),
        QStringLiteral("--max-size=100")  // Max input file size in MB
    };
    QStringList goodOptions;
    for (const auto& option : optionsToTest) {
        QStringList args;
        args << option << QStringLiteral("--no-render");
        QProcess process;
        process.start(f3d, args);
        if (!process.waitForFinished()) {
            process.kill();
            continue;
        }
        auto stderrAsBytes = process.readAllStandardError();
        if (auto stderrAsString = QString::fromUtf8(stderrAsBytes);
            !stderrAsString.contains(QLatin1String("Unknown option"))) {
            goodOptions.append(option);
        }
    }
    return goodOptions;
}

void ThumbnailSource::setupF3D()
{
    QMutexLocker locker(&_mutex);
    if (_f3d.initialized) {
        return;
    }

    // This method makes repeated blocking calls to f3d (both directly, the call below, and
    // indirectly, by calling getF3DOptions). By holding the mutex above, it ensures that these
    // calls complete before any process can attempt to make a real call to f3d to create thumbnail
    // data. ThumbnailSource is run in its own thread, so blocking here is appropriate and will not
    // affect any other part of the program.

    _f3d.initialized = true;  // Set immediately so we can use early-return below
    const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    const auto f3d = QString::fromUtf8(hGrp->GetASCII("f3d", "f3d").c_str());
    const QStringList args {QLatin1String("--version")};
    QProcess process;
    process.start(f3d, args);
    if (!process.waitForFinished()) {
        process.kill();
    }
    if (process.exitCode() != 0) {
        return;
    }
    const QByteArray stdoutBytes = process.readAllStandardOutput();
    const auto stdoutString = QString::fromUtf8(stdoutBytes);
    const auto version = extractF3DVersion(stdoutString);
    _f3d.major = std::get<0>(version);
    _f3d.minor = std::get<1>(version);
    if (_f3d.major >= 2) {
        _f3d.baseArgs = getF3DOptions(f3d);
    }
    Base::Console().log("Running f3d version %d.%d\n", _f3d.major, _f3d.minor);
}
