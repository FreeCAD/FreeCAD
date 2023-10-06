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

#include <unordered_map>
#include <QElapsedTimer>
#include <QStyledItemDelegate>
#include <QTreeWidget>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Base/Persistence.h>
#include <Gui/DockWindow.h>
#include <Gui/Selection.h>
#include <Gui/TreeItemMode.h>

class QLineEdit;

namespace Gui {

class TreeParams;
class ViewProviderDocumentObject;
class DocumentObjectItem;
class DocumentObjectData;
using DocumentObjectDataPtr = std::shared_ptr<DocumentObjectData>;

class DocumentItem;

/** Tree view that allows drag & drop of document objects.
 * @author Werner Mayer
 */
class TreeWidget : public QTreeWidget, public SelectionObserver
{
    Q_OBJECT

public:
    explicit TreeWidget(const char *name, QWidget* parent=nullptr);
    ~TreeWidget() override;

    static void setupResizableColumn(TreeWidget *tree=nullptr);
    static void scrollItemToTop();
    void selectAllInstances(const ViewProviderDocumentObject &vpd);
    void selectLinkedObject(App::DocumentObject *linked);
    void selectAllLinks(App::DocumentObject *obj);
    void expandSelectedItems(TreeItemMode mode);

    bool eventFilter(QObject *, QEvent *ev) override;

    struct SelInfo {
        App::DocumentObject *topParent;
        std::string subname;
        ViewProviderDocumentObject *parentVp;
        ViewProviderDocumentObject *vp;
    };
    /* Return a list of selected object of a give document and their parent
     *
     * This function can return the non-group parent of the selected object,
     * which Gui::Selection() cannot provide.
     */
    static std::vector<SelInfo> getSelection(App::Document *doc=nullptr);

    static TreeWidget *instance();

    static const int DocumentType;
    static const int ObjectType;

    void markItem(const App::DocumentObject* Obj,bool mark);
    void syncView(ViewProviderDocumentObject *vp);

    void selectAll() override;

    const char *getTreeName() const;

    static void updateStatus(bool delay=true);

    static bool isObjectShowable(App::DocumentObject *obj);

    // Check if obj can be considered as a top level object
    static void checkTopParent(App::DocumentObject *&obj, std::string &subname);

    DocumentItem *getDocumentItem(const Gui::Document *) const;

    static Gui::Document *selectedDocument();

    void startDragging();

    void resetItemSearch();
    void startItemSearch(QLineEdit*);
    void itemSearch(const QString &text, bool select);

    static void synchronizeSelectionCheckBoxes();

    QList<QTreeWidgetItem *> childrenOfItem(const QTreeWidgetItem &item) const;

protected:
    /// Observer message from the Selection
    void onSelectionChanged(const SelectionChanges& msg) override;
    void contextMenuEvent (QContextMenuEvent * e) override;
    void drawRow(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    /** @name Drag and drop */
    //@{
    void startDrag(Qt::DropActions supportedActions) override;
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data,
                      Qt::DropAction action) override;
    Qt::DropActions supportedDropActions () const override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    //@}
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent * event) override;

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void leaveEvent(QEvent *) override;
    void _updateStatus(bool delay=true);

protected Q_SLOTS:
    void onCreateGroup();
    void onRelabelObject();
    void onActivateDocument(QAction*);
    void onStartEditing();
    void onFinishEditing();
    void onSelectDependents();
    void onSkipRecompute(bool on);
    void onAllowPartialRecompute(bool on);
    void onReloadDoc();
    void onCloseDoc();
    void onMarkRecompute();
    void onRecomputeObject();
    void onPreSelectTimer();
    void onSelectTimer();
    void onShowHidden();
    void onToggleVisibilityInTree();
    void onSearchObjects();

private Q_SLOTS:
    void onItemSelectionChanged();
    void onItemChanged(QTreeWidgetItem*, int);
    void onItemEntered(QTreeWidgetItem * item);
    void onItemCollapsed(QTreeWidgetItem * item);
    void onItemExpanded(QTreeWidgetItem * item);
    void onUpdateStatus();

Q_SIGNALS:
    void emitSearchObjects();

private:
    void slotNewDocument(const Gui::Document&, bool);
    void slotDeleteDocument(const Gui::Document&);
    void slotRenameDocument(const Gui::Document&);
    void slotActiveDocument(const Gui::Document&);
    void slotRelabelDocument(const Gui::Document&);
    void slotShowHidden(const Gui::Document &);
    void slotChangedViewObject(const Gui::ViewProvider &, const App::Property &);
    void slotStartOpenDocument();
    void slotFinishOpenDocument();
    void _slotDeleteObject(const Gui::ViewProviderDocumentObject&, DocumentItem *deletingDoc);
    void slotDeleteObject(const Gui::ViewProviderDocumentObject&);
    void slotChangeObject(const Gui::ViewProviderDocumentObject&, const App::Property &prop);
    void slotTouchedObject(const App::DocumentObject&);

    void changeEvent(QEvent *e) override;
    void setupText();

    void updateChildren(App::DocumentObject *obj,
            const std::set<DocumentObjectDataPtr> &data, bool output, bool force);

    bool CheckForDependents();
    void addDependentToSelection(App::Document* doc, App::DocumentObject* docObject);

private:
    QAction* createGroupAction;
    QAction* relabelObjectAction;
    QAction* finishEditingAction;
    QAction* selectDependentsAction;
    QAction* skipRecomputeAction;
    QAction* allowPartialRecomputeAction;
    QAction* markRecomputeAction;
    QAction* recomputeObjectAction;
    QAction* showHiddenAction;
    QAction* toggleVisibilityInTreeAction;
    QAction* reloadDocAction;
    QAction* closeDocAction;
    QAction* searchObjectsAction;
    QTreeWidgetItem *contextItem;
    App::DocumentObject *searchObject;
    Gui::Document *searchDoc;
    Gui::Document *searchContextDoc;
    DocumentObjectItem *editingItem;
    DocumentItem *currentDocItem;
    QTreeWidgetItem* rootItem;
    QTimer* statusTimer;
    QTimer* selectTimer;
    QTimer* preselectTimer;
    QElapsedTimer preselectTime;
    static std::unique_ptr<QPixmap> documentPixmap;
    static std::unique_ptr<QPixmap> documentPartialPixmap;
    std::unordered_map<const Gui::Document*,DocumentItem*> DocumentMap;
    std::unordered_map<App::DocumentObject*,std::set<DocumentObjectDataPtr> > ObjectTable;

    enum ChangedObjectStatus {
        CS_Output,
        CS_Error,
    };
    std::unordered_map<App::DocumentObject*,std::bitset<32> > ChangedObjects;

    std::unordered_map<std::string,std::vector<long> > NewObjects;

    static std::set<TreeWidget*> Instances;

    std::string myName; // for debugging purpose
    int updateBlocked = 0;

    friend class DocumentItem;
    friend class DocumentObjectItem;
    friend class TreeParams;

    using Connection = boost::signals2::connection;
    Connection connectNewDocument;
    Connection connectDelDocument;
    Connection connectRenDocument;
    Connection connectActDocument;
    Connection connectRelDocument;
    Connection connectShowHidden;
    Connection connectChangedViewObj;
};

/** The link between the tree and a document.
 * Every document in the application gets its associated DocumentItem which controls
 * the visibility and the functions of the document.
 * \author Jürgen Riegel
 */
class DocumentItem : public QTreeWidgetItem, public Base::Persistence
{
public:
    DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent);
    ~DocumentItem() override;

    Gui::Document* document() const;
    void clearSelection(DocumentObjectItem *exclude=nullptr);
    void updateSelection(QTreeWidgetItem *, bool unselect=false);
    void updateSelection();
    void updateItemSelection(DocumentObjectItem *);

    enum SelectionReason {
        SR_SELECT, // only select, no expansion
        SR_EXPAND, // select and expand but respect ObjectStatus::NoAutoExpand
        SR_FORCE_EXPAND, // select and force expansion
    };
    void selectItems(SelectionReason reason=SR_SELECT);

    void testStatus();
    void setData(int column, int role, const QVariant & value) override;
    void populateItem(DocumentObjectItem *item, bool refresh=false, bool delayUpdate=true);
    bool populateObject(App::DocumentObject *obj);
    void selectAllInstances(const ViewProviderDocumentObject &vpd);
    bool showItem(DocumentObjectItem *item, bool select, bool force=false);
    void updateItemsVisibility(QTreeWidgetItem *item, bool show);
    void updateLinks(const ViewProviderDocumentObject &view);
    ViewProviderDocumentObject *getViewProvider(App::DocumentObject *);

    bool showHidden() const;
    void setShowHidden(bool show);

    TreeWidget *getTree() const;
    const char *getTreeName() const;

    bool isObjectShowable(App::DocumentObject *obj);

    unsigned int getMemSize () const override;
    void Save (Base::Writer &) const override;
    void Restore(Base::XMLReader &) override;
    void Restore(Base::DocumentReader& reader) override;

    class ExpandInfo;
    using ExpandInfoPtr = std::shared_ptr<ExpandInfo>;

protected:
    /** Adds a view provider to the document item.
     * If this view provider is already added nothing happens.
     */
    void slotNewObject(const Gui::ViewProviderDocumentObject&);
    /** Removes a view provider from the document item.
     * If this view provider is not added nothing happens.
     */
    void slotInEdit          (const Gui::ViewProviderDocumentObject&);
    void slotResetEdit       (const Gui::ViewProviderDocumentObject&);
    void slotHighlightObject (const Gui::ViewProviderDocumentObject&,const Gui::HighlightMode&,bool,
                              const App::DocumentObject *parent, const char *subname);
    void slotExpandObject    (const Gui::ViewProviderDocumentObject&,const Gui::TreeItemMode&,
                              const App::DocumentObject *parent, const char *subname);
    void slotScrollToObject  (const Gui::ViewProviderDocumentObject&);
    void slotRecomputed      (const App::Document &doc, const std::vector<App::DocumentObject*> &objs);
    void slotRecomputedObject(const App::DocumentObject &);

    bool updateObject(const Gui::ViewProviderDocumentObject&, const App::Property &prop);

    bool createNewItem(const Gui::ViewProviderDocumentObject&,
                    QTreeWidgetItem *parent=nullptr, int index=-1,
                    DocumentObjectDataPtr ptrs = DocumentObjectDataPtr());

    int findRootIndex(App::DocumentObject *childObj);

    DocumentObjectItem *findItemByObject(bool sync,
            App::DocumentObject *obj, const char *subname, bool select=false);

    DocumentObjectItem *findItem(bool sync, DocumentObjectItem *item, const char *subname, bool select=true);

    App::DocumentObject *getTopParent(App::DocumentObject *obj, std::string &subname);

    using ViewParentMap = std::unordered_map<const ViewProvider *, std::vector<ViewProviderDocumentObject*> >;
    void populateParents(const ViewProvider *vp, ViewParentMap &);

private:
    const char *treeName; // for debugging purpose
    Gui::Document* pDocument;
    std::unordered_map<App::DocumentObject*,DocumentObjectDataPtr> ObjectMap;
    std::unordered_map<App::DocumentObject*, std::set<App::DocumentObject*> > _ParentMap;
    std::vector<App::DocumentObject*> PopulateObjects;

    ExpandInfoPtr _ExpandInfo;
    void restoreItemExpansion(const ExpandInfoPtr &, DocumentObjectItem *);

    using Connection = boost::signals2::connection;
    Connection connectNewObject;
    Connection connectDelObject;
    Connection connectChgObject;
    Connection connectTouchedObject;
    Connection connectEdtObject;
    Connection connectResObject;
    Connection connectHltObject;
    Connection connectExpObject;
    Connection connectScrObject;
    Connection connectRecomputed;
    Connection connectRecomputedObj;

    friend class TreeWidget;
    friend class DocumentObjectData;
    friend class DocumentObjectItem;
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
    ~DocumentObjectItem() override;

    Gui::ViewProviderDocumentObject* object() const;
    void testStatus(bool resetStatus, QIcon &icon1, QIcon &icon2);
    void testStatus(bool resetStatus);
    void displayStatusInfo();
    void setExpandedStatus(bool);
    void setData(int column, int role, const QVariant & value) override;
    bool isChildOfItem(DocumentObjectItem*);

    void restoreBackground();

    // Get the parent document (where the object is stored) of this item
    DocumentItem *getParentDocument() const;
    // Get the owner document (where the object is displayed, either stored or
    // linked in) of this object
    DocumentItem *getOwnerDocument() const;

    // check if a new item is required at root
    bool requiredAtRoot(bool excludeSelf=true) const;

    // return the owner, and full qualified subname
    App::DocumentObject *getFullSubName(std::ostringstream &str,
            DocumentObjectItem *parent = nullptr) const;

    // return the immediate descendent of the common ancestor of this item and
    // 'cousin'.
    App::DocumentObject *getRelativeParent(
            std::ostringstream &str,
            DocumentObjectItem *cousin,
            App::DocumentObject **topParent=nullptr,
            std::string *topSubname=nullptr) const;

    // return the top most linked group owner's name, and subname.  This method
    // is necessary despite have getFullSubName above is because native geo group
    // cannot handle selection with sub name. So only a linked group can have
    // subname in selection
    int getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const;
    const std::vector<std::string>& getSubNames() const {
        return mySubs;
    }

    void setHighlight(bool set, HighlightMode mode = HighlightMode::LightBlue);

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
    void setCheckState(bool checked);

    QBrush bgBrush;
    DocumentItem *myOwner;
    DocumentObjectDataPtr myData;
    std::vector<std::string> mySubs;
    using Connection = boost::signals2::connection;
    int previousStatus;
    int selected;
    bool populated;

    friend class TreeWidget;
    friend class DocumentItem;
};

class TreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit TreePanel(const char *name, QWidget* parent=nullptr);
    ~TreePanel() override;

    bool eventFilter(QObject *obj, QEvent *ev) override;

private Q_SLOTS:
    void accept();
    void showEditor();
    void hideEditor();
    void itemSearch(const QString &text);

private:
    QLineEdit* searchBox;
    TreeWidget* treeWidget;
};

/**
 * The dock window containing the tree view.
 * @author Werner Mayer
 */
class TreeDockWidget : public Gui::DockWindow
{
    Q_OBJECT

public:
    explicit TreeDockWidget(Gui::Document*  pcDocument,QWidget *parent=nullptr);
    ~TreeDockWidget() override;
};


/**
 * TreeWidget item delegate for editing
 */
class TreeWidgetEditDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TreeWidgetEditDelegate(QObject* parent=nullptr);
    QWidget* createEditor(QWidget *parent,
            const QStyleOptionViewItem &, const QModelIndex &index) const override;
};


}

#endif // GUI_TREE_H
