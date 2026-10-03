#pragma once

#include <QComboBox>

namespace Gui
{

class ComboBoxPopup;

class SearchableComboBox: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(bool searchable READ isSearchable WRITE setSearchable NOTIFY searchableChanged)

    Q_PROPERTY(bool grid READ isGrid WRITE setGrid NOTIFY gridChanged)

    Q_PROPERTY(int popupMaximumHeight READ popupMaximumHeight WRITE setPopupMaximumHeight)

public:
    explicit SearchableComboBox(QWidget* parent = nullptr);
    ~SearchableComboBox() override;

    bool isSearchable() const;
    void setSearchable(bool searchable);

    bool isGrid() const;
    void setGrid(bool grid);

    int popupMaximumHeight() const;
    void setPopupMaximumHeight(int height);

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
};

}  // namespace Gui
