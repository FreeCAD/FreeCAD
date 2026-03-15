/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QWidget>
#include <FCGlobal.h>

class QToolBox;

namespace Gui
{

namespace DockWnd
{

/** The ToolBox class provides a column of tabbed widget items.
 * A toolbox is a widget that displays a column of tabs one above the other, with the current item
 * displayed below the current tab. Every tab has an index position within the column of tabs. A
 * tab's item is a QWidget.
 *
 * Each item has an itemLabel(), an optional icon, itemIconSet(), an optional itemToolTip(), and a
 * widget. The item's attributes can be changed with setItemLabel(), setItemIconSet() and
 * setItemToolTip().
 *
 * Items are added using addItem(), or inserted at particular positions using insertItem(). The
 * total number of items is given by count(). Items can be deleted with delete, or removed from the
 * toolbox with removeItem(). Combining removeItem() and insertItem() allows one to move items to
 * different positions.
 *
 * The current item widget is returned by currentItem() and set with setCurrentItem(). If you prefer
 * you can work in terms of indexes using currentIndex(), setCurrentIndex(), indexOf() and item().
 *
 * The currentChanged() signal is emitted when the current item is changed.
 *
 * \a Note: This class implements the same API as the QToolBox does provided by Qt unless
 * removeItem(), which also deletes the item to be removed.
 * \author Werner Mayer
 */
class GuiExport ToolBox: public QWidget
{
    Q_OBJECT

public:
    ToolBox(QWidget* parent = nullptr);
    ~ToolBox() override;

    int addItem(QWidget* w, const QString& label);
    int addItem(QWidget* item, const QIcon& iconSet, const QString& label);
    int insertItem(int index, QWidget* item, const QString& label);
    int insertItem(int index, QWidget* item, const QIcon& iconSet, const QString& label);

    void removeItem(int index);

    void setItemEnabled(int index, bool enabled);
    bool isItemEnabled(int index) const;

    void setItemText(int index, const QString& label);
    QString itemText(int index) const;

    void setItemIcon(int index, const QIcon& iconSet);
    QIcon itemIcon(int index) const;

    void setItemToolTip(int index, const QString& toolTip);
    QString itemToolTip(int index) const;

    QWidget* currentWidget() const;

    int currentIndex() const;
    QWidget* widget(int index) const;
    int indexOf(QWidget* item) const;
    int count() const;

public Q_SLOTS:
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget* item);

protected:
    void changeEvent(QEvent* e) override;

Q_SIGNALS:
    /** This signal is emitted when the current item changed.
     * The new current item's index is passed in index, or -1 if there is no current item.
     */
    void currentChanged(int index);

private:
    QToolBox* _pToolBox;
};

}  // namespace DockWnd
}  // namespace Gui
