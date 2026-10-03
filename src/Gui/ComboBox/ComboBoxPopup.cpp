#include "ComboBoxPopup.h"
#include "ComboBoxGridModel.h"
#include "ComboBoxFilterModel.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QFrame>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QScreen>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableView>
#include <QScrollBar>

using namespace Gui;

ComboBoxPopup::ComboBoxPopup(QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("FreeCADComboBoxPopup"));

    setFrameShape(QFrame::StyledPanel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(2);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search..."));
    m_search->setClearButtonEnabled(true);

    m_view = new QListView(this);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);

    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setItemDelegate(new QStyledItemDelegate(m_view));

    m_gridView = new QTableView(this);
    m_gridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gridView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_gridView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    m_gridView->setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);

    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_gridView->setShowGrid(false);
    m_gridView->setWordWrap(false);
    m_gridView->setCornerButtonEnabled(false);

    m_gridView->horizontalHeader()->hide();
    m_gridView->verticalHeader()->hide();

    m_gridView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    m_gridView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    m_proxy = new ComboBoxFilterModel(this);

    m_view->setModel(m_proxy);

    m_gridModel = new ComboBoxGridModel(this);
    m_gridModel->setSourceModel(m_proxy);
    m_gridView->setModel(m_gridModel);

    layout->addWidget(m_search);
    layout->addWidget(m_view);
    layout->addWidget(m_gridView);

    m_gridView->hide();

    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setSearchText(text);

        if (m_grid) {
            m_gridColumns = calculateGridColumns();
            m_gridModel->setColumnCount(m_gridColumns);
        }

        setFixedWidth(calculatePopupWidth());

        if (m_grid) {
            updateGridSize();
        }

        updatePopupSize();

        if (m_proxy->rowCount() > 0) {
            const QModelIndex index = m_proxy->index(0, 0);
            m_view->setCurrentIndex(index);
        }
    });

    connect(m_view, &QListView::clicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = m_proxy->mapToSource(index);

        if (!sourceIndex.isValid()) {
            return;
        }

        Q_EMIT itemSelected(sourceIndex.row());

        hide();
    });

    connect(m_view, &QListView::activated, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = m_proxy->mapToSource(index);

        if (!sourceIndex.isValid()) {
            return;
        }

        Q_EMIT itemSelected(sourceIndex.row());

        hide();
    });

    connect(m_gridView, &QTableView::clicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }

        const QModelIndex proxyIndex = m_gridModel->sourceIndex(index);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        Q_EMIT itemSelected(sourceIndex.row());

        hide();
    });

    connect(m_gridView, &QTableView::activated, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }

        const QModelIndex proxyIndex = m_gridModel->sourceIndex(index);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        Q_EMIT itemSelected(sourceIndex.row());

        hide();
    });

    setSearchable(true);
    setGrid(false);
    setPopupScrollBar(true);
}

void ComboBoxPopup::setSourceModel(QAbstractItemModel* model)
{
    m_sourceModel = model;

    m_proxy->setSourceModel(model);
    m_gridModel->setSourceModel(m_proxy);

    updateViewMode();
    updatePopupSize();
}

void ComboBoxPopup::setSearchable(bool searchable)
{
    if (m_searchable == searchable) {
        return;
    }

    m_searchable = searchable;

    m_search->setVisible(searchable);

    updatePopupSize();
}

void ComboBoxPopup::setGrid(bool grid)
{
    if (m_grid == grid) {
        return;
    }

    m_grid = grid;

    updateViewMode();

    if (m_grid) {
        updateGridSize();
    }

    updatePopupSize();
}

void ComboBoxPopup::setCurrentIndex(int index)
{
    m_currentSourceRow = index;

    if (!m_sourceModel || index < 0) {
        return;
    }

    const QModelIndex sourceIndex = m_sourceModel->index(index, 0);

    const QModelIndex proxyIndex = m_proxy->mapFromSource(sourceIndex);

    if (!proxyIndex.isValid()) {
        return;
    }

    m_view->setCurrentIndex(proxyIndex);
    m_view->scrollTo(proxyIndex, QAbstractItemView::PositionAtCenter);
}

void ComboBoxPopup::setMaximumPopupHeight(int height)
{
    m_maximumHeight = qMax(100, height);
}

void ComboBoxPopup::setGridFixedColumns(int columns)
{
    if (columns < 1) {
        columns = -1;
    }

    if (m_gridFixedColumns == columns) {
        return;
    }

    m_gridFixedColumns = columns;

    if (m_grid && isVisible()) {
        updateGridSize();
        updatePopupSize();
    }
}

void ComboBoxPopup::setPopupScrollBar(bool enabled)
{
    m_popupScrollBar = enabled;

    updateViewMode();

    if (isVisible()) {
        if (m_grid) {
            updateGridSize();
        }

        updatePopupSize();
    }
}

int ComboBoxPopup::selectedSourceRow() const
{
    const QModelIndex proxyIndex = m_view->currentIndex();

    if (!proxyIndex.isValid()) {
        return -1;
    }

    const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);

    if (!sourceIndex.isValid()) {
        return -1;
    }

    return sourceIndex.row();
}

void ComboBoxPopup::updateViewMode()
{
    if (m_grid) {
        m_view->hide();
        m_gridView->show();
    }
    else {
        m_gridView->hide();
        m_view->show();
    }
}

int ComboBoxPopup::calculateItemHeight() const
{
    if (m_grid) {
        int itemHeight = m_gridView->fontMetrics().height() + 8;

        if (itemHeight <= 0) {
            itemHeight = 20;
        }

        return itemHeight;
    }

    int itemHeight = m_view->sizeHintForRow(0);

    if (itemHeight <= 0) {
        itemHeight = m_view->fontMetrics().height() + 8;
    }

    return itemHeight;
}

int ComboBoxPopup::calculateGridColumns() const
{
    if (m_gridFixedColumns > 0) {
        return m_gridFixedColumns;
    }

    const int itemCount = m_proxy->rowCount();

    if (itemCount <= 0) {
        return 1;
    }

    // 1 col / 10 items
    return qMax(1, (itemCount + 9) / 10);
}

int ComboBoxPopup::calculatePopupWidth() const
{
    const QMargins margins = layout()->contentsMargins();

    const int horizontalMargins = margins.left() + margins.right();

    int contentWidth = 0;

    if (!m_grid) {
        const int rowCount = m_proxy->rowCount();

        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = m_proxy->index(row, 0);

            if (!index.isValid()) {
                continue;
            }

            QStyleOptionViewItem option;
            option.initFrom(m_view);
            option.font = m_view->font();

            const QSize size = m_view->itemDelegate()->sizeHint(option, index);

            contentWidth = qMax(contentWidth, size.width());
        }

        // Account for the view's frame.
        contentWidth += m_view->frameWidth() * 2;
    }
    else {
        const int columns = calculateGridColumns();
        const int itemCount = m_proxy->rowCount();

        m_gridModel->setColumnCount(columns);

        QVector<int> columnWidths(columns, 0);

        for (int row = 0; row < itemCount; ++row) {
            const int column = row % columns;

            const QModelIndex index = m_gridModel->index(row / columns, column);

            if (!index.isValid()) {
                continue;
            }

            QStyleOptionViewItem option;
            option.initFrom(m_gridView);
            option.font = m_gridView->font();

            const QSize size = m_gridView->itemDelegate()->sizeHint(option, index);

            columnWidths[column] = qMax(columnWidths[column], size.width());
        }

        for (int width : columnWidths) {
            contentWidth += width;
        }

        contentWidth += m_gridView->frameWidth() * 2;
    }

    if (m_searchable) {
        contentWidth = qMax(contentWidth, m_search->sizeHint().width());
    }

    return contentWidth + horizontalMargins;
}

void ComboBoxPopup::updateGridSize()
{
    if (!m_grid) {
        return;
    }

    const QMargins margins = layout()->contentsMargins();

    const int availableWidth
        = qMax(1, width() - margins.left() - margins.right() - m_gridView->frameWidth() * 2);

    m_gridColumns = calculateGridColumns();

    m_gridModel->setColumnCount(m_gridColumns);

    QVector<int> columnWidths(m_gridColumns, 0);

    const int itemCount = m_proxy->rowCount();

    for (int row = 0; row < itemCount; ++row) {
        const int column = row % m_gridColumns;

        const QModelIndex index = m_gridModel->index(row / m_gridColumns, column);

        if (!index.isValid()) {
            continue;
        }

        QStyleOptionViewItem option;
        option.initFrom(m_gridView);
        option.font = m_gridView->font();

        const QSize size = m_gridView->itemDelegate()->sizeHint(option, index);

        columnWidths[column] = qMax(columnWidths[column], size.width());
    }

    int measuredWidth = 0;

    for (int width : columnWidths) {
        measuredWidth += width;
    }

    // If the popup was sized from the content, this should normally
    // be the same width. Keep the columns exactly filling the view.
    if (measuredWidth > 0) {
        const int extraWidth = availableWidth - measuredWidth;

        if (extraWidth > 0) {
            columnWidths[m_gridColumns - 1] += extraWidth;
        }
    }

    for (int column = 0; column < m_gridColumns; ++column) {
        m_gridView->setColumnWidth(column, qMax(1, columnWidths[column]));
    }

    const int itemHeight = calculateItemHeight();

    m_gridView->verticalHeader()->setDefaultSectionSize(itemHeight);
    m_gridView->verticalHeader()->setMinimumSectionSize(itemHeight);
}

void ComboBoxPopup::updatePopupSize()
{
    if (!m_sourceModel) {
        return;
    }

    const int itemCount = m_proxy->rowCount();

    const QMargins margins = layout()->contentsMargins();

    const int verticalMargins = margins.top() + margins.bottom();
    const int spacing = layout()->spacing();
    const int searchHeight = m_searchable ? m_search->sizeHint().height() : 0;

    if (itemCount == 0) {
        setFixedHeight(searchHeight + verticalMargins);

        return;
    }

    const int itemHeight = calculateItemHeight();

    int rows = itemCount;

    if (m_grid) {
        rows = (itemCount + m_gridColumns - 1) / m_gridColumns;
    }

    const int itemAreaHeight = rows * itemHeight;
    const int viewFrameHeight = m_grid ? m_gridView->frameWidth() * 2 : m_view->frameWidth() * 2;
    const int wantedHeight = searchHeight + (m_searchable ? spacing : 0) + itemAreaHeight
        + viewFrameHeight + verticalMargins;
    const int screenHeight = availablePopupHeight();
    const bool needsScrollBar = wantedHeight > screenHeight;
    const bool useScrollBar = m_popupScrollBar || needsScrollBar;

    const Qt::ScrollBarPolicy policy = useScrollBar ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;

    if (m_grid) {
        m_gridView->setVerticalScrollBarPolicy(policy);
    }
    else {
        m_view->setVerticalScrollBarPolicy(policy);
    }

    // no scroll bar = only show if necessary (frame too small)
    // else always show and respect maximumHeight
    if (useScrollBar) {
        const int maximumHeight = qMin(m_maximumHeight, screenHeight);

        setFixedHeight(qMin(wantedHeight, maximumHeight));
    }
    else {
        // show all items
        setFixedHeight(wantedHeight);
    }
}

void ComboBoxPopup::popup(QWidget* relativeTo)
{
    if (!relativeTo) {
        return;
    }

    m_relativeTo = relativeTo;

    updateViewMode();

    if (m_grid) {
        m_gridColumns = calculateGridColumns();
        m_gridModel->setColumnCount(m_gridColumns);
    }

    setFixedWidth(calculatePopupWidth());

    if (m_grid) {
        updateGridSize();
    }

    updatePopupSize();

    const QPoint comboTopLeft = relativeTo->mapToGlobal(QPoint(0, 0));

    const QPoint comboBottomLeft = relativeTo->mapToGlobal(QPoint(0, relativeTo->height()));

    QScreen* screen = QGuiApplication::screenAt(comboBottomLeft);

    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    QPoint position = comboBottomLeft;

    if (screen) {
        const QRect available = screen->availableGeometry();

        if (position.y() + height() > available.bottom()) {
            position.setY(comboTopLeft.y() - height());
        }

        if (position.x() + width() > available.right()) {
            position.setX(available.right() - width());
        }

        if (position.x() < available.left()) {
            position.setX(available.left());
        }
    }

    move(position);

    show();
    raise();
    activateWindow();

    setFocus(Qt::PopupFocusReason);

    if (m_searchable) {
        m_search->setFocus(Qt::PopupFocusReason);
        m_search->selectAll();
    }
    else if (m_grid) {
        m_gridView->setFocus(Qt::PopupFocusReason);
    }
    else {
        m_view->setFocus(Qt::PopupFocusReason);
    }
}

void ComboBoxPopup::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {

        const QModelIndex index = m_view->currentIndex();

        if (index.isValid()) {
            const QModelIndex sourceIndex = m_proxy->mapToSource(index);

            if (sourceIndex.isValid()) {
                Q_EMIT itemSelected(sourceIndex.row());
                hide();
                return;
            }
        }
    }

    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }

    if (m_searchable && !event->text().isEmpty() && !event->text().at(0).isSpace()) {

        const QString text = m_search->text() + event->text();

        m_search->setText(text);
        m_search->setCursorPosition(text.size());

        return;
    }

    QFrame::keyPressEvent(event);
}

void ComboBoxPopup::hideEvent(QHideEvent* event)
{
    QFrame::hideEvent(event);

    Q_EMIT popupClosed();
}

int ComboBoxPopup::availablePopupHeight() const
{
    if (!m_relativeTo) {
        return m_maximumHeight;
    }

    const QPoint topLeft = m_relativeTo->mapToGlobal(QPoint(0, 0));
    const QPoint bottomLeft = m_relativeTo->mapToGlobal(QPoint(0, m_relativeTo->height()));

    QScreen* screen = QGuiApplication::screenAt(bottomLeft);

    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen) {
        return m_maximumHeight;
    }

    const QRect available = screen->availableGeometry();

    const int spaceBelow = available.bottom() - bottomLeft.y();
    const int spaceAbove = topLeft.y() - available.top();

    return qMax(1, qMax(spaceBelow, spaceAbove));
}
