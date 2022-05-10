/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef SHEETMODEL_H
#define SHEETMODEL_H

#include <QAbstractTableModel>
#include <Mod/Spreadsheet/App/Utils.h>
#include <App/Range.h>

namespace Spreadsheet {
class Sheet;
}

namespace SpreadsheetGui {

class SheetModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SheetModel(Spreadsheet::Sheet * _sheet, QObject *parent = nullptr);
    ~SheetModel();
    
    SheetModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &) const;

private:
    void cellUpdated(App::CellAddress address);
    void rangeUpdated(const App::Range &range);

    boost::signals2::scoped_connection cellUpdatedConnection;
    boost::signals2::scoped_connection rangeUpdatedConnection;
    Spreadsheet::Sheet * sheet;
    QColor aliasBgColor;
    QColor textFgColor;
    QColor positiveFgColor;
    QColor negativeFgColor;
};

}

#endif // SHEETMODEL_H
