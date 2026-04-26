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
#include <filesystem>

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
            sizeCalculationWorkerThread->requestInterruption();
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
        std::error_code errorCode;
        auto iterator = fs::recursive_directory_iterator(
            dir,
            fs::directory_options::skip_permission_denied,
            errorCode
        );
        if (errorCode) {
            Q_EMIT(sizeFound(0));
            Q_EMIT(finished());
            return;
        }
        for (auto it = fs::begin(iterator); it != fs::end(iterator); it.increment(errorCode)) {
            if (errorCode) {
                errorCode.clear();
                continue;
            }
            if (thisThread->isInterruptionRequested()) {
                Q_EMIT(cancelled());
                Q_EMIT(finished());
                return;
            }
            if (it->is_regular_file(errorCode) && !errorCode) {
                auto fileSize = fs::file_size(it->path(), errorCode);
                if (!errorCode) {
                    size += fileSize;
                }
                errorCode.clear();
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

PathMigrationWorker::PathMigrationWorker(std::string configDir, std::string userAppDir, int major, int minor)
    : _configDir(std::move(configDir))
    , _userAppDir(std::move(userAppDir))
    , _major(major)
    , _minor(minor)
{}

void PathMigrationWorker::run()
{
    try {
        App::GetApplication().GetUserParameter().SaveDocument();
        auto result = App::Application::directories()->migrateAllPaths({_userAppDir, _configDir});
        replaceOccurrencesInPreferences();
        if (!result.failedPaths.empty()) {
            writeMigrationLog(result.failedPaths);
            QStringList skippedList;
            for (const auto& path : result.failedPaths) {
                skippedList.append(QString::fromStdString(Base::FileInfo::pathToString(path)));
            }
            Q_EMIT(completedWithWarnings(skippedList));
        }
        else {
            Q_EMIT(complete());
        }
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

void PathMigrationWorker::replaceOccurrencesInPreferences()
{
    std::filesystem::path prefPath = locateNewPreferences();
    std::map<std::string, std::string> replacements = {
        {_configDir, generateNewUserAppPathString(_configDir)},
        {_userAppDir, generateNewUserAppPathString(_userAppDir)}
    };

    try {
        std::ifstream prefFile(prefPath);
        std::string contents(
            (std::istreambuf_iterator<char>(prefFile)),
            std::istreambuf_iterator<char>()
        );

        for (const auto& [oldString, newString] : replacements) {
            replaceInContents(contents, oldString, newString);
        }

        std::ofstream newPrefFile(prefPath);
        newPrefFile << contents;
    }
    catch (const std::exception& e) {
        Base::Console().error("Error reading preferences file: %s\n", e.what());
    }
}

std::filesystem::path PathMigrationWorker::locateNewPreferences() const
{
    std::filesystem::path path(_configDir);
    if (path.filename().empty()) {
        // Handle the case where the path was constructed from a std::string with a trailing /
        path = path.parent_path();
    }
    fs::path newPath;

    if (App::Application::directories()->isVersionedPath(path)) {
        newPath = path.parent_path()
            / App::ApplicationDirectories::versionStringForPath(_major, _minor);
    }
    else {
        newPath = path / App::ApplicationDirectories::versionStringForPath(_major, _minor);
    }
    newPath /= "user.cfg";
    return newPath;
}

std::string PathMigrationWorker::generateNewUserAppPathString(const std::string& oldPath) const
{
    std::filesystem::path newPath = Base::FileInfo::stringToPath(oldPath);
    if (App::Application::directories()->isVersionedPath(newPath)) {
        newPath = newPath.parent_path();
    }
    newPath /= App::ApplicationDirectories::versionStringForPath(_major, _minor);
    std::string result = Base::FileInfo::pathToString(newPath);
    if (oldPath.back() == std::filesystem::path::preferred_separator) {
        result += std::filesystem::path::preferred_separator;
    }
    return result;
}

void PathMigrationWorker::replaceInContents(
    std::string& contents,
    const std::string& oldString,
    const std::string& newString
)
{
    if (oldString.empty()) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = contents.find(oldString, pos)) != std::string::npos) {
        contents.replace(pos, oldString.length(), newString);
        pos += newString.length();
    }
}

void PathMigrationWorker::writeMigrationLog(const std::vector<std::filesystem::path>& skippedPaths)
{
    auto logName = "migration-to-"
        + App::ApplicationDirectories::versionStringForPath(_major, _minor) + ".log";
    auto logPath = locateNewPreferences().parent_path() / logName;
    try {
        std::ofstream log(logPath);
        log << "Migration completed with " << skippedPaths.size() << " skipped file(s):\n\n";
        for (const auto& path : skippedPaths) {
            log << "  " << Base::FileInfo::pathToString(path) << "\n";
        }
        log << "\nSee the FreeCAD Report View for additional details.\n";
    }
    catch (const std::exception& e) {
        Base::Console().warning(
            "Migration: could not write log to '%s': %s\n",
            Base::FileInfo::pathToString(logPath).c_str(),
            e.what()
        );
    }
}

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
        sizeCalculationWorkerThread->requestInterruption();
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
    int major = std::stoi(App::Application::Config()["BuildVersionMajor"]);
    int minor = std::stoi(App::Application::Config()["BuildVersionMinor"]);
    auto* workerThread = new QThread(mainWindow);
    auto* worker = new PathMigrationWorker(
        App::Application::getUserConfigPath(),
        App::Application::getUserAppDataDir(),
        major,
        minor
    );
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::started, worker, &PathMigrationWorker::run);
    connect(worker, &PathMigrationWorker::finished, workerThread, &QThread::quit);
    connect(worker, &PathMigrationWorker::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    QStringList skippedFiles;
    auto migrationRunning = new QMessageBox(this);
    migrationRunning->setWindowTitle(QObject::tr("Migrating"));
    migrationRunning->setText(QObject::tr("Migrating configuration data and addons…"));
    migrationRunning->setStandardButtons(QMessageBox::NoButton);
    connect(worker, &PathMigrationWorker::complete, migrationRunning, &QMessageBox::accept);
    connect(
        worker,
        &PathMigrationWorker::completedWithWarnings,
        migrationRunning,
        [migrationRunning, &skippedFiles](QStringList paths) {
            skippedFiles = std::move(paths);
            migrationRunning->accept();
        }
    );
    connect(worker, &PathMigrationWorker::failed, migrationRunning, &QMessageBox::reject);

    workerThread->start();
    migrationRunning->exec();

    if (migrationRunning->result() == QDialog::Accepted) {
        if (!skippedFiles.isEmpty()) {
            // Some of these file paths might be very long, so truncate them to a reasonable
            // (arbitrary) length of 70 chars.
            constexpr int maxDisplayLength = 70;
            constexpr int prefixLength = maxDisplayLength / 3;
            constexpr int suffixLength = maxDisplayLength - prefixLength - 1;  // 1 for ellipsis
            QStringList truncated;
            for (const auto& path : skippedFiles) {
                if (path.length() > maxDisplayLength) {
                    truncated.append(
                        path.left(prefixLength) + QStringLiteral("…") + path.right(suffixLength)
                    );
                }
                else {
                    truncated.append(path);
                }
            }
            auto warning = new QMessageBox(mainWindow);
            warning->setIcon(QMessageBox::Warning);
            warning->setWindowTitle(QObject::tr("Migration completed with warnings"));
            auto logFileName = QStringLiteral("migration-to-")
                + QString::fromStdString(
                                   App::ApplicationDirectories::versionStringForPath(major, minor)
                )
                + QStringLiteral(".log");
            warning->setText(
                QObject::tr(
                    "%n file(s) could not be copied and were skipped. "
                    "A full list has been saved to %1 in your new configuration directory.",
                    "",
                    static_cast<int>(skippedFiles.size())
                )
                    .arg(logFileName)
            );
            warning->setDetailedText(truncated.join(QStringLiteral("\n")));
            warning->setStandardButtons(QMessageBox::Ok);
            warning->exec();
        }
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
