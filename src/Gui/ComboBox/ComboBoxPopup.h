#pragma once

#include <QFrame>

class QAbstractItemModel;
class QHideEvent;
class QKeyEvent;
class QLineEdit;
class QListView;

namespace Gui
{

class ComboBoxFilterModel;

class ComboBoxPopup: public QFrame
{
    Q_OBJECT

public:
    explicit ComboBoxPopup(QWidget* parent = nullptr);

    void setSourceModel(QAbstractItemModel* model);

    void setSearchable(bool searchable);
    void setGrid(bool grid);

    void setCurrentIndex(int index);
    void setMaximumPopupHeight(int height);

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

    QLineEdit* m_search = nullptr;
    QListView* m_view = nullptr;

    ComboBoxFilterModel* m_proxy = nullptr;

    QAbstractItemModel* m_sourceModel = nullptr;

    bool m_searchable = true;
    bool m_grid = false;

    int m_currentSourceRow = -1;
    int m_maximumHeight = 400;

    QWidget* m_relativeTo = nullptr;
};

}  // namespace Gui
