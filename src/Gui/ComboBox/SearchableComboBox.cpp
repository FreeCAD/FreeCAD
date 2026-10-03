#include "SearchableComboBox.h"
#include "ComboBoxPopup.h"

using namespace Gui;

SearchableComboBox::SearchableComboBox(QWidget* parent)
    : QComboBox(parent)
{
    // combo box shoudldn't follow popup size
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    m_popup = new ComboBoxPopup(nullptr);

    connect(m_popup, &ComboBoxPopup::itemSelected, this, [this](int sourceRow) {
        if (sourceRow < 0 || sourceRow >= count()) {
            return;
        }

        setCurrentIndex(sourceRow);
    });

    connect(m_popup, &ComboBoxPopup::popupClosed, this, []() {
        // Do not invoke QComboBox::hidePopup() here.
    });
}

SearchableComboBox::~SearchableComboBox()
{
    delete m_popup;
}

bool SearchableComboBox::isSearchable() const
{
    return m_searchable;
}

void SearchableComboBox::setSearchable(bool searchable)
{
    if (m_searchable == searchable) {
        return;
    }

    m_searchable = searchable;

    m_popup->setSearchable(searchable);

    Q_EMIT searchableChanged(searchable);
}

bool SearchableComboBox::isGrid() const
{
    return m_grid;
}

void SearchableComboBox::setGrid(bool grid)
{
    if (m_grid == grid) {
        return;
    }

    m_grid = grid;

    m_popup->setGrid(grid);

    Q_EMIT gridChanged(grid);
}

int SearchableComboBox::popupMaximumHeight() const
{
    return m_popupMaximumHeight;
}

void SearchableComboBox::setPopupMaximumHeight(int height)
{
    height = qMax(100, height);

    if (m_popupMaximumHeight == height) {
        return;
    }

    m_popupMaximumHeight = height;

    m_popup->setMaximumPopupHeight(height);
}

void SearchableComboBox::showPopup()
{
    m_popup->setSourceModel(model());
    m_popup->setSearchable(m_searchable);
    m_popup->setGrid(m_grid);
    m_popup->setMaximumPopupHeight(m_popupMaximumHeight);
    m_popup->setGridFixedColumns(m_gridFixedColumns);
    m_popup->setPopupScrollBar(m_popupScrollBar);
    m_popup->setCurrentIndex(currentIndex());

    m_popup->popup(this);
}

void SearchableComboBox::hidePopup()
{
    if (m_popup && m_popup->isVisible()) {
        m_popup->hide();
    }
}

int SearchableComboBox::gridFixedColumns() const
{
    return m_gridFixedColumns;
}

void SearchableComboBox::setGridFixedColumns(int columns)
{
    if (columns < 1) {
        columns = -1;
    }

    if (m_gridFixedColumns == columns) {
        return;
    }

    m_gridFixedColumns = columns;

    m_popup->setGridFixedColumns(columns);
}

bool SearchableComboBox::popupScrollBar() const
{
    return m_popupScrollBar;
}

void SearchableComboBox::setPopupScrollBar(bool enabled)
{
    if (m_popupScrollBar == enabled) {
        return;
    }

    m_popupScrollBar = enabled;

    m_popup->setPopupScrollBar(enabled);
}
