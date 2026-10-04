#pragma once

#include <QFrame>

class QAbstractItemModel;
class QHideEvent;
class QKeyEvent;
class QLineEdit;
class QListView;
class QTableView;

namespace Gui
{

class LargeComboBoxFilterModel;
class LargeComboBoxGridModel;

class LargeComboBoxPopup: public QFrame
{
    Q_OBJECT

public:
    explicit LargeComboBoxPopup(QWidget* parent = nullptr);

    void setSourceModel(QAbstractItemModel* model);

    void setSearchable(bool searchable);
    void setGrid(bool grid);

    void setCurrentIndex(int index);
    void setMaximumPopupHeight(int height);

    void setGridFixedColumns(int columns);
    void setPopupScrollBar(bool enabled);

    int selectedSourceRow() const;

    void popup(QWidget* relativeTo);

Q_SIGNALS:
    void itemSelected(int sourceRow);
    void popupClosed();

protected:
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateViewMode();
    void updatePopupSize();
    void updateGridSize();

    int calculatePopupWidth() const;
    int calculateItemHeight() const;
    int calculateGridColumns() const;
    int availablePopupHeight() const;

    QLineEdit* m_search = nullptr;
    QListView* m_view = nullptr;
    QTableView* m_gridView = nullptr;

    LargeComboBoxFilterModel* m_proxy = nullptr;
    LargeComboBoxGridModel* m_gridModel = nullptr;

    QAbstractItemModel* m_sourceModel = nullptr;

    bool m_searchable = true;
    bool m_grid = false;

    int m_currentSourceRow = -1;

    int m_maximumHeight = 400;

    // -1 = automatic.
    int m_gridFixedColumns = -1;
    // Actual column count for the currently displayed popup.
    int m_gridColumns = 1;

    bool m_popupScrollBar = true;

    QWidget* m_relativeTo = nullptr;
};

}  // namespace Gui
