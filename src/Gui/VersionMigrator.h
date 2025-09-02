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

#ifndef GUI_VERSIONMIGRATOR_H
#define GUI_VERSIONMIGRATOR_H

#include <FCGlobal.h>
#include <cstdint>
#include <QObject>

namespace Gui {

    class MainWindow;


class GuiExport VersionMigrator : public QObject
{
    Q_OBJECT

public:
    explicit VersionMigrator(MainWindow *mw);
    void execute();

protected Q_SLOTS:

    void confirmMigration();
    void migrationCancelled() const;
    void showSizeOfMigration(uintmax_t size);
    void migrateToCurrentVersion();

private:
    MainWindow* mainWindow;
};


}

#endif // GUI_VERSIONMIGRATOR_H
