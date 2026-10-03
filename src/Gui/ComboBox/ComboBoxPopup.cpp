#include "ComboBoxPopup.h"
#include "ComboBoxFilterModel.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QScreen>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

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

    /*
     * Use Qt's normal item delegate.
     */
    m_view->setItemDelegate(new QStyledItemDelegate(m_view));

    m_proxy = new ComboBoxFilterModel(this);
    m_view->setModel(m_proxy);

    layout->addWidget(m_search);
    layout->addWidget(m_view);

    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setSearchText(text);

        updatePopupSize();

        if (m_proxy->rowCount() > 0) {
            QModelIndex index = m_proxy->index(0, 0);
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

    setSearchable(true);
    setGrid(false);
}

void ComboBoxPopup::setSourceModel(QAbstractItemModel* model)
{
    m_sourceModel = model;

    m_proxy->setSourceModel(model);

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
        m_view->setViewMode(QListView::IconMode);
        m_view->setFlow(QListView::LeftToRight);
        m_view->setWrapping(true);
        m_view->setResizeMode(QListView::Adjust);
        m_view->setMovement(QListView::Static);
        m_view->setSpacing(2);

        updateGridSize();
    }
    else {
        m_view->setViewMode(QListView::ListMode);
        m_view->setFlow(QListView::TopToBottom);
        m_view->setWrapping(false);
        m_view->setResizeMode(QListView::Adjust);
        m_view->setMovement(QListView::Static);
        m_view->setSpacing(0);

        m_view->setGridSize(QSize());
    }
}

void ComboBoxPopup::updateGridSize()
{
    if (!m_grid) {
        return;
    }

    /*
     * Use Qt's normal list item height.
     */
    int itemHeight = m_view->sizeHintForRow(0);

    if (itemHeight <= 0) {
        itemHeight = m_view->fontMetrics().height() + 8;
    }

    /*
     * Use the popup width to determine the number
     * of columns. The popup itself is the same width
     * as the combo.
     */
    const int width = qMax(1, m_view->viewport()->width());

    int columns = qMax(1, width / 100);

    const int cellWidth = qMax(1, width / columns);

    m_view->setGridSize(QSize(cellWidth, itemHeight));
}

void ComboBoxPopup::updatePopupSize()
{
    if (!m_sourceModel) {
        return;
    }

    const int itemCount = m_proxy->rowCount();

    const int margins = layout()->contentsMargins().top() + layout()->contentsMargins().bottom();

    const int spacing = layout()->spacing();

    const int searchHeight = m_searchable ? m_search->sizeHint().height() : 0;

    if (itemCount == 0) {
        const int height = searchHeight + margins;

        setFixedHeight(qMin(height, m_maximumHeight));

        return;
    }

    int itemHeight = m_view->sizeHintForRow(0);

    if (itemHeight <= 0) {
        itemHeight = m_view->fontMetrics().height() + 8;
    }

    int rows = itemCount;

    if (m_grid) {
        /*
         * Calculate the number of columns using
         * the actual popup width.
         */
        const int availableWidth = qMax(1, width() - margins);

        const int minimumCellWidth = 100;

        const int columns = qMax(1, availableWidth / minimumCellWidth);

        rows = (itemCount + columns - 1) / columns;
    }

    const int wantedHeight = searchHeight + (m_searchable ? spacing : 0) + rows * itemHeight
        + margins;

    setFixedHeight(qMin(wantedHeight, m_maximumHeight));

    m_view->doItemsLayout();
}

void ComboBoxPopup::popup(QWidget* relativeTo)
{
    if (!relativeTo) {
        return;
    }

    m_relativeTo = relativeTo;

    updateViewMode();

    /*
     * The popup has exactly the same width as
     * the combo box.
     */
    setFixedWidth(relativeTo->width());

    updateGridSize();
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

        /*
         * Prefer below the combo.
         */
        if (position.y() + height() > available.bottom()) {

            /*
             * Not enough room below, so put it
             * directly above the combo.
             */
            position.setY(comboTopLeft.y() - height());
        }

        /*
         * Keep the popup on-screen horizontally.
         */
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

    /*
     * The popup itself receives keyboard focus.
     */
    setFocus(Qt::PopupFocusReason);

    if (m_searchable) {
        m_search->setFocus(Qt::PopupFocusReason);
        m_search->selectAll();
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

    /*
     * Enter activates the current result.
     */
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

    /*
     * Escape closes the popup.
     */
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }

    /*
     * If searchable, any printable keyboard input
     * goes into the search field.
     *
     * This makes typing work even when the popup
     * itself currently has focus.
     */
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
