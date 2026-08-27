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

#include "PreCompiled.h"

#include <algorithm>

#ifndef _PreComp_
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>
#include <QVariant>
#endif

#include "TaskSpreadsheetView.h"
#include "ui_TaskSpreadsheetView.h"

// FreeCAD Base Includes
#include <Base/Console.h>
#include <Base/Exception.h>

// FreeCAD App Includes
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/Cell.h> // For setting cell content

// FreeCAD Gui Includes
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Widgets.h> // For Gui::ColorButton

// TechDraw Includes
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawUtil.h> // For getUniqueObjectName if not in Base::Tools
#include <Mod/TechDraw/App/Preferences.h> // For default font/size
#include <Mod/TechDraw/App/LineGroup.h>
#include <Mod/TechDraw/Gui/ViewProviderSpreadsheet.h> // For ClaimSheetAsChild
#include "PreferencesGui.h"
#include "QGIViewSpreadsheet.h"

namespace TechDrawGui {

//===========================================================================
// TaskSpreadsheetView (The QWidget)
//===========================================================================

TaskSpreadsheetView::TaskSpreadsheetView(TechDraw::DrawViewSpreadsheet* viewToEdit,
                                         TechDraw::DrawPage* targetPage,
                                         QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskSpreadsheetView())
    , m_viewObject(viewToEdit)
    , m_spreadsheet(nullptr)
    , m_targetPage(targetPage)
    , m_startCol(0), m_startRow(0) // Default A1
    , m_endCol(1), m_endRow(1)     // Default B2
    , m_rangeValid(true)
    , m_isPopulatingGui(false)
{
    ui->setupUi(this);

    // Connect signals
    connect(ui->lineEdit_StartCell, &QLineEdit::editingFinished, this, &TaskSpreadsheetView::onStartCellEditingFinished);
    connect(ui->lineEdit_EndCell, &QLineEdit::editingFinished, this, &TaskSpreadsheetView::onEndCellEditingFinished);

    connect(ui->fontComboBox_Font, &QFontComboBox::currentFontChanged, this, &TaskSpreadsheetView::onFontChanged);
    connect(ui->doubleSpinBox_TextSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TaskSpreadsheetView::onTextSizeChanged);
    connect(ui->doubleSpinBox_Scale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TaskSpreadsheetView::onScaleChanged);
    connect(ui->cpFrameColor, &Gui::ColorButton::changed, this, &TaskSpreadsheetView::onTextColorChanged);
    connect(ui->doubleSpinBox_LineWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TaskSpreadsheetView::onLineWidthChanged);
    connect(ui->checkBox_ClaimSpreadsheet, &QCheckBox::toggled, this, &TaskSpreadsheetView::onClaimSpreadsheetToggled);

    // Initial placeholder text - will be overwritten by loadGuiFrom*
    ui->lineEdit_StartCell->setText(QStringLiteral("A1"));
    ui->lineEdit_EndCell->setText(QStringLiteral("B2"));
    parseCellAddress(ui->lineEdit_StartCell->text(), m_startCol, m_startRow);
    parseCellAddress(ui->lineEdit_EndCell->text(), m_endCol, m_endRow);
}

TaskSpreadsheetView::~TaskSpreadsheetView()
{
    disableViewEditor();
}

bool TaskSpreadsheetView::initializeContent()
{
    if (!m_targetPage || !m_targetPage->getDocument()) {
        Base::Console().error(
            "TaskSpreadsheetView::initializeContent: No valid target page or document.\n");
        return false;
    }

    m_isPopulatingGui = true; // Block signals during initial load

    m_doc = Gui::Application::Instance->getDocument(m_targetPage->getDocument());
    if (!m_doc) {
        Base::Console().error(
            "TaskSpreadsheetView::initializeContent: No valid target page or document.\n"
        );
        return false;
    }

    if (!m_viewObject) {
        Base::Console().error("TaskSpreadsheetView: No Spreadsheet View to edit.\n");
        return false;
    }

    m_spreadsheet = freecad_cast<Spreadsheet::Sheet*>(m_viewObject->Source.getValue());
    if (!m_spreadsheet) {
        Base::Console().warning("TaskSpreadsheetView: Source spreadsheet is bad.\n");
        return false;
    }

    loadGuiFromView();

    m_isPopulatingGui = false;

    setupViewEditor();
    revalidateRangeAndUpdateEditor();
    return true;
}


void TaskSpreadsheetView::loadGuiFromView()
{
    ui->lineEdit_StartCell->setText(QString::fromStdString(m_viewObject->CellStart.getValue()));
    ui->lineEdit_EndCell->setText(QString::fromStdString(m_viewObject->CellEnd.getValue()));

    parseCellAddress(ui->lineEdit_StartCell->text(), m_startCol, m_startRow);
    parseCellAddress(ui->lineEdit_EndCell->text(), m_endCol, m_endRow);

    ui->fontComboBox_Font->setCurrentFont(QFont(QString::fromStdString(m_viewObject->Font.getValue())));
    ui->doubleSpinBox_TextSize->setValue(m_viewObject->TextSize.getValue());
    ui->doubleSpinBox_Scale->setValue(m_viewObject->Scale.getValue());
    ui->cpFrameColor->setColor(m_viewObject->TextColor.getValue().asValue<QColor>());
    ui->doubleSpinBox_LineWidth->setValue(m_viewObject->LineWidth.getValue());

    auto* vps = getVps();
    if (vps) {
        ui->checkBox_ClaimSpreadsheet->setChecked(vps->ClaimSheetAsChild.getValue());
    }
}

bool TaskSpreadsheetView::apply()
{
    if (!m_rangeValid) {
        QMessageBox::warning(this, tr("Invalid Range"), tr("The specified cell range is invalid. Please correct it."));
        return false;
    }

    processPendingCellEdits();
    disableViewEditor();
    m_doc->commitCommand();
    m_doc->resetEdit();

    m_viewObject->touch();
    m_viewObject->getDocument()->recompute();

    return true;
}

void TaskSpreadsheetView::reject()
{
    m_pendingCellEdits.clear();
    disableViewEditor();
    if (m_doc) {
        m_doc->abortCommand();
        m_doc->resetEdit();
    }
}

ViewProviderSpreadsheet* TaskSpreadsheetView::getVps()
{
    if (!m_viewObject || !m_viewObject->getDocument()) {
        return nullptr;
    }

    auto* guiDocument =
        Gui::Application::Instance->getDocument(m_viewObject->getDocument());
    if (!guiDocument) {
        return nullptr;
    }

    return freecad_cast<TechDrawGui::ViewProviderSpreadsheet*>(
        guiDocument->getViewProvider(m_viewObject));
}

QGIViewSpreadsheet* TaskSpreadsheetView::getQgiView()
{
    auto* viewProvider = getVps();
    if (!viewProvider) {
        return nullptr;
    }
    return dynamic_cast<QGIViewSpreadsheet*>(viewProvider->getQView());
}

void TaskSpreadsheetView::setupViewEditor()
{
    m_qgiView = getQgiView();
    if (!m_qgiView) {
        Base::Console().warning(
            "TaskSpreadsheetView: Spreadsheet graphics item is not available for editing.\n");
        return;
    }

    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::editorActivated,
            this,
            [this]() {
                Gui::Selection().clearSelection();
                // A page-scene selection change can be delivered after the proxy widget's press
                // signal. Clear once more after the current mouse event has fully propagated.
                QTimer::singleShot(0, this, []() {
                    Gui::Selection().clearSelection();
                });
            });
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::cellChanged,
            this,
            &TaskSpreadsheetView::onEditorCellChanged);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::columnResized,
            this,
            &TaskSpreadsheetView::onEditorColumnResized);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::rowResized,
            this,
            &TaskSpreadsheetView::onEditorRowResized);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::addRowRequested,
            this,
            &TaskSpreadsheetView::onAddRowRequested);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::addColumnRequested,
            this,
            &TaskSpreadsheetView::onAddColumnRequested);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::deleteRowsRequested,
            this,
            &TaskSpreadsheetView::onDeleteRowsRequested);
    connect(m_qgiView.data(),
            &QGIViewSpreadsheet::deleteColumnsRequested,
            this,
            &TaskSpreadsheetView::onDeleteColumnsRequested);

    Gui::Selection().clearSelection();

    if (auto* editorScene = m_qgiView->scene()) {
        const auto sceneViews = editorScene->views();
        if (!sceneViews.isEmpty()) {
            m_viewport = sceneViews.constFirst()->viewport();
            m_viewport->installEventFilter(this);
        }
    }

    m_qgiView->setEditMode(true);
    refreshViewEditor();
}

void TaskSpreadsheetView::disableViewEditor()
{
    if (m_viewport) {
        m_viewport->removeEventFilter(this);
        m_viewport = nullptr;
    }
    m_acceptPending = false;

    if (!m_qgiView) {
        return;
    }

    m_qgiView->setEditMode(false);
    disconnect(m_qgiView.data(), nullptr, this, nullptr);
    m_qgiView = nullptr;
}

void TaskSpreadsheetView::refreshViewEditor()
{
    if (!m_qgiView) {
        return;
    }

    m_qgiView->setEditorRange(
        m_rangeValid ? m_spreadsheet : nullptr, m_startCol, m_startRow, m_endCol, m_endRow);
}

void TaskSpreadsheetView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    } else {
        QWidget::changeEvent(e);
    }
}

bool TaskSpreadsheetView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_viewport || event->type() != QEvent::MouseButtonPress || !m_qgiView
        || m_acceptPending) {
        return QWidget::eventFilter(watched, event);
    }

    QGraphicsView* graphicsView = nullptr;
    if (auto* editorScene = m_qgiView->scene()) {
        const auto sceneViews = editorScene->views();
        for (auto* sceneView : sceneViews) {
            if (sceneView->viewport() == m_viewport) {
                graphicsView = sceneView;
                break;
            }
        }
    }
    if (!graphicsView) {
        return QWidget::eventFilter(watched, event);
    }

    const auto* mouseEvent = static_cast<QMouseEvent*>(event);
    QGraphicsItem* clickedItem = graphicsView->itemAt(mouseEvent->pos());
    while (clickedItem) {
        if (clickedItem == m_qgiView) {
            return QWidget::eventFilter(watched, event);
        }
        clickedItem = clickedItem->parentItem();
    }

    // Proxy widgets can consume a mouse press before itemAt() resolves their graphics item.
    // Treat any point within the Spreadsheet item's complete child bounds as an inside click.
    if (m_qgiView->sceneBoundingRect().contains(graphicsView->mapToScene(mouseEvent->pos()))) {
        return QWidget::eventFilter(watched, event);
    }

    App::Document* document = m_targetPage ? m_targetPage->getDocument() : nullptr;
    QPointer<Gui::TaskView::TaskDialog> expectedDialog =
        document ? Gui::Control().activeDialog(document) : nullptr;
    if (!document || !expectedDialog) {
        return QWidget::eventFilter(watched, event);
    }

    m_acceptPending = true;

    // Moving focus to the page commits an active cell editor. Accepting is queued so the
    // delegate can finish closing before the task removes the in-view table.
    m_viewport->setFocus(Qt::MouseFocusReason);
    QTimer::singleShot(0, this, [this, document, expectedDialog]() {
        if (Gui::Control().activeDialog(document) != expectedDialog) {
            m_acceptPending = false;
            return;
        }

        const QPointer<TaskSpreadsheetView> guard(this);
        Gui::Control().accept(document);
        if (guard) {
            guard->m_acceptPending = false;
        }
    });
    return true;
}

bool TaskSpreadsheetView::parseCellAddress(const QString& addressStr, int& colIdx, int& rowIdx) const
{
    // A simple parser for "A1", "B2", "AA10" etc.
    // Converts to 0-indexed colIdx, rowIdx
    if (addressStr.isEmpty()) return false;

    static const QRegularExpression re(
        QStringLiteral("^([A-Z]+)([1-9][0-9]*)$"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(addressStr.toUpper());

    if (!match.hasMatch()) {
        return false;
    }

    QString colStr = match.captured(1);
    QString rowStr = match.captured(2);

    // Convert column letters to 0-indexed integer
    long tempCol = 0;
    for (QChar c : colStr) {
        tempCol = tempCol * 26 + (c.toLatin1() - 'A' + 1);
    }
    colIdx = static_cast<int>(tempCol - 1); // 0-indexed

    bool ok;
    rowIdx = rowStr.toInt(&ok) - 1; // 0-indexed

    return ok && colIdx >= 0 && rowIdx >=0;
}

QString TaskSpreadsheetView::cellAddressToString(int colIdx, int rowIdx) const
{
    // Converts 0-indexed colIdx, rowIdx to "A1" style string
    if (colIdx < 0 || rowIdx < 0) return QString();

    QString colStr;
    int tempCol = colIdx + 1; // 1-indexed for conversion
    while (tempCol > 0) {
        int remainder = (tempCol - 1) % 26;
        colStr.prepend(QChar('A' + remainder));
        tempCol = (tempCol - 1) / 26;
    }
    return colStr + QString::number(rowIdx + 1);
}

void TaskSpreadsheetView::revalidateRangeAndUpdateEditor()
{
    if (m_isPopulatingGui) {
        return;
    }

    bool startOk = parseCellAddress(ui->lineEdit_StartCell->text(), m_startCol, m_startRow);
    bool endOk = parseCellAddress(ui->lineEdit_EndCell->text(), m_endCol, m_endRow);

    m_rangeValid = startOk && endOk &&
                   m_startCol <= m_endCol &&
                   m_startRow <= m_endRow;

    const bool reversedRange = startOk && endOk
        && (m_startCol > m_endCol || m_startRow > m_endRow);
    ui->lineEdit_StartCell->setProperty(
        "validationState", !startOk || reversedRange ? QStringLiteral("error") : QVariant());
    ui->lineEdit_EndCell->setProperty(
        "validationState", !endOk || reversedRange ? QStringLiteral("error") : QVariant());
    refreshViewEditor();

    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onStartCellEditingFinished()
{
    m_viewObject->CellStart.setValue(ui->lineEdit_StartCell->text().toStdString());
    revalidateRangeAndUpdateEditor();
}

void TaskSpreadsheetView::onEndCellEditingFinished()
{
    m_viewObject->CellEnd.setValue(ui->lineEdit_EndCell->text().toStdString());
    revalidateRangeAndUpdateEditor();
}

void TaskSpreadsheetView::onEditorCellChanged(int row, int column, const QString& data)
{
    if (!m_spreadsheet || !m_rangeValid) {
        return;
    }

    const int sheetColumn = m_startCol + column;
    const int sheetRow = m_startRow + row;
    m_pendingCellEdits.push_back({sheetRow, sheetColumn, data});

    // Match SpreadsheetGui::SheetModel: application data must not be changed directly from a
    // delegate commit. Qt is still closing the editor while cellChanged is emitted.
    QTimer::singleShot(0, this, &TaskSpreadsheetView::processPendingCellEdits);
}

void TaskSpreadsheetView::processPendingCellEdits()
{
    if (m_processingCellEdits || m_pendingCellEdits.empty()) {
        return;
    }
    if (!m_spreadsheet || !m_viewObject) {
        m_pendingCellEdits.clear();
        return;
    }

    m_processingCellEdits = true;
    std::vector<PendingCellEdit> edits;
    edits.swap(m_pendingCellEdits);

    for (const auto& edit : edits) {
        try {
            updateSpreadsheetCellValue(
                edit.sheetRow + 1, edit.sheetColumn + 1, edit.data);
        }
        catch (const Base::Exception& error) {
            error.reportException();
        }
    }

    m_processingCellEdits = false;
}

void TaskSpreadsheetView::updateSpreadsheetCellValue(int sheet_row_1based, int sheet_col_1based, const QString& data)
{
    if (!m_spreadsheet) return;

    m_spreadsheet->setCell(App::CellAddress(sheet_row_1based - 1, sheet_col_1based - 1).toString().c_str(),
        data.toStdString().c_str());
}

void TaskSpreadsheetView::onFontChanged(const QFont& font)
{
    if (m_isPopulatingGui) {
        return;
    }

    m_viewObject->Font.setValue(font.family().toStdString());

    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onTextSizeChanged(double value)
{
    if (m_isPopulatingGui) {
        return;
    }

    m_viewObject->TextSize.setValue(value);

    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onScaleChanged(double value)
{
    if (m_isPopulatingGui) {
        return;
    }

    m_viewObject->Scale.setValue(value);

    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onTextColorChanged()
{
    if (m_isPopulatingGui) {
        return;
    }

    Base::Color ac;
    ac.setValue<QColor>(ui->cpFrameColor->color());
    m_viewObject->TextColor.setValue(ac);

    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onLineWidthChanged(double value)
{
    if (m_isPopulatingGui) {
        return;
    }

    m_viewObject->LineWidth.setValue(value);

    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onClaimSpreadsheetToggled(bool val)
{
    if (m_isPopulatingGui) {
        return;
    }

    TechDrawGui::ViewProviderSpreadsheet* vp = getVps();
    if (vp) {
        vp->ClaimSheetAsChild.setValue(val);
    }

    m_viewObject->touch();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onEditorColumnResized(int logicalIndex, int newSize)
{
    if (m_isPopulatingGui || !m_rangeValid || !m_spreadsheet) {
        return;
    }

    m_spreadsheet->setColumnWidth(m_startCol + logicalIndex, newSize);
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onEditorRowResized(int logicalIndex, int newSize)
{
    if (m_isPopulatingGui || !m_rangeValid || !m_spreadsheet) {
        return;
    }

    m_spreadsheet->setRowHeight(m_startRow + logicalIndex, newSize);
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onAddRowRequested()
{
    if (!m_rangeValid) {
        return;
    }
    if (m_processingCellEdits || !m_pendingCellEdits.empty()) {
        // cellChanged() deliberately queues Spreadsheet writes until after
        // the delegate commit. Do not force that write synchronously from a
        // structure-button callback; wait for the already queued edit pass.
        QTimer::singleShot(0, this, &TaskSpreadsheetView::onAddRowRequested);
        return;
    }

    ++m_endRow;
    const QString endAddress = cellAddressToString(m_endCol, m_endRow);
    ui->lineEdit_EndCell->setText(endAddress);
    m_viewObject->CellEnd.setValue(endAddress.toStdString());
    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onAddColumnRequested()
{
    if (!m_rangeValid) {
        return;
    }
    if (m_processingCellEdits || !m_pendingCellEdits.empty()) {
        QTimer::singleShot(0, this, &TaskSpreadsheetView::onAddColumnRequested);
        return;
    }

    ++m_endCol;
    const QString endAddress = cellAddressToString(m_endCol, m_endRow);
    ui->lineEdit_EndCell->setText(endAddress);
    m_viewObject->CellEnd.setValue(endAddress.toStdString());
    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onDeleteRowsRequested(int firstTableRow, int count)
{
    if (m_processingCellEdits || !m_pendingCellEdits.empty()) {
        QTimer::singleShot(0, this, [this, firstTableRow, count]() {
            onDeleteRowsRequested(firstTableRow, count);
        });
        return;
    }
    const int rowCount = m_endRow - m_startRow + 1;
    const int removableCount =
        std::min(count, std::max(0, rowCount - 1));
    if (!m_rangeValid || !m_spreadsheet || firstTableRow < 0 || removableCount <= 0
        || firstTableRow + removableCount > rowCount) {
        return;
    }

    m_spreadsheet->removeRows(m_startRow + firstTableRow, removableCount);
    m_endRow -= removableCount;
    const QString endAddress = cellAddressToString(m_endCol, m_endRow);
    ui->lineEdit_EndCell->setText(endAddress);
    m_viewObject->CellEnd.setValue(endAddress.toStdString());
    refreshViewEditor();
    m_viewObject->recomputeFeature();
}

void TaskSpreadsheetView::onDeleteColumnsRequested(int firstTableColumn, int count)
{
    if (m_processingCellEdits || !m_pendingCellEdits.empty()) {
        QTimer::singleShot(0, this, [this, firstTableColumn, count]() {
            onDeleteColumnsRequested(firstTableColumn, count);
        });
        return;
    }
    const int columnCount = m_endCol - m_startCol + 1;
    const int removableCount =
        std::min(count, std::max(0, columnCount - 1));
    if (!m_rangeValid || !m_spreadsheet || firstTableColumn < 0 || removableCount <= 0
        || firstTableColumn + removableCount > columnCount) {
        return;
    }

    m_spreadsheet->removeColumns(m_startCol + firstTableColumn, removableCount);
    m_endCol -= removableCount;
    const QString endAddress = cellAddressToString(m_endCol, m_endRow);
    ui->lineEdit_EndCell->setText(endAddress);
    m_viewObject->CellEnd.setValue(endAddress.toStdString());
    refreshViewEditor();
    m_viewObject->recomputeFeature();
}


//===========================================================================
// TaskDlgSpreadsheetView (The Dialog)
//===========================================================================

TaskDlgSpreadsheetView::TaskDlgSpreadsheetView(TechDraw::DrawPage* targetPage,
                                               TechDraw::DrawViewSpreadsheet* viewToEdit,
                                               bool creatingView)
    : Gui::TaskView::TaskDialog()
    , m_widget(new TaskSpreadsheetView(viewToEdit, targetPage))
{
    m_widget->setWindowTitle(creatingView ? tr("Create Spreadsheet View")
                                          : tr("Edit Spreadsheet View"));
    addTaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_SpreadsheetView"), m_widget);
}

TechDraw::DrawViewSpreadsheet* TaskDlgSpreadsheetView::getViewObject() const
{
    return m_widget->getViewObject();
}

void TaskDlgSpreadsheetView::open()
{
    if (!m_widget->initializeContent()) {
        Base::Console().error("TaskDlgSpreadsheetView::open: Widget initialization failed. Rejecting.\n");
        Gui::Control().closeDialog();
        return;
    }
    Gui::TaskView::TaskDialog::open();
}

bool TaskDlgSpreadsheetView::accept()
{
    return m_widget->apply();
}

bool TaskDlgSpreadsheetView::reject()
{
    m_widget->reject();
    return true; // Always allow reject to close the dialog
}


} // namespace TechDrawGui

#include "moc_TaskSpreadsheetView.cpp"
