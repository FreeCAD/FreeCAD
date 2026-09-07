/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QPointer>
#include <QString>

#include "QGIViewSymbol.h"
#include "QGIUserTypes.h"

class QGraphicsItemGroup;
class QGraphicsProxyWidget;
class QTableWidget;
class QToolButton;
class QWidget;

namespace Spreadsheet
{
class Sheet;
}

namespace TechDraw
{
class DrawViewSpreadsheet;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIViewSpreadsheet : public QGIViewSymbol
{
    Q_OBJECT

public:
    QGIViewSpreadsheet();
    ~QGIViewSpreadsheet() override;

    enum {Type = UserType::QGISpreadsheet};
    int type() const override { return Type;}

    void setViewFeature(TechDraw::DrawViewSpreadsheet *obj);
    void updateView(bool update = false) override;

    void setEditMode(bool enable);
    void setEditorRange(Spreadsheet::Sheet* sheet,
                        int startColumn,
                        int startRow,
                        int endColumn,
                        int endRow);

Q_SIGNALS:
    void editorActivated();
    void cellChanged(int tableRow, int tableColumn, const QString& value);
    void columnResized(int tableColumn, int newSize);
    void rowResized(int tableRow, int newSize);
    void addRowRequested();
    void addColumnRequested();
    void deleteRowsRequested(int firstTableRow, int count);
    void deleteColumnsRequested(int firstTableColumn, int count);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    QRectF frameRect() const override;

private:
    enum class PendingStructureAction
    {
        None,
        AddRow,
        AddColumn
    };

    void createEditor();
    void destroyEditor();
    void populateEditor();
    void updateEditorGeometry();
    void updateEditorTransform();
    void updateEditorButtons();
    void updateCellItem(int tableRow, int tableColumn);
    void queueStructureAction(PendingStructureAction action);
    void processPendingStructureAction();

    bool m_isEditing {false};
    bool m_isPopulatingEditor {false};
    PendingStructureAction m_pendingStructureAction {PendingStructureAction::None};
    QPointer<QWidget> m_closingCellEditor;
    Spreadsheet::Sheet* m_sheet {nullptr};
    int m_startColumn {0};
    int m_startRow {0};
    int m_endColumn {1};
    int m_endRow {1};

    QGraphicsItemGroup* m_editorGroup {nullptr};
    QGraphicsProxyWidget* m_tableProxy {nullptr};
    QGraphicsProxyWidget* m_addRowProxy {nullptr};
    QGraphicsProxyWidget* m_addColumnProxy {nullptr};
    QTableWidget* m_table {nullptr};
    QToolButton* m_addRowButton {nullptr};
    QToolButton* m_addColumnButton {nullptr};
};

} // end namespace TechDrawGui
