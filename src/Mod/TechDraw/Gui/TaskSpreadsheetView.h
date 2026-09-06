/* SPDX - License - Identifier: LGPL - 2.1 - or -later
 ****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/
#ifndef TECHDRAWGUI_TASKSPREADSHEETVIEW_H
#define TECHDRAWGUI_TASKSPREADSHEETVIEW_H

#include <Gui/TaskView/TaskDialog.h>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <memory>  // For std::unique_ptr
#include <vector>

// Forward declarations
class Ui_TaskSpreadsheetView;  // The class name from the .ui file for the widget's UI
namespace Spreadsheet
{
class Sheet;
}
namespace TechDraw
{
class DrawViewSpreadsheet;
class DrawPage;
}  // namespace TechDraw
namespace App
{
class DocumentObject;
}  // namespace App
namespace Gui
{
class Document;
}  // namespace Gui
class QFont;

namespace TechDrawGui
{
class QGIViewSpreadsheet;
class ViewProviderSpreadsheet;

//---------------------------------------------------------------------------
// TaskSpreadsheetView (The QWidget holding the UI elements)
//---------------------------------------------------------------------------
class TaskSpreadsheetView: public QWidget
{
    Q_OBJECT

public:
    explicit TaskSpreadsheetView(TechDraw::DrawViewSpreadsheet* viewToEdit,
                                 TechDraw::DrawPage* targetPage,
                                 QWidget* parent = nullptr);
    ~TaskSpreadsheetView() override;

    bool initializeContent();

    bool apply();
    void reject();
    TechDraw::DrawViewSpreadsheet* getViewObject() const { return m_viewObject; }


protected:
    void changeEvent(QEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void onStartCellEditingFinished();
    void onEndCellEditingFinished();
    void onEditorCellChanged(int row, int column, const QString& data);
    void onEditorColumnResized(int logicalIndex, int newSize);
    void onEditorRowResized(int logicalIndex, int newSize);
    void onAddRowRequested();
    void onAddColumnRequested();
    void onDeleteRowsRequested(int firstTableRow, int count);
    void onDeleteColumnsRequested(int firstTableColumn, int count);
    void onFontChanged(const QFont& font);
    void onScaleChanged(double value);
    void onTextSizeChanged(double value);
    void onTextColorChanged();
    void onClaimSpreadsheetToggled(bool val);
    void onLineWidthChanged(double value);

private:
    struct PendingCellEdit
    {
        int sheetRow;
        int sheetColumn;
        QString data;
    };

    ViewProviderSpreadsheet* getVps();
    QGIViewSpreadsheet* getQgiView();
    void loadGuiFromView();

    void setupViewEditor();
    void disableViewEditor();
    void refreshViewEditor();
    void processPendingCellEdits();
    void updateSpreadsheetCellValue(int tableRow, int tableCol, const QString& data);
    bool parseCellAddress(const QString& address, int& col, int& row) const;  // 0-indexed
    QString cellAddressToString(int col, int row) const;  // 0-indexed to A1 style
    void revalidateRangeAndUpdateEditor();

    std::unique_ptr<Ui_TaskSpreadsheetView> ui;
    TechDraw::DrawViewSpreadsheet* m_viewObject;  // The view object being created or edited
    Spreadsheet::Sheet* m_spreadsheet;            // The source spreadsheet
    TechDraw::DrawPage* m_targetPage;             // Page to add new views to
    Gui::Document* m_doc {nullptr};
    QPointer<QGIViewSpreadsheet> m_qgiView;
    QPointer<QWidget> m_viewport;

    // Parsed range for the in-view editor, 0-indexed
    int m_startCol, m_startRow;
    int m_endCol, m_endRow;
    bool m_rangeValid;

    bool m_isPopulatingGui;              // Flag to prevent ui signals during programmatic data load
    bool m_acceptPending {false};
    bool m_processingCellEdits {false};
    std::vector<PendingCellEdit> m_pendingCellEdits;
};


//---------------------------------------------------------------------------
// TaskDlgSpreadsheetView (The TaskDialog itself)
//---------------------------------------------------------------------------
class TaskDlgSpreadsheetView: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    // targetPage is the page containing viewToEdit.
    explicit TaskDlgSpreadsheetView(TechDraw::DrawPage* targetPage,
                                    TechDraw::DrawViewSpreadsheet* viewToEdit,
                                    bool creatingView = false);

    TechDraw::DrawViewSpreadsheet* getViewObject() const;

    void open() override;
    bool accept() override;
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }


    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        // Ok and Cancel are standard, Apply can be added if needed for non-modal updates
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskSpreadsheetView* m_widget;
};

}  // namespace TechDrawGui

#endif  // TECHDRAWGUI_TASKSPREADSHEETVIEW_H
