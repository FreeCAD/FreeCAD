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

int ComboBoxPopup::calculateGridColumns(int availableWidth) const
{
    if (m_gridFixedColumns > 0) {
        return m_gridFixedColumns;
    }

    constexpr int minimumCellWidth = 100;

    return qMax(1, availableWidth / minimumCellWidth);
}

int ComboBoxPopup::calculatePopupWidth() const
{
    return m_relativeTo ? m_relativeTo->width() : 200;
}

void ComboBoxPopup::updateGridSize()
{
    if (!m_grid) {
        return;
    }

    const int margins = layout()->contentsMargins().left() + layout()->contentsMargins().right();

    const int availableWidth = qMax(1, width() - margins);

    m_gridColumns = calculateGridColumns(availableWidth);

    m_gridModel->setColumnCount(m_gridColumns);

    // exactly x columns
    const int baseWidth = availableWidth / m_gridColumns;
    const int remainder = availableWidth % m_gridColumns;

    for (int column = 0; column < m_gridColumns; ++column) {
        const int columnWidth = baseWidth + (column < remainder ? 1 : 0);

        m_gridView->setColumnWidth(column, columnWidth);
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
    setFixedWidth(360);

    setFixedWidth(calculatePopupWidth());
    updateViewMode();

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
