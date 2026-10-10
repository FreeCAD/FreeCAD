#pragma once

#include <FCGlobal.h>
#include <QComboBox>

namespace Gui
{

class LargeComboBoxPopup;

class GuiExport LargeComboBox: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(bool searchable READ isSearchable WRITE setSearchable NOTIFY searchableChanged)
    Q_PROPERTY(bool grid READ isGrid WRITE setGrid NOTIFY gridChanged)
    Q_PROPERTY(int popupMaximumHeight READ popupMaximumHeight WRITE setPopupMaximumHeight)
    Q_PROPERTY(int gridFixedColumns READ gridFixedColumns WRITE setGridFixedColumns)
    Q_PROPERTY(bool popupScrollBar READ popupScrollBar WRITE setPopupScrollBar)
    Q_PROPERTY(int gridRowCount READ gridRowCount WRITE setGridFixedColumns)

public:
    explicit LargeComboBox(QWidget* parent = nullptr);
    ~LargeComboBox() override;

    bool isSearchable() const;
    void setSearchable(bool searchable);

    bool isGrid() const;
    void setGrid(bool grid);

    int popupMaximumHeight() const;
    void setPopupMaximumHeight(int height);

    int gridFixedColumns() const;
    void setGridFixedColumns(int columns);

    int gridRowCount() const;
    void setGridRowCount(int rows);

    bool popupScrollBar() const;
    void setPopupScrollBar(bool enabled);

    void showPopup() override;
    void hidePopup() override;

    bool isPopupShown() const;

Q_SIGNALS:
    void searchableChanged(bool searchable);
    void gridChanged(bool grid);

private:
    LargeComboBoxPopup* m_popup = nullptr;

    bool m_searchable = true;
    bool m_grid = false;

    int m_popupMaximumHeight = 400;

    // -1 = automatic column count.
    int m_gridFixedColumns = -1;
    int m_gridRowCount = 10;

    bool m_popupScrollBar = false;
};

}  // namespace Gui
