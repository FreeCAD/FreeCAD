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
class DlgSheetConf;
}

namespace SpreadsheetGui
{

class DlgSheetConf: public QDialog
{
    Q_OBJECT

public:
    explicit DlgSheetConf(Spreadsheet::Sheet* sheet, App::Range range, QWidget* parent = nullptr);
    ~DlgSheetConf() override;

    void accept() override;

    App::Property* prepare(
        App::CellAddress& from,
        App::CellAddress& to,
        std::string& rangeConf,
        App::ObjectIdentifier& path,
        bool init
    );

public Q_SLOTS:
    void onOrientationChanged();
    void onDiscard();

private:
    Spreadsheet::Sheet* sheet;
    Ui::DlgSheetConf* ui;
    App::Range originalRange;
};

}  // namespace SpreadsheetGui
