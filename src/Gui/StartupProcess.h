// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QStringList>

class QApplication;
class QMessageBox;

namespace Gui
{

class Application;
class MainWindow;

class GuiExport StartupProcess
{
public:
    StartupProcess();
    static void setupApplication();
    void execute();

private:
    void setLibraryPath();
    void setStyleSheetPaths();
    void setImagePaths();
    void registerEventType();
    void setThemePaths();
    void setupFileDialog();
};

class GuiExport StartupPostProcess
{
public:
    StartupPostProcess(MainWindow* mw, Application& guiApp, QApplication* app);
    void setLoadFromPythonModule(bool value);
    void execute();

private:
    void setWindowTitle();
    void setProcessMessages();
    void setAutoSaving();
    void checkQtSvgImageFormatSupport();
    void setToolBarIconSize();
    void setWheelEventFilter();
    void setLocale();
    void setCursorFlashing();
    void setQtStyle();
    void migrateOldTheme(const std::string& style);
    void checkOpenGL();
    void loadOpenInventor();
    void setBranding();
    void setStyleSheet();
    void autoloadModules(const QStringList& wb);
    void setImportImageFormats();
    void showMainWindow();
    void activateWorkbench();
    void checkParameters();
    void checkVersionMigration() const;

private:
    bool loadFromPythonModule = false;
    MainWindow* mainWindow;
    Application& guiApp;
    QApplication* qtApp;
};


}  // namespace Gui
