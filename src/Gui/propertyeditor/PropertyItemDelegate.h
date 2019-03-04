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


#ifndef PROPERTYITEMDELEGATE_H
#define PROPERTYITEMDELEGATE_H

#include <QItemDelegate>

namespace Gui {
namespace PropertyEditor {

class PropertyItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    PropertyItemDelegate(QObject* parent);
    ~PropertyItemDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const;
    virtual QWidget * createEditor (QWidget *, const QStyleOptionViewItem&, const QModelIndex&) const;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex& index ) const;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual bool editorEvent (QEvent *event, QAbstractItemModel *model,
                              const QStyleOptionViewItem& option, const QModelIndex& index);

public Q_SLOTS:
    void valueChanged();
    void editorClosed (QWidget * editor, QAbstractItemDelegate::EndEditHint hint);

private:
    mutable bool pressed;
};

} // namespace PropertyEditor
} // namespace Gui

#endif // PROPERTYITEMDELEGATE_H
