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


#ifndef PROPERTYEDITOR_H
#define PROPERTYEDITOR_H

#include <map>
#include <string>
#include <vector>

#include <QTreeView>

#include <App/DocumentObserver.h>
#include "PropertyItem.h"
#include "PropertyModel.h"

namespace App {
class Property;
}

namespace Gui {
namespace PropertyEditor {

class PropertyItemDelegate;
class PropertyModel;
/*!
 Put this into the .qss file after Gui--PropertyEditor--PropertyEditor
 
 Gui--PropertyEditor--PropertyEditor
 {
    qproperty-groupBackground: gray;
    qproperty-groupTextColor: white;
 }

 See also: https://man42.net/blog/2011/09/qt-4-7-modify-a-custom-q_property-with-a-qt-style-sheet/

*/

class PropertyEditor : public QTreeView
{
    Q_OBJECT

    Q_PROPERTY(QBrush groupBackground READ groupBackground WRITE setGroupBackground DESIGNABLE true SCRIPTABLE true)
    Q_PROPERTY(QColor groupTextColor READ groupTextColor WRITE setGroupTextColor DESIGNABLE true SCRIPTABLE true)

public:
    PropertyEditor(QWidget *parent = 0);
    ~PropertyEditor();

    /** Builds up the list view with the properties. */
    void buildUp(PropertyModel::PropertyList &&props = PropertyModel::PropertyList());
    void updateProperty(const App::Property&);
    void updateEditorMode(const App::Property&);
    void appendProperty(const App::Property&);
    void removeProperty(const App::Property&);
    void setAutomaticDocumentUpdate(bool);
    bool isAutomaticDocumentUpdate(bool) const;
    /*! Reset the internal state of the view. */
    virtual void reset();

    QBrush groupBackground() const;
    void setGroupBackground(const QBrush& c);
    QColor groupTextColor() const;
    void setGroupTextColor(const QColor& c);

    bool isBinding() const { return binding; }

protected Q_SLOTS:
    void onItemActivated(const QModelIndex &index);
    void onMenuAction(QAction *);

protected:
    virtual void closeEditor (QWidget * editor, QAbstractItemDelegate::EndEditHint hint);
    virtual void commitData (QWidget * editor);
    virtual void editorDestroyed (QObject * editor);
    virtual void currentChanged (const QModelIndex & current, const QModelIndex & previous);
    virtual void rowsInserted (const QModelIndex & parent, int start, int end);
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual bool event(QEvent*);

private:
    void setEditorMode(const QModelIndex & parent, int start, int end);
    void updateItemEditor(bool enable, int column, const QModelIndex& parent);

private:
    PropertyItemDelegate *delegate;
    PropertyModel* propertyModel;
    QStringList selectedProperty;
    PropertyModel::PropertyList propList;
    bool autoupdate;
    bool committing;
    bool delaybuild;
    QColor groupColor;
    QBrush background;

    App::DocumentObjectT context;
    QModelIndex contextIndex;
    bool binding;
};

} //namespace PropertyEditor
} //namespace Gui

#endif // PROPERTYEDITOR_H
