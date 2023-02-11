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

#include <unordered_set>

#include <QTreeView>

#include "PropertyItem.h"
#include "PropertyModel.h"


namespace App {
class Property;
class Document;
}

namespace Gui {

class PropertyView;

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

    Q_PROPERTY(QBrush groupBackground READ groupBackground WRITE setGroupBackground DESIGNABLE true SCRIPTABLE true) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QColor groupTextColor READ groupTextColor WRITE setGroupTextColor DESIGNABLE true SCRIPTABLE true) // clazy:exclude=qproperty-without-notify

public:
    PropertyEditor(QWidget *parent = nullptr);
    ~PropertyEditor() override;

    /** Builds up the list view with the properties. */
    void buildUp(PropertyModel::PropertyList &&props = PropertyModel::PropertyList(), bool checkDocument=false);
    void updateProperty(const App::Property&);
    void removeProperty(const App::Property&);
    void setAutomaticExpand(bool);
    bool isAutomaticExpand(bool) const;
    void setAutomaticDocumentUpdate(bool);
    bool isAutomaticDocumentUpdate(bool) const;
    /*! Reset the internal state of the view. */
    void reset() override;

    QBrush groupBackground() const;
    void setGroupBackground(const QBrush& c);
    QColor groupTextColor() const;
    void setGroupTextColor(const QColor& c);

    bool isBinding() const { return binding; }
    void openEditor(const QModelIndex &index);
    void closeEditor();

protected Q_SLOTS:
    void onItemActivated(const QModelIndex &index);
    void onItemExpanded(const QModelIndex &index);
    void onItemCollapsed(const QModelIndex &index);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &dst, int row);
    void onRowsRemoved(const QModelIndex &parent, int start, int end);

protected:
    void closeEditor (QWidget * editor, QAbstractItemDelegate::EndEditHint hint) override;
    void commitData (QWidget * editor) override;
    void editorDestroyed (QObject * editor) override;
    void currentChanged (const QModelIndex & current, const QModelIndex & previous) override;
    void rowsInserted (const QModelIndex & parent, int start, int end) override;
    void rowsAboutToBeRemoved (const QModelIndex & parent, int start, int end) override;
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QStyleOptionViewItem viewOptions() const override;
#else
    void initViewItemOption(QStyleOptionViewItem *option) const override;
#endif
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool event(QEvent*) override;

private:
    void setEditorMode(const QModelIndex & parent, int start, int end);
    void closeTransaction();
    void recomputeDocument(App::Document*);

private:
    PropertyItemDelegate *delegate;
    PropertyModel* propertyModel;
    QStringList selectedProperty;
    PropertyModel::PropertyList propList;
    std::unordered_set<const App::PropertyContainer*> propOwners;
    bool autoexpand;
    bool autoupdate;
    bool committing;
    bool delaybuild;
    bool binding;
    bool checkDocument;
    bool closingEditor;

    int transactionID = 0;

    QColor groupColor;
    QBrush background;

    QPointer<QWidget> activeEditor;
    QPersistentModelIndex editingIndex;
    int removingRows = 0;

    friend class Gui::PropertyView;
    friend class PropertyItemDelegate;
};

} //namespace PropertyEditor
} //namespace Gui

#endif // PROPERTYEDITOR_H
