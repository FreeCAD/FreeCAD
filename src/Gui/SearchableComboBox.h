#pragma once

#include <QComboBox>

namespace Gui
{

class ComboBoxPopup;

class GuiExport SearchableComboBox: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(bool searchable READ isSearchable WRITE setSearchable NOTIFY searchableChanged)
    Q_PROPERTY(bool grid READ isGrid WRITE setGrid NOTIFY gridChanged)
    Q_PROPERTY(int popupMaximumHeight READ popupMaximumHeight WRITE setPopupMaximumHeight)
    Q_PROPERTY(int gridFixedColumns READ gridFixedColumns WRITE setGridFixedColumns)
    Q_PROPERTY(bool popupScrollBar READ popupScrollBar WRITE setPopupScrollBar)

public:
    explicit SearchableComboBox(QWidget* parent = nullptr);
    ~SearchableComboBox() override;

    bool isSearchable() const;
    void setSearchable(bool searchable);

    bool isGrid() const;
    void setGrid(bool grid);

    int popupMaximumHeight() const;
    void setPopupMaximumHeight(int height);

    int gridFixedColumns() const;
    void setGridFixedColumns(int columns);

    bool popupScrollBar() const;
    void setPopupScrollBar(bool enabled);

    void showPopup() override;
    void hidePopup() override;

Q_SIGNALS:
    void searchableChanged(bool searchable);
    void gridChanged(bool grid);

private:
    ComboBoxPopup* m_popup = nullptr;

    bool m_searchable = true;
    bool m_grid = false;

    int m_popupMaximumHeight = 400;

    // -1 = automatic column count.
    int m_gridFixedColumns = -1;

    bool m_popupScrollBar = true;
};

}  // namespace Gui
