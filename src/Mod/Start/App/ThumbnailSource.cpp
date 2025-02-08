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
#include <QProcess>
#include <QMutexLocker>
#endif

#include "ThumbnailSource.h"

#include <QThread>

#include "FileUtilities.h"

#include <App/Application.h>

using namespace Start;

bool ThumbnailSource::_f3dInitialized = false;
int ThumbnailSource::_f3dMajor = 0;
int ThumbnailSource::_f3dMinor = 0;
QStringList ThumbnailSource::_f3dBaseArgs;
QMutex ThumbnailSource::_mutex;

ThumbnailSource::ThumbnailSource(QString file)
    : _file(std::move(file))
{}

ThumbnailSource::~ThumbnailSource()
{
    if (_process && _process->state() == QProcess::Running) {
        _process->kill();
        _process->waitForFinished();
    }
}

ThumbnailSourceSignals* ThumbnailSource::signals()
{
    return &_signals;
}

void ThumbnailSource::run()
{
    const QString thumbnailPath = getUniquePNG(_file);
    if (!useCachedPNG(thumbnailPath, _file)) {
        setupF3D();
        if (_f3dMajor < 2) {
            return;
        }
        const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Start");
        const auto f3d = QString::fromUtf8(hGrp->GetASCII("f3d", "f3d").c_str());
        QStringList args(_f3dBaseArgs);
        args << QLatin1String("--output=") + thumbnailPath << _file;

        _process = std::make_unique<QProcess>();
        Base::Console().Log("Creating thumbnail for %s...\n", _file.toStdString());
        _process->start(f3d, args);
        constexpr int checkEveryMs {50};
        while (!_process->waitForFinished(checkEveryMs)) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                _process->kill();
                break;
            }
        }
        if (_process->exitCode() != 0) {
            Base::Console().Log("Creating thumbnail for %s failed\n", _file.toStdString());
            return;
        }
        Base::Console().Log("Creating thumbnail for %s succeeded, wrote to %s\n",
                            _file.toStdString(),
                            thumbnailPath.toStdString());
    }

    if (QFile thumbnailFile(thumbnailPath); thumbnailFile.exists()) {
        thumbnailFile.open(QIODevice::OpenModeFlag::ReadOnly);
        Q_EMIT _signals.thumbnailAvailable(_file, thumbnailFile.readAll());
    }
}

namespace
{
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
                    Base::Console().Log(
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
        QStringLiteral("--no-background"),
        QStringLiteral("--max-size=100")  // Max input file size in MB
    };
    QStringList goodOptions;
    for (const auto& option : optionsToTest) {
        QStringList args;
        args << option << QStringLiteral("--no-render");
        QProcess process;
        process.start(f3d, args);
        process.waitForFinished();
        auto stderrAsBytes = process.readAllStandardError();
        if (auto stderrAsString = QString::fromUtf8(stderrAsBytes);
            !stderrAsString.contains(QLatin1String("Unknown option"))) {
            goodOptions.append(option);
        }
    }
    return goodOptions;
}
}  // namespace

void ThumbnailSource::setupF3D()
{
    QMutexLocker locker(&_mutex);
    if (_f3dInitialized) {
        return;
    }

    _f3dInitialized = true;  // Set immediately so we can use early-return below
    const ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start");
    const auto f3d = QString::fromUtf8(hGrp->GetASCII("f3d", "f3d").c_str());
    const QStringList args {QLatin1String("--version")};
    QProcess process;
    process.start(f3d, args);
    process.waitForFinished();
    if (process.exitCode() != 0) {
        return;
    }
    const QByteArray stdoutBytes = process.readAllStandardOutput();
    const auto stdoutString = QString::fromUtf8(stdoutBytes);
    const auto version = extractF3DVersion(stdoutString);
    _f3dMajor = std::get<0>(version);
    _f3dMinor = std::get<1>(version);
    if (_f3dMajor >= 2) {
        _f3dBaseArgs = getF3DOptions(f3d);
    }
}
