// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2005 Werner Mayer <wmayer@users.sourceforge.net>                       *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef SPREADSHEET_WORKBENCH_H
#define SPREADSHEET_WORKBENCH_H

#include <Gui/Workbench.h>
#include <Mod/Spreadsheet/SpreadsheetGlobal.h>

class QtColorPicker;
class QColor;

namespace SpreadsheetGui
{

/**
 * @author Eivind Kvedalen
 */

class SpreadsheetGuiExport WorkbenchHelper: public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void setForegroundColor(const QColor& color);
    void setBackgroundColor(const QColor& color);
};

class SpreadsheetGuiExport Workbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;
    void activated() override;

private:
    bool initialized;
    std::unique_ptr<WorkbenchHelper> workbenchHelper;

protected:
    Gui::MenuItem* setupMenuBar() const override;
    Gui::ToolBarItem* setupToolBars() const override;
    Gui::ToolBarItem* setupCommandBars() const override;
};

}  // namespace SpreadsheetGui


#endif  // SPREADSHEET_WORKBENCH_H
