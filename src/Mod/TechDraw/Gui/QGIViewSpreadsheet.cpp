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


#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <string>

#include <QAbstractItemView>
#include <QAbstractItemDelegate>
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QFrame>
#include <QGraphicsItemGroup>
#include <QGraphicsProxyWidget>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QPalette>
#include <QPainter>
#include <QPen>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QTransform>
#include <QVariant>

#include <App/Range.h>
#include <Mod/Spreadsheet/App/Cell.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/Utils.h>

#include "QGCustomSvg.h"
#include "QGDisplayArea.h"
#include "QGIViewSpreadsheet.h"
#include "ViewProviderSpreadsheet.h"


using namespace TechDrawGui;

namespace
{

constexpr int MaxEditorColumns = 100;
constexpr int MaxEditorRows = 200;
constexpr int SpreadsheetAlignmentRole = Qt::UserRole + 1;
constexpr int SpreadsheetTextSizeRole = Qt::UserRole + 2;
constexpr int SpreadsheetTextColorRole = Qt::UserRole + 3;
constexpr int SpreadsheetGridColorRole = Qt::UserRole + 4;
constexpr int SpreadsheetLineWidthRole = Qt::UserRole + 5;
constexpr qreal StructureButtonSpacing = 6.0;

class SpreadsheetTableItem final : public QTableWidgetItem
{
public:
    QVariant data(int role) const override
    {
        if (role == Qt::EditRole) {
            return m_editValue;
        }
        return QTableWidgetItem::data(role);
    }

    void setData(int role, const QVariant& value) override
    {
        if (role == Qt::EditRole) {
            m_editValue = value;
            QTableWidgetItem::setData(Qt::DisplayRole, value);
            return;
        }
        QTableWidgetItem::setData(role, value);
    }

    void setCellValues(const QString& displayValue,
                       const QString& editValue,
                       bool stringLiteral,
                       int alignment,
                       double textSize,
                       const QColor& textColor,
                       const QColor& gridColor,
                       double lineWidth)
    {
        m_editValue = editValue;
        m_stringLiteral = stringLiteral;
        QTableWidgetItem::setData(Qt::DisplayRole, displayValue);
        QTableWidgetItem::setData(SpreadsheetAlignmentRole, alignment);
        QTableWidgetItem::setData(SpreadsheetTextSizeRole, textSize);
        QTableWidgetItem::setData(SpreadsheetTextColorRole, textColor);
        QTableWidgetItem::setData(SpreadsheetGridColorRole, gridColor);
        QTableWidgetItem::setData(SpreadsheetLineWidthRole, lineWidth);
    }

    QString serializedEditValue() const
    {
        const QString value = m_editValue.toString();
        return m_stringLiteral ? QStringLiteral("'") + value : value;
    }

private:
    QVariant m_editValue;
    bool m_stringLiteral {false};
};

class SpreadsheetItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override
    {
        QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
        if (!editor) {
            return nullptr;
        }
        const QColor textColor = index.data(SpreadsheetTextColorRole).value<QColor>();
        QPalette palette = editor->palette();
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, Qt::white);
        palette.setColor(QPalette::Text, textColor);
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Highlight, QColor(190, 215, 240));
        palette.setColor(QPalette::HighlightedText, textColor);
        editor->setPalette(palette);
        // The text color is document data rather than theme chrome. A widget-level rule is needed
        // because application stylesheets otherwise override the delegate editor's palette.
        const QString colorName = textColor.name(QColor::HexArgb);
        editor->setStyleSheet(
            QStringLiteral("color: %1; selection-color: %1;").arg(colorName));
        return editor;
    }

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem backgroundOption(option);
        initStyleOption(&backgroundOption, index);
        const QString text = backgroundOption.text;
        backgroundOption.text.clear();
        QStyle* style = option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &backgroundOption, painter, option.widget);

        painter->save();
        painter->setClipRect(option.rect);
        painter->setFont(backgroundOption.font);

        const QRectF cellRect(option.rect);
        const double textSize = index.data(SpreadsheetTextSizeRole).toDouble();
        const QColor textColor = index.data(SpreadsheetTextColorRole).value<QColor>();
        const QColor gridColor = index.data(SpreadsheetGridColorRole).value<QColor>();
        const qreal lineWidth = index.data(SpreadsheetLineWidthRole).toDouble();
        const double horizontalInset = textSize / 2.0;
        const QFontMetricsF fontMetrics(backgroundOption.font);
        const double textWidth = fontMetrics.horizontalAdvance(text);
        const int alignment = index.data(SpreadsheetAlignmentRole).toInt();

        painter->setPen(QPen(gridColor, lineWidth));
        const qreal halfLineWidth = lineWidth / 2.0;
        const qreal left = cellRect.left() + halfLineWidth;
        const qreal right = cellRect.right() - halfLineWidth;
        const qreal top = cellRect.top() + halfLineWidth;
        const qreal bottom = cellRect.bottom() - halfLineWidth;
        if (index.column() == 0) {
            painter->drawLine(QPointF(left, top), QPointF(left, bottom));
        }
        if (index.row() == 0) {
            painter->drawLine(QPointF(left, top), QPointF(right, top));
        }
        painter->drawLine(QPointF(right, top), QPointF(right, bottom));
        painter->drawLine(QPointF(left, bottom), QPointF(right, bottom));

        if (text.isEmpty()) {
            painter->restore();
            return;
        }

        double textX = cellRect.left() + horizontalInset;
        if (alignment & Spreadsheet::Cell::ALIGNMENT_HCENTER) {
            textX = cellRect.center().x() - textWidth / 2.0;
        }
        else if (alignment & Spreadsheet::Cell::ALIGNMENT_RIGHT) {
            textX = cellRect.left() + cellRect.width() - horizontalInset - textWidth;
        }

        painter->setPen(textColor);
        const double textY = cellRect.top()
            + TechDraw::DrawViewSpreadsheet::TextBaselineHeightRatio * cellRect.height();
        painter->drawText(QPointF(textX, textY), text);
        painter->restore();
    }
};

class SpreadsheetTableWidget final : public QTableWidget
{
public:
    std::function<void(Qt::Orientation, int, int)> deleteSections;

    bool isEditingCell() const
    {
        return state() == QAbstractItemView::EditingState;
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) {
            QTableWidget::keyPressEvent(event);
            return;
        }

        const auto selected = selectedItems();
        const auto selectedRows = selectionModel()->selectedRows();
        if (rowCount() > 1 && !selectedRows.empty()
            && selected.size() == selectedRows.size() * columnCount()) {
            int firstRow = rowCount();
            int lastRow = -1;
            for (const auto& index : selectedRows) {
                firstRow = std::min(firstRow, index.row());
                lastRow = std::max(lastRow, index.row());
            }
            if (deleteSections && lastRow >= firstRow) {
                deleteSections(Qt::Vertical, firstRow, lastRow - firstRow + 1);
            }
            event->accept();
            return;
        }

        const auto selectedColumns = selectionModel()->selectedColumns();
        if (columnCount() > 1 && !selectedColumns.empty()
            && selected.size() == selectedColumns.size() * rowCount()) {
            int firstColumn = columnCount();
            int lastColumn = -1;
            for (const auto& index : selectedColumns) {
                firstColumn = std::min(firstColumn, index.column());
                lastColumn = std::max(lastColumn, index.column());
            }
            if (deleteSections && lastColumn >= firstColumn) {
                deleteSections(Qt::Horizontal, firstColumn, lastColumn - firstColumn + 1);
            }
            event->accept();
            return;
        }

        for (auto* item : selected) {
            item->setData(Qt::EditRole, QString());
        }
        event->accept();
    }
};

}  // namespace

QGIViewSpreadsheet::QGIViewSpreadsheet()
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

QGIViewSpreadsheet::~QGIViewSpreadsheet()
{
    destroyEditor();
}

void QGIViewSpreadsheet::setViewFeature(TechDraw::DrawViewSpreadsheet *obj)
{
    // called from QGVPage. (once)
    QGIView::setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}

void QGIViewSpreadsheet::updateView(bool update)
{
    if (m_isEditing) {
        // The editor is a child proxy widget of this graphics item. Reloading the hidden SVG calls
        // prepareGeometryChange() on the parent group and is unsafe while a child widget may be
        // completing a Qt event. Keep the normal view mechanics, but defer SVG regeneration until
        // edit mode is closed.
        QGIView::updateView(update);

        // QGIViewSymbol::updateView() normally updates the SVG item's scale in drawSvg(). Since
        // drawing is deliberately skipped above, keep its scale in sync explicitly so the editor
        // transform follows scale changes made while the task panel is open.
        auto* view = getViewObject<TechDraw::DrawViewSpreadsheet>();
        auto* viewProvider = getViewProvider<ViewProviderSpreadsheet>(view);
        if (view && viewProvider) {
            const double scale = viewProvider->LegacyScaling.getValue()
                ? legacyScaler(view)
                : symbolScaler(view);
            m_svgItem->setScale(scale);
        }

        updateEditorTransform();
        m_svgItem->setVisible(!m_editorGroup || !m_editorGroup->isVisible());
        drawBorder();
        return;
    }

    QGIViewSymbol::updateView(update);
}

void QGIViewSpreadsheet::setEditMode(bool enable)
{
    if (m_isEditing == enable) {
        return;
    }

    m_isEditing = enable;
    if (enable) {
        createEditor();
        populateEditor();
        return;
    }

    destroyEditor();
    m_sheet = nullptr;
    m_svgItem->show();
}

void QGIViewSpreadsheet::setEditorRange(Spreadsheet::Sheet* sheet,
                                        int startColumn,
                                        int startRow,
                                        int endColumn,
                                        int endRow)
{
    m_sheet = sheet;
    m_startColumn = startColumn;
    m_startRow = startRow;
    m_endColumn = endColumn;
    m_endRow = endRow;

    if (m_isEditing) {
        populateEditor();
    }
}

void QGIViewSpreadsheet::createEditor()
{
    if (m_editorGroup) {
        return;
    }

    m_editorGroup = new QGraphicsItemGroup();
    addToGroup(m_editorGroup);
    // addToGroup() preserves the item's scene transform. Reset it so the editor follows this
    // Spreadsheet view instead of remaining at the page scene origin.
    m_editorGroup->setPos(0.0, 0.0);
    m_editorGroup->setTransform(QTransform());
    m_editorGroup->setRotation(0.0);
    m_editorGroup->setScale(1.0);
    m_editorGroup->setHandlesChildEvents(false);
    m_editorGroup->setZValue(m_displayArea->zValue() + 1.0);

    m_table = new SpreadsheetTableWidget();
    m_table->setObjectName(QStringLiteral("spreadsheetViewEditor"));
    m_table->setAlternatingRowColors(false);
    m_table->setShowGrid(false);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                             | QAbstractItemView::EditKeyPressed
                             | QAbstractItemView::AnyKeyPressed);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setItemDelegate(new SpreadsheetItemDelegate(m_table));
    m_table->setFrameStyle(QFrame::NoFrame);
    m_table->horizontalHeader()->setObjectName(QStringLiteral("spreadsheetHorizontalHeader"));
    m_table->verticalHeader()->setObjectName(QStringLiteral("spreadsheetVerticalHeader"));
    m_table->horizontalHeader()->setSectionsClickable(true);
    m_table->verticalHeader()->setSectionsClickable(true);
    m_table->horizontalHeader()->setHighlightSections(true);
    m_table->verticalHeader()->setHighlightSections(true);
    m_table->horizontalHeader()->setMinimumSectionSize(1);
    m_table->verticalHeader()->setMinimumSectionSize(1);
    m_table->setAttribute(Qt::WA_TranslucentBackground);
    m_table->viewport()->setAttribute(Qt::WA_TranslucentBackground);
    m_table->viewport()->setAutoFillBackground(false);

    m_tableProxy = new QGraphicsProxyWidget();
    m_editorGroup->addToGroup(m_tableProxy);
    m_tableProxy->setWidget(m_table);

    m_addRowButton = new QToolButton();
    m_addColumnButton = new QToolButton();
    for (auto* button : {m_addRowButton, m_addColumnButton}) {
        button->setAutoRaise(true);
        button->setCursor(Qt::ArrowCursor);
        button->setFixedSize(24, 24);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setIconSize(QSize(18, 18));
        button->setObjectName(QStringLiteral("overlayButton"));
        button->setIcon(QIcon(QStringLiteral(":/icons/overlay-add.svg")));
    }
    m_addRowButton->setToolTip(tr("Add row"));
    m_addColumnButton->setToolTip(tr("Add column"));

    m_addRowProxy = new QGraphicsProxyWidget();
    m_editorGroup->addToGroup(m_addRowProxy);
    m_addRowProxy->setWidget(m_addRowButton);
    m_addColumnProxy = new QGraphicsProxyWidget();
    m_editorGroup->addToGroup(m_addColumnProxy);
    m_addColumnProxy->setWidget(m_addColumnButton);

    connect(m_table,
            &QTableWidget::cellChanged,
            this,
            [this](int row, int column) {
                if (m_isPopulatingEditor) {
                    return;
                }
                auto* item = m_table->item(row, column);
                if (item) {
                    auto* spreadsheetItem = static_cast<SpreadsheetTableItem*>(item);
                    Q_EMIT cellChanged(row, column, spreadsheetItem->serializedEditValue());
                }
            });
    connect(m_table, &QTableWidget::cellPressed, this, [this](int, int) {
        Q_EMIT editorActivated();
    });
    connect(m_table->horizontalHeader(),
            &QHeaderView::sectionResized,
            this,
            [this](int section, int, int newSize) {
                if (m_isPopulatingEditor) {
                    return;
                }
                updateEditorGeometry();
                drawBorder();
                Q_EMIT columnResized(section, newSize);
            });
    connect(m_table->verticalHeader(),
            &QHeaderView::sectionResized,
            this,
            [this](int section, int, int newSize) {
                if (m_isPopulatingEditor) {
                    return;
                }
                updateEditorGeometry();
                drawBorder();
                Q_EMIT rowResized(section, newSize);
            });
    connect(m_table->horizontalHeader(),
            &QHeaderView::sectionPressed,
            this,
            [this](int) {
                Q_EMIT editorActivated();
            });
    connect(m_table->verticalHeader(),
            &QHeaderView::sectionPressed,
            this,
            [this](int) {
                Q_EMIT editorActivated();
            });
    connect(m_table->horizontalHeader(),
            &QHeaderView::sectionClicked,
            this,
            [this](int section) {
                if (!m_isPopulatingEditor) {
                    m_table->clearSelection();
                    m_table->selectColumn(section);
                    m_table->setFocus(Qt::MouseFocusReason);
                }
            });
    connect(m_table->verticalHeader(),
            &QHeaderView::sectionClicked,
            this,
            [this](int section) {
                if (!m_isPopulatingEditor) {
                    m_table->clearSelection();
                    m_table->selectRow(section);
                    m_table->setFocus(Qt::MouseFocusReason);
                }
            });
    connect(m_addRowButton, &QToolButton::clicked, this, [this]() {
        queueStructureAction(PendingStructureAction::AddRow);
    });
    connect(m_addColumnButton,
            &QToolButton::clicked,
            this,
            [this]() {
                queueStructureAction(PendingStructureAction::AddColumn);
            });
    connect(m_table->itemDelegate(),
            &QAbstractItemDelegate::closeEditor,
            this,
            [this](QWidget* editor, QAbstractItemDelegate::EndEditHint) {
                // QAbstractItemView leaves EditingState before the delegate
                // editor is necessarily destroyed. Rebuilding the table in
                // that interval invalidates the editor's model index and can
                // cause an access violation after a populated cell is
                // committed by clicking a structure button.
                m_closingCellEditor = editor;
                connect(editor,
                        &QObject::destroyed,
                        this,
                        [this, editor]() {
                            if (m_closingCellEditor == editor) {
                                m_closingCellEditor = nullptr;
                            }
                            QTimer::singleShot(
                                0,
                                this,
                                &QGIViewSpreadsheet::processPendingStructureAction);
                        });
                // Some styles/delegates retain a closed editor for reuse
                // instead of destroying it. Two queued turns put us beyond
                // both the button event and QAbstractItemView's closeEditor()
                // cleanup without depending on editor deletion.
                QTimer::singleShot(0, this, [this, editor]() {
                    QTimer::singleShot(0, this, [this, editor]() {
                        if (m_closingCellEditor == editor) {
                            m_closingCellEditor = nullptr;
                        }
                        processPendingStructureAction();
                    });
                });
            });

    auto* spreadsheetTable = static_cast<SpreadsheetTableWidget*>(m_table);
    spreadsheetTable->deleteSections = [this](Qt::Orientation orientation, int first, int count) {
        QTimer::singleShot(0, this, [this, orientation, first, count]() {
            if (m_isEditing) {
                if (orientation == Qt::Vertical) {
                    Q_EMIT deleteRowsRequested(first, count);
                }
                else {
                    Q_EMIT deleteColumnsRequested(first, count);
                }
            }
        });
    };
}

void QGIViewSpreadsheet::destroyEditor()
{
    if (!m_editorGroup) {
        return;
    }

    delete m_editorGroup;
    m_editorGroup = nullptr;
    m_tableProxy = nullptr;
    m_addRowProxy = nullptr;
    m_addColumnProxy = nullptr;
    m_table = nullptr;
    m_addRowButton = nullptr;
    m_addColumnButton = nullptr;
    m_closingCellEditor = nullptr;
    m_pendingStructureAction = PendingStructureAction::None;
}

void QGIViewSpreadsheet::populateEditor()
{
    if (!m_editorGroup || !m_table) {
        return;
    }

    const int columnCount = m_endColumn - m_startColumn + 1;
    const int rowCount = m_endRow - m_startRow + 1;
    if (!m_sheet || columnCount <= 0 || rowCount <= 0 || columnCount > MaxEditorColumns
        || rowCount > MaxEditorRows) {
        m_editorGroup->hide();
        m_svgItem->show();
        return;
    }

    const int currentRow = m_table->currentRow();
    const int currentColumn = m_table->currentColumn();
    m_isPopulatingEditor = true;
    m_table->setUpdatesEnabled(false);
    m_table->clear();
    m_table->setRowCount(rowCount);
    m_table->setColumnCount(columnCount);

    QStringList columnHeaders;
    for (int column = 0; column < columnCount; ++column) {
        columnHeaders.push_back(
            QString::fromStdString(Spreadsheet::columnName(m_startColumn + column)));
    }
    m_table->setHorizontalHeaderLabels(columnHeaders);

    QStringList rowHeaders;
    for (int row = 0; row < rowCount; ++row) {
        rowHeaders.push_back(QString::number(m_startRow + row + 1));
    }
    m_table->setVerticalHeaderLabels(rowHeaders);

    for (int column = 0; column < columnCount; ++column) {
        m_table->setColumnWidth(column, m_sheet->getColumnWidth(m_startColumn + column));
    }
    for (int row = 0; row < rowCount; ++row) {
        m_table->setRowHeight(row, m_sheet->getRowHeight(m_startRow + row));
        for (int column = 0; column < columnCount; ++column) {
            updateCellItem(row, column);
        }
    }

    if (auto* view = getViewObject<TechDraw::DrawViewSpreadsheet>()) {
        QFont font(QString::fromStdString(view->Font.getValue()));
        font.setPixelSize(std::max(1, static_cast<int>(std::round(view->TextSize.getValue()))));
        m_table->setFont(font);
    }

    if (currentRow >= 0 && currentRow < rowCount && currentColumn >= 0
        && currentColumn < columnCount) {
        m_table->setCurrentCell(currentRow, currentColumn);
    }
    else {
        m_table->setCurrentCell(0, 0);
    }

    m_table->setUpdatesEnabled(true);
    m_isPopulatingEditor = false;
    m_editorGroup->show();
    m_svgItem->hide();
    updateEditorButtons();
    updateEditorGeometry();
    updateEditorTransform();
    drawBorder();
}

void QGIViewSpreadsheet::updateCellItem(int tableRow, int tableColumn)
{
    if (!m_table || !m_sheet || tableRow < 0 || tableColumn < 0
        || tableRow >= m_table->rowCount() || tableColumn >= m_table->columnCount()) {
        return;
    }

    const App::CellAddress address(m_startRow + tableRow, m_startColumn + tableColumn);
    Spreadsheet::Cell* cell = m_sheet->getCell(address);
    QString displayValue;
    QString editValue;
    bool stringLiteral = false;
    int alignment = Spreadsheet::Cell::ALIGNMENT_LEFT;
    if (cell) {
        std::string content;
        if (cell->getStringContent(content)) {
            stringLiteral = content.starts_with('\'');
            if (stringLiteral) {
                content.erase(0, 1);
            }
            editValue = QString::fromUtf8(content.c_str());
        }
        const std::string propertyName = address.toString();
        const auto* property =
            m_sheet->getPropertyByName(propertyName.c_str());
        const auto& dirtyCells = m_sheet->getCells()->getDirty();
        const bool dirty = dirtyCells.find(address) != dirtyCells.end();
        displayValue = property && !dirty
            ? QString::fromStdString(cell->getFormattedQuantity())
            : editValue;
        cell->getAlignment(alignment);
    }

    auto* item = dynamic_cast<SpreadsheetTableItem*>(m_table->item(tableRow, tableColumn));
    if (!item) {
        item = new SpreadsheetTableItem();
        m_table->setItem(tableRow, tableColumn, item);
    }
    const auto* view = getViewObject<TechDraw::DrawViewSpreadsheet>();
    const double textSize = view ? view->TextSize.getValue() : 12.0;
    const QColor gridColor = view
        ? view->TextColor.getValue().asValue<QColor>()
        : QColor(Qt::black);
    QColor textColor = gridColor;
    Base::Color cellTextColor;
    if (cell && cell->getForeground(cellTextColor)) {
        textColor = cellTextColor.asValue<QColor>();
    }
    const double lineWidth = view ? view->LineWidth.getValue() / view->getScale() : 1.0;
    item->setCellValues(
        displayValue,
        editValue,
        stringLiteral,
        alignment,
        textSize,
        textColor,
        gridColor,
        lineWidth);
}

void QGIViewSpreadsheet::updateEditorGeometry()
{
    if (!m_table || !m_tableProxy || !m_addRowButton || !m_addRowProxy
        || !m_addColumnButton || !m_addColumnProxy) {
        return;
    }

    int cellWidth = 0;
    for (int column = 0; column < m_table->columnCount(); ++column) {
        cellWidth += m_table->columnWidth(column);
    }
    int cellHeight = 0;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        cellHeight += m_table->rowHeight(row);
    }

    const int frameWidth = m_table->frameWidth();
    const int headerWidth = m_table->verticalHeader()->width();
    const int headerHeight = m_table->horizontalHeader()->height();
    m_table->setFixedSize(cellWidth + headerWidth + 2 * frameWidth,
                          cellHeight + headerHeight + 2 * frameWidth);

    m_tableProxy->setPos(-0.5 * cellWidth - headerWidth - frameWidth,
                         -0.5 * cellHeight - headerHeight - frameWidth);

    const QSize rowButtonSize = m_addRowButton->size();
    m_addRowProxy->setPos(
        -0.5 * cellWidth - 0.5 * (headerWidth + rowButtonSize.width()),
        0.5 * cellHeight + StructureButtonSpacing);

    const QSize columnButtonSize = m_addColumnButton->size();
    m_addColumnProxy->setPos(
        0.5 * cellWidth + StructureButtonSpacing,
        -0.5 * cellHeight - 0.5 * (headerHeight + columnButtonSize.height()));
}

QRectF QGIViewSpreadsheet::frameRect() const
{
    if (!m_isEditing || !m_editorGroup || !m_table || !m_editorGroup->isVisible()) {
        return QGIViewSymbol::frameRect();
    }

    int cellWidth = 0;
    for (int column = 0; column < m_table->columnCount(); ++column) {
        cellWidth += m_table->columnWidth(column);
    }
    int cellHeight = 0;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        cellHeight += m_table->rowHeight(row);
    }

    // The table proxy also contains editor-only headers and structure buttons. Use the centered
    // cell area so the view decorations match the rendered Spreadsheet content.
    const QRectF cellArea(-0.5 * cellWidth, -0.5 * cellHeight, cellWidth, cellHeight);
    return mapFromItem(m_editorGroup, cellArea).boundingRect();
}

void QGIViewSpreadsheet::updateEditorTransform()
{
    if (!m_editorGroup || !m_svgItem || !m_displayArea) {
        return;
    }

    m_editorGroup->setTransformOriginPoint(0.0, 0.0);
    m_editorGroup->setScale(m_svgItem->scale());
    m_editorGroup->setRotation(m_displayArea->rotation());
}

void QGIViewSpreadsheet::updateEditorButtons()
{
    if (!m_table || !m_addRowButton || !m_addColumnButton) {
        return;
    }

    m_addRowButton->setEnabled(m_table->rowCount() < MaxEditorRows);
    m_addColumnButton->setEnabled(m_table->columnCount() < MaxEditorColumns);
}

void QGIViewSpreadsheet::queueStructureAction(PendingStructureAction action)
{
    if (!m_isEditing || !m_table) {
        return;
    }

    m_pendingStructureAction = action;
    QTimer::singleShot(0, this, &QGIViewSpreadsheet::processPendingStructureAction);
}

void QGIViewSpreadsheet::processPendingStructureAction()
{
    if (!m_isEditing || !m_table
        || m_pendingStructureAction == PendingStructureAction::None) {
        m_pendingStructureAction = PendingStructureAction::None;
        return;
    }

    const auto* spreadsheetTable = static_cast<SpreadsheetTableWidget*>(m_table);
    if (spreadsheetTable->isEditingCell() || m_closingCellEditor) {
        // The delegate editor's destroyed signal will try again after its
        // model index can no longer be accessed.
        return;
    }

    const PendingStructureAction action = m_pendingStructureAction;
    m_pendingStructureAction = PendingStructureAction::None;
    if (action == PendingStructureAction::AddRow) {
        Q_EMIT addRowRequested();
    }
    else {
        Q_EMIT addColumnRequested();
    }
}

void QGIViewSpreadsheet::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_isEditing) {
        QGIViewSymbol::mouseDoubleClickEvent(event);
        return;
    }

    Q_UNUSED(event);
    auto vp = static_cast<ViewProviderSpreadsheet*>(getViewProvider(getViewObject()));
    if (vp) {
        vp->doubleClicked();
    }
}
