// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 The FreeCAD project association AISBL              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QOpenGLFunctions>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QWindow>

#include <set>
#include <string>
#include <ranges>
#endif

#include "VersionMigrator.h"

#include "MainWindow.h"
#include <App/Application.h>
#include <App/ApplicationDirectories.h>


using namespace Gui;
namespace fs = std::filesystem;


uintmax_t calculateDirectorySize(const fs::path &dir) {
    uintmax_t size = 0;
    for (auto &entry: fs::recursive_directory_iterator(dir)) {
        if (fs::is_regular_file(entry.status())) {
            size += fs::file_size(entry.path());
        }
    }
    return size;
}

std::set<std::string> getKnownVersions() {
    auto splitCommas = [](const std::string &input) {
        std::set<std::string> result;
        std::stringstream ss(input);
        std::string token;
        while (std::getline(ss, token, ',')) {
            result.insert(token);
        }
        return result;
    };

    auto prefGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Migration");

    // Split our comma-separated list of already-migrated-to version directories into a set for easy
    // searching
    std::string offeredToMigrateToVersionedConfig =
            prefGroup->GetASCII("OfferedToMigrateToVersionedConfig", "");
    std::set<std::string> knownVersions;
    if (!offeredToMigrateToVersionedConfig.empty()) {
        knownVersions = splitCommas(offeredToMigrateToVersionedConfig);
    }
    return knownVersions;
}

void setKnownVersions(const std::set<std::string> &knownVersions) {
    auto joinCommas = [](const std::set<std::string> &s) {
        std::ostringstream oss;
        for (auto it = s.begin(); it != s.end(); ++it) {
            if (it != s.begin()) {
                oss << ',';
            }
            oss << *it;
        }
        return oss.str();
    };

    auto prefGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Migration");
    prefGroup->SetASCII("OfferedToMigrateToVersionedConfig", joinCommas(knownVersions));
}

void markCurrentVersionAsKnown() {
    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    std::string currentVersionedDirName = App::ApplicationDirectories::versionStringForPath(major, minor);
    std::set<std::string> knownVersions = getKnownVersions();
    knownVersions.insert(currentVersionedDirName);
    setKnownVersions(knownVersions);
}

VersionMigrator::VersionMigrator(MainWindow *mw) : QObject(mw), mainWindow(mw) {
}

void VersionMigrator::execute() {
    // If the user is running a custom directory set, there is no migration to versioned directories
    if (App::Application::directories()->usingCustomDirectories()) {
        return;
    }

    auto prefGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Migration");

    // Split our comma-separated list of already-migrated-to version directories into a set for easy
    // searching
    std::string offeredToMigrateToVersionedConfig =
            prefGroup->GetASCII("OfferedToMigrateToVersionedConfig", "");
    std::set<std::string> knownVersions = getKnownVersions();

    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    std::string currentVersionedDirName = App::ApplicationDirectories::versionStringForPath(major, minor);
    if (!knownVersions.contains(currentVersionedDirName)
        && !App::Application::directories()->usingCurrentVersionConfig(
            App::Application::directories()->getUserAppDataDir())) {
        auto programName = QString::fromStdString(App::Application::getExecutableName());
        auto result = QMessageBox::question(
            mainWindow,
            QObject::tr("Welcome to %1 v%2.%3").arg(programName, QString::number(major), QString::number(minor)),
            QObject::tr("Welcome to %1 v%2.%3\n\n").arg(programName, QString::number(major), QString::number(minor))
            + QObject::tr("Configuration data and addons from previous program version found. "
                "Migrate the configuration to a new directory for this version? Answering 'No' will "
                "continue to use the old directory. 'Yes' will copy it."),
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            confirmMigration();
        } else {
            // Don't ask again for this version
            markCurrentVersionAsKnown();
        }
    }
}

class DirectorySizeCalculationWorker : public QObject {
    Q_OBJECT

public:
    void run() {
        auto dir = App::Application::directories()->getUserAppDataDir();
        uintmax_t size = 0;
        auto thisThread = QThread::currentThread();
        for (auto &entry: fs::recursive_directory_iterator(dir)) {
            if (thisThread->isInterruptionRequested()) {
                Q_EMIT(cancelled());
                Q_EMIT(finished());
                return;
            }
            if (fs::is_regular_file(entry.status())) {
                size += fs::file_size(entry.path());
            }
        }
        Q_EMIT(sizeFound(size));
        Q_EMIT(finished());
    }

Q_SIGNALS:
    void finished();

    void sizeFound(uintmax_t _t1);

    void cancelled();
};

class PathMigrationWorker : public QObject {
    Q_OBJECT

public:
    void run() {
        try {
            App::GetApplication().GetUserParameter().SaveDocument();
            App::Application::directories()->migrateAllPaths(
                {
                    App::Application::directories()->getUserAppDataDir(),
                    App::Application::directories()->getUserConfigPath()
                });
            Q_EMIT(complete());
        } catch (const Base::Exception &e) {
            Base::Console().error("Error migrating configuration data: %s\n", e.what());
            Q_EMIT(failed());
        } catch (const std::exception &e) {
            Base::Console().error("Unrecognized error migrating configuration data: %s\n", e.what());
            Q_EMIT(failed());
        } catch (...) {
            Base::Console().error("Error migrating configuration data\n");
            Q_EMIT(failed());
        }
        Q_EMIT(finished());
    }

Q_SIGNALS:
    void finished();

    void complete();

    void failed();
};

void VersionMigrator::confirmMigration() {
    auto *workerThread = new QThread(mainWindow);
    auto *worker = new DirectorySizeCalculationWorker();
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &DirectorySizeCalculationWorker::run);

    connect(worker, &DirectorySizeCalculationWorker::sizeFound, this, &VersionMigrator::showSizeOfMigration);
    connect(worker, &DirectorySizeCalculationWorker::finished, workerThread, &QThread::quit);
    connect(worker, &DirectorySizeCalculationWorker::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    auto calculatingSize = new QMessageBox(mainWindow);
    calculatingSize->setWindowTitle(QObject::tr("Calculating size"));
    calculatingSize->setText(QObject::tr("Calculating directory size…"));
    calculatingSize->setStandardButtons(QMessageBox::Cancel);
    connect(worker, &DirectorySizeCalculationWorker::sizeFound, calculatingSize, &QMessageBox::accept);
    connect(calculatingSize, &QMessageBox::rejected, workerThread, &QThread::requestInterruption);
    connect(calculatingSize, &QMessageBox::rejected, this, &VersionMigrator::migrationCancelled);

    workerThread->start();
    calculatingSize->exec();
}

void VersionMigrator::migrationCancelled() const {
    auto message = QObject::tr(
        "Migration cancelled. Ask again on next restart?");
    auto result = QMessageBox::question(
        mainWindow,
        QObject::tr("Migration cancelled"),
        message,
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::No) {
        markCurrentVersionAsKnown();
    }
}

void VersionMigrator::showSizeOfMigration(uintmax_t size) {
    auto sizeString = QLocale().formattedDataSize(static_cast<qint64>(size));
    auto result = QMessageBox::question(
        mainWindow,
        QObject::tr("Migration Size"),
        QObject::tr("Migrating will copy %1 into a versioned subdirectory. Continue?").arg(sizeString),
        QMessageBox::Yes | QMessageBox::No
    );
    if (result == QMessageBox::Yes) {
        migrateToCurrentVersion();
    } else {
        migrationCancelled();
    }
}

void VersionMigrator::migrateToCurrentVersion() {
    auto oldKnownVersions = getKnownVersions();
    markCurrentVersionAsKnown();  // This MUST be done before the migration, or it won't get remembered
    auto *workerThread = new QThread(mainWindow);
    auto *worker = new PathMigrationWorker();
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &PathMigrationWorker::run);
    connect(worker, &PathMigrationWorker::finished, workerThread, &QThread::quit);
    connect(worker, &PathMigrationWorker::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    auto migrationRunning = new QMessageBox(mainWindow);
    migrationRunning->setWindowTitle(QObject::tr("Migrating"));
    migrationRunning->setText(QObject::tr("Migrating configuration data and addons..."));
    migrationRunning->setStandardButtons(QMessageBox::NoButton);
    connect(worker, &PathMigrationWorker::complete, migrationRunning, &QMessageBox::accept);
    connect(worker, &PathMigrationWorker::failed, migrationRunning, &QMessageBox::reject);

    workerThread->start();
    migrationRunning->exec();

    if (migrationRunning->result() == QDialog::Accepted) {
        App::GetApplication().GetUserParameter().SaveDocument(); // Flush to disk before restarting
        auto *restarting = new QMessageBox(mainWindow);
        restarting->setText(
            QObject::tr("Migration complete. Restarting..."));
        restarting->setWindowTitle(QObject::tr("Restarting"));
        restarting->setStandardButtons(QMessageBox::NoButton);
        auto closeNotice = [restarting]() {
            restarting->reject();
        };

        // Insert a short delay before restart so the user can see the success message, and
        // knows it's a restart and not a crash...
        constexpr int delayRestartMillis{2000};
        QTimer::singleShot(delayRestartMillis, closeNotice);
        restarting->exec();

        connect(qApp, &QCoreApplication::aboutToQuit, [=] {
            if (getMainWindow()->close()) {
                auto args = QApplication::arguments();
                args.removeFirst();
                QProcess::startDetached(QApplication::applicationFilePath(),
                                        args,
                                        QApplication::applicationDirPath());
            }
        });
        QCoreApplication::exit(0);
        _exit(0); // No really. Die.
    } else {
        setKnownVersions(oldKnownVersions);  // Reset, we didn't migrate after all
        QMessageBox::critical(mainWindow, QObject::tr("Migration failed"),
                              QObject::tr("Migration failed. See the Report View for details."));
    }
}

#include "VersionMigrator.moc"
