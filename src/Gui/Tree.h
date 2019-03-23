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

class QLineEdit;

namespace Gui {

class ViewProviderDocumentObject;
class DocumentObjectItem;
typedef std::set<DocumentObjectItem*> DocumentObjectItems;
typedef std::shared_ptr<DocumentObjectItems> DocumentObjectItemsPtr;
class DocumentItem;

/// highlight modes for the tree items
enum HighlightMode {  Underlined,
                      Italic,
                      Overlined,
                      Bold,
                      Blue,
                      LightBlue,
                      UserDefined
};

/// highlight modes for the tree items
enum TreeItemMode {  ExpandItem,
                     ExpandPath,
                     CollapseItem,
                     ToggleItem
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
    /** @name Drag and drop */
    //@{
    void startDrag(Qt::DropActions supportedActions);
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data,
                      Qt::DropAction action);
    Qt::DropActions supportedDropActions () const;
    QMimeData * mimeData (const QList<QTreeWidgetItem *> items) const;
    void dragEnterEvent(QDragEnterEvent * event);
    void dragLeaveEvent(QDragLeaveEvent * event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    //@}
    bool event(QEvent *e);
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    QList<App::DocumentObject *> buildListChildren(QTreeWidgetItem* targetitem,
                                                   Gui::ViewProviderDocumentObject* vp);

protected Q_SLOTS:
    void onCreateGroup();
    void onRelabelObject();
    void onActivateDocument(QAction*);
    void onStartEditing();
    void onFinishEditing();
    void onSkipRecompute(bool on);
    void onMarkRecompute();
    void onSearchObjects();

private Q_SLOTS:
    void onItemSelectionChanged(void);
    void onItemEntered(QTreeWidgetItem * item);
    void onItemCollapsed(QTreeWidgetItem * item);
    void onItemExpanded(QTreeWidgetItem * item);
    void onTestStatus(void);

Q_SIGNALS:
    void emitSearchObjects();

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
    QAction* skipRecomputeAction;
    QAction* markRecomputeAction;
    QAction* searchObjectsAction;
    QTreeWidgetItem* contextItem;

    QTreeWidgetItem* rootItem;
    QTimer* statusTimer;
    static QPixmap* documentPixmap;
    std::map<const Gui::Document*,DocumentItem*> DocumentMap;
    bool fromOutside;

    typedef boost::signals2::connection Connection;
    Connection connectNewDocument;
    Connection connectDelDocument;
    Connection connectRenDocument;
    Connection connectActDocument;
    Connection connectRelDocument;
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
    void populateItem(DocumentObjectItem *item, bool refresh = false);

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
    void slotScrollToObject  (const Gui::ViewProviderDocumentObject&);

    bool createNewItem(const Gui::ViewProviderDocumentObject&, 
                    QTreeWidgetItem *parent=0, int index=-1, 
                    DocumentObjectItemsPtr ptrs = DocumentObjectItemsPtr());
        
private:
    const Gui::Document* pDocument;
    std::map<std::string,DocumentObjectItemsPtr> ObjectMap;

    typedef boost::signals2::connection Connection;
    Connection connectNewObject;
    Connection connectDelObject;
    Connection connectChgObject;
    Connection connectRenObject;
    Connection connectActObject;
    Connection connectEdtObject;
    Connection connectResObject;
    Connection connectHltObject;
    Connection connectExpObject;
    Connection connectScrObject;
};

/** The link between the tree and a document object.
 * Every object in the document gets its associated DocumentObjectItem which controls
 * the visibility and the functions of the object.
 * @author Werner Mayer
 */
class DocumentObjectItem : public QTreeWidgetItem
{
public:
    DocumentObjectItem(Gui::ViewProviderDocumentObject* pcViewProvider, 
                       DocumentObjectItemsPtr selves);
    ~DocumentObjectItem();

    Gui::ViewProviderDocumentObject* object() const;
    void testStatus();
    void displayStatusInfo();
    void setExpandedStatus(bool);
    void setData(int column, int role, const QVariant & value);
    bool isChildOfItem(DocumentObjectItem*);

protected:
    void slotChangeIcon();
    void slotChangeToolTip(const QString&);
    void slotChangeStatusTip(const QString&);

private:
    typedef boost::signals2::connection Connection;
    int previousStatus;
    Gui::ViewProviderDocumentObject* viewObject;
    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    DocumentObjectItemsPtr myselves;
    bool populated;

    friend class TreeWidget;
    friend class DocumentItem;
};

class TreePanel : public QWidget
{
    Q_OBJECT

public:
    TreePanel(QWidget* parent=nullptr);
    virtual ~TreePanel();

    bool eventFilter(QObject *obj, QEvent *ev);

private Q_SLOTS:
    void accept();
    void showEditor();
    void hideEditor();
    void findMatchingItems(const QString&);

private:
    void searchTreeItem(QTreeWidgetItem* item, const QString& text);
    void selectTreeItem(QTreeWidgetItem* item, const QString& text);
    void resetBackground(QTreeWidgetItem* item);

private:
    QLineEdit* searchBox;
    QTreeWidget* treeWidget;
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

