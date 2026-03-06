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

#pragma once

#include <FCGlobal.h>
#include <cstdint>
#include <memory>
#include <QDialog>


namespace Gui
{

class MainWindow;

namespace Dialog
{

class GuiExport DlgVersionMigrator final: public QDialog
{
    Q_OBJECT

public:
    explicit DlgVersionMigrator(MainWindow* mw);
    ~DlgVersionMigrator() override;
    Q_DISABLE_COPY_MOVE(DlgVersionMigrator)

    int exec() override;

protected Q_SLOTS:

    void calculateMigrationSize();  // Async -> this starts the process and immediately returns
    void showSizeOfMigration(uintmax_t size);
    void migrate();
    void share();
    void freshStart();
    void help();

private:
    MainWindow* mainWindow;
    QThread* sizeCalculationWorkerThread;
    std::unique_ptr<class Ui_DlgVersionMigrator> ui;

    void restart(const QString& message);
};


class GuiExport PathMigrationWorker: public QObject
{
    Q_OBJECT

public:
    PathMigrationWorker(std::string configDir, std::string userAppDir, int major, int minor);
    void run();

Q_SIGNALS:
    void finished();
    void complete();
    void failed();

protected:
    /**
     * @brief Find any occurrence of the original config and userAppDir paths in the new copy of the
     * config file and replace them with updated versions.
     */
    void replaceOccurrencesInPreferences();

    /**
     * @brief Locate the new user config file
     *
     * After it's been moved, this method figures out the path to the new user.cfg file. It does not
     * verify the existence of the file, just determines where it *should* be.
     *
     * @return The path to the new version of user.cfg.
     */
    std::filesystem::path locateNewPreferences() const;

    /**
     * @brief Given an old path, figure out what the new versioned one would be
     *
     * @param oldPath  The old path
     * @return An equivalent new versioned path
     */
    std::string generateNewUserAppPathString(const std::string& oldPath) const;

    /**
     * @brief Replace all occurrences of oldString with newString, modifying contents in place.
     *
     * @param[inout] contents The string to do the replacement in
     * @param[in] oldString The string to search for
     * @param[in] newString The new string to put in place of oldString
     */
    static void replaceInContents(
        std::string& contents,
        const std::string& oldString,
        const std::string& newString
    );

private:
    std::string _configDir;
    std::string _userAppDir;
    int _major;
    int _minor;
};

}  // namespace Dialog
}  // namespace Gui
