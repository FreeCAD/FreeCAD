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

#ifndef GUI_DIALOG_VERSIONMIGRATOR_H
#define GUI_DIALOG_VERSIONMIGRATOR_H

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

}  // namespace Dialog
}  // namespace Gui

#endif  // GUI_DIALOG_VERSIONMIGRATOR_H
