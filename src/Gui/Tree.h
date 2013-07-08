/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TREE_H
#define GUI_TREE_H

#include <QTreeWidget>

#include <App/Document.h>
#include <App/Application.h>

#include <Gui/DockWindow.h>
#include <Gui/Selection.h>


namespace Gui {

class ViewProviderDocumentObject;
class DocumentObjectItem;
class DocumentItem;

/// highlight modes for the tree items
enum HighlightMode {    Underlined,
                        Italic    ,
                        Overlined ,
                        Bold      ,
                        Blue      
};

/// highlight modes for the tree items
enum TreeItemMode {  Expand,
                     Collapse,
                     Toggle
};


/** Tree view that allows drag & drop of document objects.
 * @author Werner Mayer
 */
class TreeWidget : public QTreeWidget, public SelectionObserver
{
    Q_OBJECT

public:
    TreeWidget(QWidget* parent=0);
    ~TreeWidget();

    void scrollItemToTop(Gui::Document*);
    void setItemsSelected (const QList<QTreeWidgetItem *> items, bool select);

    static const int DocumentType;
    static const int ObjectType;

    void markItem(const App::DocumentObject* Obj,bool mark);

protected:
    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges& msg);
    void contextMenuEvent (QContextMenuEvent * e);
    void drawRow(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data,
                      Qt::DropAction action);
    Qt::DropActions supportedDropActions () const;
    QMimeData * mimeData (const QList<QTreeWidgetItem *> items) const;
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    bool event(QEvent *e);
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent * event);

protected Q_SLOTS:
    void onCreateGroup();
    void onRelabelObject();
    void onActivateDocument(QAction*);
    void onStartEditing();
    void onFinishEditing();

private Q_SLOTS:
    void onItemSelectionChanged(void);
    void onItemEntered(QTreeWidgetItem * item);
    void onItemCollapsed(QTreeWidgetItem * item);
    void onItemExpanded(QTreeWidgetItem * item);
    void onTestStatus(void);

private:
    void slotNewDocument(const Gui::Document&);
    void slotDeleteDocument(const Gui::Document&);
    void slotRenameDocument(const Gui::Document&);
    void slotActiveDocument(const Gui::Document&);
    void slotRelabelDocument(const Gui::Document&);

    void changeEvent(QEvent *e);

private:
    QAction* createGroupAction;
    QAction* relabelObjectAction;
    QAction* finishEditingAction;
    QTreeWidgetItem* contextItem;

    QTreeWidgetItem* rootItem;
    QTimer* statusTimer;
    static QPixmap* documentPixmap;
    std::map<const Gui::Document*,DocumentItem*> DocumentMap;
    bool fromOutside;
};

/** The link between the tree and a document.
 * Every document in the application gets its associated DocumentItem which controls 
 * the visibility and the functions of the document.
 * \author Jürgen Riegel
 */
class DocumentItem : public QTreeWidgetItem
{
public:
    DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent);
    ~DocumentItem();

    const Gui::Document* document() const;
    void setObjectHighlighted(const char*, bool);
    void setObjectSelected(const char*, bool);
    void clearSelection(void);
    void updateSelection(void);
    void selectItems(void);
    void testStatus(void);
    void setData(int column, int role, const QVariant & value);

protected:
    /** Adds a view provider to the document item.
     * If this view provider is already added nothing happens.
     */
    void slotNewObject(const Gui::ViewProviderDocumentObject&);
    /** Removes a view provider from the document item.
     * If this view provider is not added nothing happens.
     */
    void slotDeleteObject    (const Gui::ViewProviderDocumentObject&);
    void slotChangeObject    (const Gui::ViewProviderDocumentObject&);
    void slotRenameObject    (const Gui::ViewProviderDocumentObject&);
    void slotActiveObject    (const Gui::ViewProviderDocumentObject&);
    void slotInEdit          (const Gui::ViewProviderDocumentObject&);
    void slotResetEdit       (const Gui::ViewProviderDocumentObject&);
    void slotHighlightObject (const Gui::ViewProviderDocumentObject&,const Gui::HighlightMode&,bool);
    void slotExpandObject    (const Gui::ViewProviderDocumentObject&,const Gui::TreeItemMode&);

private:
    const Gui::Document* pDocument;
    std::map<std::string,DocumentObjectItem*> ObjectMap;
};

/** The link between the tree and a document object.
 * Every object in the document gets its associated DocumentObjectItem which controls 
 * the visibility and the functions of the object.
 * @author Werner Mayer
 */
class DocumentObjectItem : public QTreeWidgetItem
{
public:
    DocumentObjectItem(Gui::ViewProviderDocumentObject* pcViewProvider, QTreeWidgetItem * parent);
    ~DocumentObjectItem();

    Gui::ViewProviderDocumentObject* object() const;
    void testStatus();
    void displayStatusInfo();
    void setExpandedStatus(bool);
    void setData(int column, int role, const QVariant & value);

protected:
    void slotChangeIcon();
    void slotChangeToolTip(const QString&);
    void slotChangeStatusTip(const QString&);

private:
    typedef boost::BOOST_SIGNALS_NAMESPACE::connection Connection;
    int previousStatus;
    Gui::ViewProviderDocumentObject* viewObject;
    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    friend class TreeWidget;
};

/**
 * The dock window containing the tree view.
 * @author Werner Mayer
 */
class TreeDockWidget : public Gui::DockWindow
{
    Q_OBJECT

public:
    TreeDockWidget(Gui::Document*  pcDocument,QWidget *parent=0);
    ~TreeDockWidget();

private:
    QTreeWidget* treeWidget;
};

}


#endif // GUI_TREE_H

