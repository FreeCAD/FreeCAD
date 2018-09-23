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
#include <QTime>

#include <App/Document.h>
#include <App/Application.h>

#include <Gui/DockWindow.h>
#include <Gui/Selection.h>


namespace Gui {

class ViewProviderDocumentObject;
class DocumentObjectItem;
class DocumentObjectData;
typedef std::shared_ptr<DocumentObjectData> DocumentObjectDataPtr;

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
    TreeWidget(const char *name, QWidget* parent=0);
    ~TreeWidget();

    void scrollItemToTop(Gui::Document*);
    void selectAllInstances(const ViewProviderDocumentObject &vpd);
    void selectLinkedObject(App::DocumentObject *linked); 
    void selectAllLinks(App::DocumentObject *obj); 
    void expandSelectedItems(TreeItemMode mode);

    /* Return a list of selected object of a give document and their parent
     *
     * This function can return the non-group parent of the selected object,
     * which Gui::Selection() cannot provide.
     */
    static std::vector<std::pair<ViewProviderDocumentObject*,ViewProviderDocumentObject*> > 
        getSelection(App::Document *doc);

    static const int DocumentType;
    static const int ObjectType;

    void markItem(const App::DocumentObject* Obj,bool mark);
    void syncView(ViewProviderDocumentObject *vp);

    static void toggleSyncView(bool enable);
    static bool checkSyncView();

    const char *getTreeName() const;

    static void updateStatus(bool delay=false);

    DocumentItem *getDocumentItem(const Gui::Document *) const;

    void startDragging();

protected:
    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges& msg);
    void syncSelection(const char *pDocName=0);
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

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void leaveEvent(QEvent *) override;
    void _updateStatus(bool delay=false);

protected Q_SLOTS:
    void onCreateGroup();
    void onRelabelObject();
    void onActivateDocument(QAction*);
    void onStartEditing();
    void onFinishEditing();
    void onSkipRecompute(bool on);
    void onAllowPartialRecompute(bool on);
    void onReloadDoc();
    void onMarkRecompute();
    void onRecomputeObject();
    void onSyncSelection();
    void onPreSelection();
    void onPreSelectTimer();
    void onSyncView();
    void onSyncPlacement();
    void onShowHidden();
    void onHideInTree();

private Q_SLOTS:
    void onItemSelectionChanged(void);
    void onItemEntered(QTreeWidgetItem * item);
    void onItemCollapsed(QTreeWidgetItem * item);
    void onItemExpanded(QTreeWidgetItem * item);
    void onUpdateStatus(void);

private:
    void slotNewDocument(const Gui::Document&);
    void slotDeleteDocument(const Gui::Document&);
    void slotRenameDocument(const Gui::Document&);
    void slotActiveDocument(const Gui::Document&);
    void slotRelabelDocument(const Gui::Document&);
    void slotShowHidden(const Gui::Document &);
    void slotChangedViewObject(const Gui::ViewProvider &, const App::Property &);
    void slotFinishRestoreDocument(const App::Document&);

    void changeEvent(QEvent *e);
    void setupText();

private:
    QAction* createGroupAction;
    QAction* relabelObjectAction;
    QAction* finishEditingAction;
    QAction* skipRecomputeAction;
    QAction* allowPartialRecomputeAction;
    QAction* markRecomputeAction;
    QAction* recomputeObjectAction;
    QAction* preSelectionAction;
    QAction* syncSelectionAction;
    QAction* syncViewAction;
    QAction* syncPlacementAction;
    QAction* showHiddenAction;
    QAction* hideInTreeAction;
    QAction* reloadDocAction;
    QTreeWidgetItem* contextItem;
    DocumentObjectItem *editingItem;
    DocumentItem *currentDocItem;
    QTreeWidgetItem* rootItem;
    QTimer* statusTimer;
    QTimer* preselectTimer;
    QTime preselectTime;
    static std::unique_ptr<QPixmap> documentPixmap;
    static std::unique_ptr<QPixmap> documentPartialPixmap;
    std::map<const Gui::Document*,DocumentItem*> DocumentMap;
    bool fromOutside;
    int statusUpdateDelay;

    std::string myName; // for debugging purpose

    friend class DocumentItem;
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
    void clearSelection(DocumentObjectItem *exclude=0);
    void updateSelection(QTreeWidgetItem *, bool unselect=false);
    void updateSelection();
    void updateItemSelection(DocumentObjectItem *);
    void selectItems(bool sync);
    void testStatus(void);
    void setData(int column, int role, const QVariant & value);
    void populateItem(DocumentObjectItem *item, bool refresh = false);
    bool populateObject(App::DocumentObject *obj);
    void selectAllInstances(const ViewProviderDocumentObject &vpd);
    bool showItem(DocumentObjectItem *item, bool select, bool force=false);
    void updateItemsVisibility(QTreeWidgetItem *item, bool show);
    void setItemVisibility(const Gui::ViewProviderDocumentObject&);
    void updateLinks(const ViewProviderDocumentObject &view);
    ViewProviderDocumentObject *getViewProvider(App::DocumentObject *);
    void onDeleteDocument(DocumentItem *docItem);
    void checkRemoveChildrenFromRoot(const ViewProviderDocumentObject& view);

    bool showHidden() const;
    void setShowHidden(bool show);

    TreeWidget *getTree() const;
    const char *getTreeName() const;

protected:
    /** Adds a view provider to the document item.
     * If this view provider is already added nothing happens.
     */
    void slotNewObject(const Gui::ViewProviderDocumentObject&);
    /** Removes a view provider from the document item.
     * If this view provider is not added nothing happens.
     */
    void slotDeleteObject    (const Gui::ViewProviderDocumentObject&, bool boradcast);
    void slotChangeObject    (const Gui::ViewProviderDocumentObject&, const App::Property &prop);
    void slotRenameObject    (const Gui::ViewProviderDocumentObject&);
    void slotActiveObject    (const Gui::ViewProviderDocumentObject&);
    void slotInEdit          (const Gui::ViewProviderDocumentObject&);
    void slotResetEdit       (const Gui::ViewProviderDocumentObject&);
    void slotHighlightObject (const Gui::ViewProviderDocumentObject&,const Gui::HighlightMode&,bool,
                              const App::DocumentObject *parent, const char *subname);
    void slotExpandObject    (const Gui::ViewProviderDocumentObject&,const Gui::TreeItemMode&);
    void slotScrollToObject  (const Gui::ViewProviderDocumentObject&);
    void slotRecomputed      (const App::Document &doc, const std::vector<App::DocumentObject*> &objs);
    void slotTransactionDone (const App::Document &doc);

    bool updateObject(const Gui::ViewProviderDocumentObject&, const App::Property &prop);

    bool createNewItem(const Gui::ViewProviderDocumentObject&, 
                    QTreeWidgetItem *parent=0, int index=-1, 
                    DocumentObjectDataPtr ptrs = DocumentObjectDataPtr());

    void findSelection(bool sync, DocumentObjectItem *item, const char *subname);

    typedef std::map<const ViewProvider *, std::vector<ViewProviderDocumentObject*> > ParentMap;
    void populateParents(const ViewProvider *vp, ParentMap &);

private:
    const char *treeName; // for debugging purpose
    const Gui::Document* pDocument;
    std::map<App::DocumentObject*,DocumentObjectDataPtr> ObjectMap;
    std::vector<long> TransactingObjects;

    typedef boost::BOOST_SIGNALS_NAMESPACE::connection Connection;
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
    Connection connectRecomputed;
    Connection connectUndo;
    Connection connectRedo;

    friend class TreeWidget;
};

/** The link between the tree and a document object.
 * Every object in the document gets its associated DocumentObjectItem which controls
 * the visibility and the functions of the object.
 * @author Werner Mayer
 */
class DocumentObjectItem : public QTreeWidgetItem
{
public:
    DocumentObjectItem(DocumentItem *ownerDocItem, DocumentObjectDataPtr data);
    ~DocumentObjectItem();

    Gui::ViewProviderDocumentObject* object() const;
    void testStatus(bool resetStatus, QIcon &icon1, QIcon &icon2);
    void testStatus(bool resetStatus);
    void displayStatusInfo();
    void setExpandedStatus(bool);
    void setData(int column, int role, const QVariant & value);
    bool isChildOfItem(DocumentObjectItem*);

    // Get the parent document (where the object is stored) of this item
    DocumentItem *getParentDocument() const;
    // Get the owner document (where the object is displayed, either stored or
    // linked in) of this object
    DocumentItem *getOwnerDocument() const;

    // check if a new item is required at root
    bool requiredAtRoot(bool excludeSelf=true) const;
    
    // return the owner, and full quanlified subname
    App::DocumentObject *getFullSubName(std::ostringstream &str,
            DocumentObjectItem *parent = 0) const;

    // return the immediate decendent of the common ancestor of this item and
    // 'cousin'.
    App::DocumentObject *getRelativeParent(
            std::ostringstream &str,
            DocumentObjectItem *cousin, 
            App::DocumentObject **topParent=0,
            std::string *topSubname=0) const;

    // return the top most linked group owner's name, and subname.  This method
    // is necssary despite have getFullSubName above is because native geo group
    // cannot handle selection with sub name. So only a linked group can have
    // subname in selection
    int getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const;

    void setHighlight(bool set, Gui::HighlightMode mode = Gui::LightBlue);

    const char *getName() const;
    const char *getTreeName() const;

    bool isLink() const;
    bool isLinkFinal() const;
    bool isParentLink() const;
    int isGroup() const;
    int isParentGroup() const;

    DocumentObjectItem *getParentItem() const;
    TreeWidget *getTree() const;

private:
    DocumentItem *myOwner;
    DocumentObjectDataPtr myData;
    std::vector<std::string> mySubs;
    int previousStatus;
    int selected;
    bool populated;

    friend class TreeWidget;
    friend class DocumentItem;
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
