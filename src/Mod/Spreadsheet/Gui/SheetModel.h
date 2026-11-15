// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Eivind Kvedalen
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

#include "fastsignals/connection.h"
#include <QAbstractTableModel>
#include <boost/signals2.hpp>

#include <App/Range.h>


namespace Spreadsheet
{
class Sheet;
}

namespace SpreadsheetGui
{

class SheetModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SheetModel(Spreadsheet::Sheet* _sheet, QObject* parent = nullptr);
    ~SheetModel() override;

    explicit SheetModel(QObject* parent);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex&) const override;

private Q_SLOTS:
    void setCellData(QModelIndex index, QString str);

private:
    void containSheetDataInView();
    void cellUpdated(App::CellAddress address);
    void rangeUpdated(const App::Range& range);

    std::vector<fastsignals::scoped_connection> connections;
    Spreadsheet::Sheet* sheet;
    int rows, cols;
    QColor aliasBgColor;
    QColor textFgColor;
    QColor positiveFgColor;
    QColor negativeFgColor;
};

}  // namespace SpreadsheetGui
