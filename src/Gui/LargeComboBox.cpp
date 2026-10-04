#include "LargeComboBox.h"
#include "LargeComboBoxPopup.h"

using namespace Gui;

LargeComboBox::LargeComboBox(QWidget* parent)
    : QComboBox(parent)
{
    m_popup = new LargeComboBoxPopup(nullptr);

    connect(m_popup, &LargeComboBoxPopup::itemSelected, this, [this](int sourceRow) {
        if (sourceRow < 0 || sourceRow >= count()) {
            return;
        }

        setCurrentIndex(sourceRow);
    });

    connect(m_popup, &LargeComboBoxPopup::popupClosed, this, []() {
        // Do not invoke QComboBox::hidePopup() here.
    });
}

LargeComboBox::~LargeComboBox()
{
    delete m_popup;
}

bool LargeComboBox::isSearchable() const
{
    return m_searchable;
}

void LargeComboBox::setSearchable(bool searchable)
{
    if (m_searchable == searchable) {
        return;
    }

    m_searchable = searchable;

    m_popup->setSearchable(searchable);

    Q_EMIT searchableChanged(searchable);
}

bool LargeComboBox::isGrid() const
{
    return m_grid;
}

void LargeComboBox::setGrid(bool grid)
{
    if (m_grid == grid) {
        return;
    }

    m_grid = grid;

    m_popup->setGrid(grid);

    Q_EMIT gridChanged(grid);
}

int LargeComboBox::popupMaximumHeight() const
{
    return m_popupMaximumHeight;
}

void LargeComboBox::setPopupMaximumHeight(int height)
{
    height = qMax(100, height);

    if (m_popupMaximumHeight == height) {
        return;
    }

    m_popupMaximumHeight = height;

    m_popup->setMaximumPopupHeight(height);
}

void LargeComboBox::showPopup()
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

void LargeComboBox::hidePopup()
{
    if (m_popup && m_popup->isVisible()) {
        m_popup->hide();
    }
}

int LargeComboBox::gridFixedColumns() const
{
    return m_gridFixedColumns;
}

void LargeComboBox::setGridFixedColumns(int columns)
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

bool LargeComboBox::popupScrollBar() const
{
    return m_popupScrollBar;
}

void LargeComboBox::setPopupScrollBar(bool enabled)
{
    if (m_popupScrollBar == enabled) {
        return;
    }

    m_popupScrollBar = enabled;

    m_popup->setPopupScrollBar(enabled);
}
