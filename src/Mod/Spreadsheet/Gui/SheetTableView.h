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

#ifndef SHEETTABLEVIEW_H
#define SHEETTABLEVIEW_H

#include <QTableView>
#include <QHeaderView>
#include <boost/signals/connection.hpp>
#include "PreCompiled.h"
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/Utils.h>

namespace SpreadsheetGui {

class SheetViewHeader : public QHeaderView {
    Q_OBJECT
public:
    SheetViewHeader(Qt::Orientation o) : QHeaderView(o) {
        setClickable(true);
    }
Q_SIGNALS:
    void resizeFinished();
protected:
    void mouseReleaseEvent(QMouseEvent * event);
};

class SheetTableView : public QTableView
{
    Q_OBJECT
public:
    explicit SheetTableView(QWidget *parent = 0);
    ~SheetTableView();
    
    void edit(const QModelIndex &index);
    void setSheet(Spreadsheet::Sheet *_sheet);
    std::vector<Spreadsheet::Range> selectedRanges() const;
protected Q_SLOTS:
    void commitData(QWidget *editor);
    void updateCellSpan(Spreadsheet::CellAddress address);
    void insertRows();
    void removeRows();
    void insertColumns();
    void removeColumns();
    void cellProperties();
protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    QModelIndex currentEditIndex;
    Spreadsheet::Sheet * sheet;

    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection cellSpanChangedConnection;
};

}

#endif // SHEETTABLEVIEW_H
