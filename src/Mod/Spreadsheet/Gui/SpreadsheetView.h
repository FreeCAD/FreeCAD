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

#ifndef SpreadsheetView_H
#define SpreadsheetView_H

#include <Gui/MDIView.h>
#include <QHeaderView>
#include "SheetModel.h"
#include <Mod/Spreadsheet/App/Sheet.h>

class QSlider;
class QAction;
class QActionGroup;
class QPopupMenu;
class QToolBar;

namespace App {
class DocumentObject;
class Property;
}

namespace Ui {
class Sheet;
}

class QTableWidgetItem;

namespace SpreadsheetGui
{

class SpreadsheetDelegate;

class SpreadsheetGuiExport SheetView : public Gui::MDIView
{
    Q_OBJECT

    TYPESYSTEM_HEADER();

public:
    SheetView(Gui::Document* pcDocument, App::DocumentObject* docObj, QWidget* parent);
    ~SheetView();

    const char *getName(void) const {return "SheetView";}

    bool onMsg(const char* pMsg,const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;

    void updateCell(const App::Property * prop);

    Spreadsheet::Sheet * getSheet() { return sheet; }

    std::vector<App::Range> selectedRanges() const;

    QModelIndexList selectedIndexes() const;

    QModelIndex currentIndex() const;

    void deleteSelection();

    PyObject *getPyObject(void);

    virtual void deleteSelf();

    void updateHiddenRows();
    void updateHiddenColumns();

protected Q_SLOTS:
    void editingFinished();
    void currentChanged( const QModelIndex & current, const QModelIndex & previous );
    void columnResized(int col, int oldSize, int newSize);
    void rowResized(int row, int oldSize, int newSize);
    void columnResizeFinished();
    void rowResizeFinished();
    void modelUpdated(const QModelIndex & topLeft, const QModelIndex & bottomRight);
protected:
    void updateContentLine();
    void setCurrentCell(QString str);
    void keyPressEvent(QKeyEvent *event);
    void resizeColumn(int col, int newSize);
    void resizeRow(int col, int newSize);

    Ui::Sheet * ui;
    Spreadsheet::Sheet * sheet;
    SpreadsheetDelegate * delegate;
    SheetModel * model;
    boost::signals2::scoped_connection columnWidthChangedConnection;
    boost::signals2::scoped_connection rowHeightChangedConnection;
    boost::signals2::scoped_connection positionChangedConnection;

    QMap<int, int> newColumnSizes;
    QMap<int, int> newRowSizes;
};

} // namespace SpreadsheetModGui

#endif // SpreadsheetView_H
