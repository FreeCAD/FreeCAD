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

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QMessageBox>
#include <QOpenGLFunctions>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QWindow>

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <ranges>
#include <cstdlib>

#include "DlgVersionMigrator.h"
#include "SplitButton.h"

#include "ui_DlgVersionMigrator.h"

#include "../MainWindow.h"
#include <App/Application.h>
#include <App/ApplicationDirectories.h>

#include "QTextBrowser"


using namespace Gui::Dialog;
namespace fs = std::filesystem;

bool isCurrentVersionKnown()
{
    std::set<fs::path> paths = {
        App::Application::directories()->getUserAppDataDir(),
        App::Application::directories()->getUserConfigPath()
    };
    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    std::string currentVersionedDirName
        = App::ApplicationDirectories::versionStringForPath(major, minor);
    for (auto& path : paths) {
        if (App::Application::directories()->usingCurrentVersionConfig(path)) {
            return true;
        }
        fs::path markerPath = path;
        if (App::Application::directories()->isVersionedPath(path)) {
            markerPath = path.parent_path();
        }
        markerPath /= currentVersionedDirName + ".do_not_migrate";
        if (fs::exists(markerPath)) {
            return true;
        }
    }
    return false;
}

void markCurrentVersionAsDoNotMigrate()
{
    std::set<fs::path> paths = {
        App::Application::directories()->getUserAppDataDir(),
        App::Application::directories()->getUserConfigPath()
    };
    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    std::string currentVersionedDirName
        = App::ApplicationDirectories::versionStringForPath(major, minor);
    for (auto& path : paths) {
        if (App::Application::directories()->usingCurrentVersionConfig(path)) {
            // No action to take: the migration is done, so this call doesn't need to do anything
            continue;
        }
        fs::path markerPath = path;
        if (App::Application::directories()->isVersionedPath(path)) {
            markerPath = path.parent_path();
        }
        markerPath /= currentVersionedDirName + ".do_not_migrate";
        std::ofstream markerFile(markerPath);
        if (!markerFile.is_open()) {
            Base::Console().error(
                "Unable to open marker file %s\n",
                Base::FileInfo::pathToString(markerPath).c_str()
            );
            continue;
        }
        markerFile << "Migration to version " << currentVersionedDirName << " was declined. "
                   << "To request migration again, delete this file.";
        markerFile.close();
    }
}


DlgVersionMigrator::DlgVersionMigrator(MainWindow* mw)
    : QDialog(mw)
    , mainWindow(mw)
    , sizeCalculationWorkerThread(nullptr)
    , ui(std::make_unique<Ui_DlgVersionMigrator>())
{
    ui->setupUi(this);

    auto prefGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Migration"
    );

    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);

    auto programName = QString::fromStdString(App::Application::getExecutableName());

    // NOTE: All rich-text strings are generated programmatically so that translators don't have to
    // deal with the markup. The two strings in the middle of the dialog are set in the UI file.

    auto programNameString
        = tr("Welcome to %1 %2.%3").arg(programName, QString::number(major), QString::number(minor));
    auto welcomeString = QStringLiteral("<b>") + programNameString + QStringLiteral("</b>");

    auto calculatingSizeString = QStringLiteral("<b>") + tr("Calculating size…")
        + QStringLiteral("</b>");

    auto shareConfigurationString = QStringLiteral("<a href='#shareConfiguration'>")
        + tr("Share configuration between versions") + QStringLiteral("</a>");

    setWindowTitle(programNameString);
#ifdef Q_OS_MACOS
    // macOS does not show the window title on modals, so add the extra label
    ui->welcomeLabel->setText(welcomeString);
#else
    ui->welcomeLabel->hide();
#endif
    ui->sizeLabel->setText(calculatingSizeString);

    ui->copyButton->mainButton()->setDefault(true);
    ui->copyButton->mainButton()->setAutoDefault(true);

    ui->copyButton->mainButton()->setText(tr("Copy Configuration (Recommended)"));

    connect(ui->copyButton, &SplitButton::defaultClicked, this, &DlgVersionMigrator::migrate);
    connect(ui->helpButton, &QPushButton::clicked, this, &DlgVersionMigrator::help);

    // Set up the menu actions for the two hidden options
    connect(ui->copyButton->mainButton(), &QPushButton::clicked, this, &DlgVersionMigrator::migrate);

    auto* menu = ui->copyButton->menu();

    QAction* share = menu->addAction(tr("Share configuration with previous version"));
    QAction* reset = menu->addAction(tr("Use a new default configuration"));

    connect(share, &QAction::triggered, this, &DlgVersionMigrator::share);
    connect(reset, &QAction::triggered, this, &DlgVersionMigrator::freshStart);
}

DlgVersionMigrator::~DlgVersionMigrator() = default;

int DlgVersionMigrator::exec()
{
    // If the user is running a custom directory set, there is no migration to versioned directories
    if (App::Application::directories()->usingCustomDirectories()) {
        return 0;
    }
    if (!isCurrentVersionKnown()) {
        calculateMigrationSize();
        QDialog::exec();
        if (sizeCalculationWorkerThread && sizeCalculationWorkerThread->isRunning()) {
            sizeCalculationWorkerThread->quit();
        }
    }
    return 0;
}


class DirectorySizeCalculationWorker: public QObject
{
    Q_OBJECT

public:
    void run()
    {
        auto dir = App::Application::directories()->getUserAppDataDir();
        uintmax_t size = 0;
        auto thisThread = QThread::currentThread();
        for (auto& entry : fs::recursive_directory_iterator(dir)) {
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

class PathMigrationWorker: public QObject
{
    Q_OBJECT

public:
    void run()
    {
        try {
            App::GetApplication().GetUserParameter().SaveDocument();
            App::Application::directories()->migrateAllPaths(
                {App::Application::directories()->getUserAppDataDir(),
                 App::Application::directories()->getUserConfigPath()}
            );
            Q_EMIT(complete());
        }
        catch (const Base::Exception& e) {
            Base::Console().error("Error migrating configuration data: %s\n", e.what());
            Q_EMIT(failed());
        }
        catch (const std::exception& e) {
            Base::Console().error("Unrecognized error migrating configuration data: %s\n", e.what());
            Q_EMIT(failed());
        }
        catch (...) {
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

void DlgVersionMigrator::calculateMigrationSize()
{
    sizeCalculationWorkerThread = new QThread(mainWindow);
    auto* worker = new DirectorySizeCalculationWorker();
    worker->moveToThread(sizeCalculationWorkerThread);
    connect(sizeCalculationWorkerThread, &QThread::started, worker, &DirectorySizeCalculationWorker::run);

    connect(
        worker,
        &DirectorySizeCalculationWorker::sizeFound,
        this,
        &DlgVersionMigrator::showSizeOfMigration
    );
    connect(worker, &DirectorySizeCalculationWorker::finished, sizeCalculationWorkerThread, &QThread::quit);
    connect(worker, &DirectorySizeCalculationWorker::finished, worker, &QObject::deleteLater);
    connect(
        sizeCalculationWorkerThread,
        &QThread::finished,
        sizeCalculationWorkerThread,
        &QObject::deleteLater
    );

    sizeCalculationWorkerThread->start();
}

void DlgVersionMigrator::share()
{
    markCurrentVersionAsDoNotMigrate();
    if (sizeCalculationWorkerThread && sizeCalculationWorkerThread->isRunning()) {
        sizeCalculationWorkerThread->quit();
    }
    close();
}

void DlgVersionMigrator::showSizeOfMigration(uintmax_t size)
{
    auto sizeString = QLocale().formattedDataSize(static_cast<qint64>(size));
    auto sizeMessage = QStringLiteral("<b>")
        + QObject::tr("Estimated size of data to copy: %1").arg(sizeString) + QStringLiteral("</b>");
    ui->sizeLabel->setText(sizeMessage);
    sizeCalculationWorkerThread = nullptr;  // Deleted via a previously-configured deleteLater()
}

void DlgVersionMigrator::migrate()
{
    hide();
    auto* workerThread = new QThread(mainWindow);
    auto* worker = new PathMigrationWorker();
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &PathMigrationWorker::run);
    connect(worker, &PathMigrationWorker::finished, workerThread, &QThread::quit);
    connect(worker, &PathMigrationWorker::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    auto migrationRunning = new QMessageBox(this);
    migrationRunning->setWindowTitle(QObject::tr("Migrating"));
    migrationRunning->setText(QObject::tr("Migrating configuration data and addons…"));
    migrationRunning->setStandardButtons(QMessageBox::NoButton);
    connect(worker, &PathMigrationWorker::complete, migrationRunning, &QMessageBox::accept);
    connect(worker, &PathMigrationWorker::failed, migrationRunning, &QMessageBox::reject);

    workerThread->start();
    migrationRunning->exec();

    if (migrationRunning->result() == QDialog::Accepted) {
        restart(tr("Migration complete"));
    }
    else {
        QMessageBox::critical(
            mainWindow,
            QObject::tr("Migration failed"),
            QObject::tr("Migration failed. See the Report View for details.")
        );
    }
}


void DlgVersionMigrator::freshStart()
{
    // Create the versioned directories, but don't put anything in them
    std::set<fs::path> paths = {
        App::Application::directories()->getUserAppDataDir(),
        App::Application::directories()->getUserConfigPath()
    };
    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    std::string currentVersionedDirName
        = App::ApplicationDirectories::versionStringForPath(major, minor);
    for (auto& path : paths) {
        if (App::Application::directories()->usingCurrentVersionConfig(path)) {
            continue;
        }
        fs::path versionDir = path;
        if (App::Application::directories()->isVersionedPath(path)) {
            versionDir = path.parent_path();
        }
        versionDir /= currentVersionedDirName;
        if (fs::exists(versionDir)) {
            continue;
        }
        fs::create_directory(versionDir);
    }
    restart(tr("New default configuration created"));
}

void DlgVersionMigrator::help()
{
    auto helpPage = QStringLiteral("https://wiki.freecad.org/Version_migration");
    QDesktopServices::openUrl(QUrl(helpPage));
}

void DlgVersionMigrator::restart(const QString& message)
{
    App::GetApplication().GetUserParameter().SaveDocument();  // Flush to disk before restarting
    auto* restarting = new QMessageBox(this);
    restarting->setText(message + QObject::tr(" → Restarting…"));
    restarting->setWindowTitle(QObject::tr("Restarting"));
    restarting->setStandardButtons(QMessageBox::NoButton);
    auto closeNotice = [restarting]() {
        restarting->reject();
    };

    // Insert a short delay before restart so the user can see the success message and
    // knows it's a restart and not a crash...
    constexpr int delayRestartMillis {2000};
    QTimer::singleShot(delayRestartMillis, closeNotice);
    restarting->exec();

    connect(qApp, &QCoreApplication::aboutToQuit, [=] {
        if (getMainWindow()->close()) {
            auto args = QApplication::arguments();
            args.removeFirst();
            QProcess::startDetached(
                QApplication::applicationFilePath(),
                args,
                QApplication::applicationDirPath()
            );
        }
    });
    QCoreApplication::exit(0);
    _Exit(0);  // No really. Die.
}


#include "DlgVersionMigrator.moc"
