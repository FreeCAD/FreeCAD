// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2019 Zheng Lei
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
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

#pragma once

#include <Mod/Spreadsheet/App/Sheet.h>
#include <QDialog>

namespace Ui
{
class DlgBindSheet;
}

namespace SpreadsheetGui
{

class DlgBindSheet: public QDialog
{
    Q_OBJECT

public:
    explicit DlgBindSheet(
        Spreadsheet::Sheet* sheet,
        const std::vector<App::Range>& range,
        QWidget* parent = nullptr
    );
    ~DlgBindSheet() override;

    void accept() override;

public Q_SLOTS:
    void onDiscard();

private:
    Spreadsheet::Sheet* sheet;
    App::Range range;
    Ui::DlgBindSheet* ui;
};

}  // namespace SpreadsheetGui
