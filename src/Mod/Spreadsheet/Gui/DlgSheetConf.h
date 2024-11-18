/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef DLG_SHEETCONF_H
#define DLG_SHEETCONF_H

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

    App::Property* prepare(App::CellAddress& from,
                           App::CellAddress& to,
                           std::string& rangeConf,
                           App::ObjectIdentifier& path,
                           bool init);

public Q_SLOTS:
    void onDiscard();

private:
    Spreadsheet::Sheet* sheet;
    Ui::DlgSheetConf* ui;
};

}  // namespace SpreadsheetGui

#endif  // DLG_SHEETCONF_H
