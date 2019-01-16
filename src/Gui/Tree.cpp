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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <boost/bind.hpp>
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <qcursor.h>
# include <QVBoxLayout>
# include <qlayout.h>
# include <qstatusbar.h>
# include <QContextMenuEvent>
# include <QMenu>
# include <QPixmap>
# include <QTimer>
# include <QToolTip>
# include <QHeaderView>
# include <qmessagebox.h>
#endif

#include <Base/Console.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/GeoFeatureGroupExtension.h>

#include "Tree.h"
#include "Command.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"
#include "MenuManager.h"
#include "Application.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "Macro.h"
#include "Workbench.h"
#include "Widgets.h"

FC_LOG_LEVEL_INIT("Tree",false,true,true);

#define _TREE_PRINT(_level,_func,_msg) \
    _FC_PRINT(FC_LOG_INSTANCE,_level,_func, '['<<getTreeName()<<"] " << _msg)
#define TREE_MSG(_msg) _TREE_PRINT(FC_LOGLEVEL_MSG,NotifyMessage,_msg)
#define TREE_WARN(_msg) _TREE_PRINT(FC_LOGLEVEL_WARN,NotifyWarning,_msg)
#define TREE_ERR(_msg) _TREE_PRINT(FC_LOGLEVEL_ERR,NotifyError,_msg)
#define TREE_LOG(_msg) _TREE_PRINT(FC_LOGLEVEL_LOG,NotifyLog,_msg)
#define TREE_TRACE(_msg) _TREE_PRINT(FC_LOGLEVEL_TRACE,NotifyLog,_msg)

using namespace Gui;

#define TREEVIEW_PARAM "User parameter:BaseApp/Preferences/TreeView"
#define GET_TREEVIEW_PARAM(_name) \
    ParameterGrp::handle _name = App::GetApplication().GetParameterGroupByPath(TREEVIEW_PARAM)

/////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QPixmap>  TreeWidget::documentPixmap;
std::unique_ptr<QPixmap>  TreeWidget::documentPartialPixmap;
std::set<TreeWidget *> TreeWidget::Instances;
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;

TreeParams::TreeParams() {
    GET_TREEVIEW_PARAM(hGrp);
    handle = hGrp;
    handle->Attach(this);

#define FC_TREEPARAM_INIT(_name,_type,_Type,_default) \
    _##_name = handle->Get##_Type(#_name,_default);

#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF FC_TREEPARAM_INIT
    FC_TREEPARAM_DEFS
}

#define FC_TREEPARAM_SET_FUNC(_name,_type,_Type,_default) \
void TreeParams::set##_name(_type value) {\
    if(_##_name != value) {\
        handle->Set##_Type(#_name,value);\
    }\
}

#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF FC_TREEPARAM_SET_FUNC
FC_TREEPARAM_DEFS

void TreeParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
#define FC_TREEPARAM_CHANGE(_name,_type,_Type,_default) \
    if(strcmp(sReason,#_name)==0) {\
        _##_name = handle->Get##_Type(#_name,_default);\
        on##_name##Changed();\
        return;\
    }

#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF FC_TREEPARAM_CHANGE
    FC_TREEPARAM_DEFS
}

void TreeParams::onPreSelectionChanged() {}

void TreeParams::onSyncSelectionChanged() {
    if(!FC_TREEPARAM(SyncSelection) || !Gui::Selection().hasSelection())
        return;
    TreeWidget::scrollItemToTop();
}

void TreeParams::onSyncViewChanged() {}
void TreeParams::onSyncPlacementChanged() {}
void TreeParams::onRecordSelectionChanged() {}

void TreeParams::onDocumentModeChanged() {
    App::GetApplication().setActiveDocument(App::GetApplication().getActiveDocument());
}

TreeParams *TreeParams::Instance() {
    static TreeParams *instance;
    if(!instance)
        instance = new TreeParams;
    return instance;
}

// ---------------------------------------------------------------------------

typedef std::set<DocumentObjectItem*> DocumentObjectItems;

class Gui::DocumentObjectData {
public:
    DocumentItem *docItem;
    DocumentObjectItems items;
    ViewProviderDocumentObject *viewObject;
    DocumentObjectItem *rootItem;
    std::vector<App::DocumentObject*> children;
    std::set<App::DocumentObject*> childSet;
    bool removeChildrenFromRoot;
    std::string label;
    std::string label2;

    typedef boost::signals2::scoped_connection Connection;

    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    DocumentObjectData(DocumentItem *docItem, ViewProviderDocumentObject* vpd)
        : docItem(docItem), viewObject(vpd),rootItem(0)
    {
        // Setup connections
        connectIcon = viewObject->signalChangeIcon.connect(
                boost::bind(&DocumentObjectData::slotChangeIcon, this));
        connectTool = viewObject->signalChangeToolTip.connect(
                boost::bind(&DocumentObjectData::slotChangeToolTip, this, _1));
        connectStat = viewObject->signalChangeStatusTip.connect(
                boost::bind(&DocumentObjectData::slotChangeStatusTip, this, _1));

        removeChildrenFromRoot = viewObject->canRemoveChildrenFromRoot();
        label = viewObject->getObject()->Label.getValue();
        label2 = viewObject->getObject()->Label2.getValue();
    }

    const char *getTreeName() const {
        return docItem->getTreeName();
    }

    void updateChildren(DocumentObjectDataPtr other) {
        children = other->children;
        childSet = other->childSet;
    }

    bool updateChildren(bool checkVisibility) {
        auto newChildren = viewObject->claimChildren();
        auto obj = viewObject->getObject();
        std::set<App::DocumentObject *> newSet;
        bool updated = false;
        for (auto child : newChildren) {
            if(child && child->getNameInDocument()) {
                if(!newSet.insert(child).second) {
                    TREE_WARN("duplicate child item " << obj->getFullName() 
                        << '.' << child->getNameInDocument());
                }else if(!childSet.erase(child)) {
                    // this means new child detected
                    updated = true;
                    if(child->getDocument()==obj->getDocument() && 
                       child->getDocument()==docItem->document()->getDocument())
                        docItem->_ParentMap[child].insert(obj);
                }
            }
        }
        for (auto child : childSet) {
            if(newSet.find(child) == newSet.end()) {
                // this means old child removed
                updated = true;
                docItem->_ParentMap[child].erase(obj);
            }
        }
        // We still need to check the order of the children
        updated = updated || children!=newChildren;
        children.swap(newChildren);
        childSet.swap(newSet);

        if(updated && checkVisibility) {
            for(auto child : children) {
                if(!child || !child->getNameInDocument() || !child->Visibility.getValue())
                    continue;
                if(child->getDocument()==obj->getDocument() && !docItem->isObjectShowable(child))
                    child->Visibility.setValue(false);
            }
        }
        return updated;
    }

    void testStatus(bool resetStatus = false) {
        QIcon icon,icon2;
        for(auto item : items)
            item->testStatus(resetStatus,icon,icon2);
    }

    void slotChangeIcon() {
        testStatus(true);
    }

    void slotChangeToolTip(const QString& tip) {
        for(auto item : items)
            item->setToolTip(0, tip);
    }

    void slotChangeStatusTip(const QString& tip) {
        for(auto item : items)
            item->setStatusTip(0, tip);
    }
};

// ---------------------------------------------------------------------------

TreeWidgetEditDelegate::TreeWidgetEditDelegate(QObject* parent)
    : QStyledItemDelegate(parent),activeTransactionID(0)
{
    connect(this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), 
            this, SLOT(editorClosed(QWidget*)));
}

QWidget* TreeWidgetEditDelegate::createEditor(
        QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const 
{
    auto ti = static_cast<QTreeWidgetItem*>(index.internalPointer());
    if(ti->type()!=TreeWidget::ObjectType || index.column()>1)
        return 0;
    DocumentObjectItem *item = static_cast<DocumentObjectItem*>(ti);
    App::DocumentObject *obj = item->object()->getObject();
    auto &prop = index.column()?obj->Label2:obj->Label;

    std::ostringstream str;
    str << "Change " << obj->getNameInDocument() << '.' << prop.getName();
    activeTransactionID = App::GetApplication().setActiveTransaction(str.str().c_str());
    FC_LOG("create editor transaction " << App::GetApplication().getActiveTransaction());

    ExpLineEdit *le = new ExpLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(prop.isReadOnly());
    le->bind(App::ObjectIdentifier(prop));
    le->setAutoApply(true);
    return le;
}

void TreeWidgetEditDelegate::editorClosed(QWidget *editor) {
    int id = 0;
    const char *name = App::GetApplication().getActiveTransaction(&id);
    if(id && id==activeTransactionID) {
        FC_LOG("editor close transaction " << name);
        App::GetApplication().closeActiveTransaction();
    }else
        FC_LOG("editor closed");
    activeTransactionID = 0;
    editor->close();
}

// ---------------------------------------------------------------------------

TreeWidget::TreeWidget(const char *name, QWidget* parent)
    : QTreeWidget(parent), SelectionObserver(false,0), contextItem(0)
    , searchObject(0), searchDoc(0), searchContextDoc(0)
    , editingItem(0), errItem(0), currentDocItem(0),fromOutside(false)
    ,statusUpdateDelay(0),myName(name)
{
    Instances.insert(this);

    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);
    this->setRootIsDecorated(false);
    this->setColumnCount(2);
    this->setItemDelegate(new TreeWidgetEditDelegate(this));

    this->showHiddenAction = new QAction(this);
    this->showHiddenAction->setCheckable(true);
    connect(this->showHiddenAction, SIGNAL(triggered()),
            this, SLOT(onShowHidden()));

    this->hideInTreeAction = new QAction(this);
    this->hideInTreeAction->setCheckable(true);
    connect(this->hideInTreeAction, SIGNAL(triggered()),
            this, SLOT(onHideInTree()));

    this->createGroupAction = new QAction(this);
    connect(this->createGroupAction, SIGNAL(triggered()),
            this, SLOT(onCreateGroup()));

    this->relabelObjectAction = new QAction(this);
#ifndef Q_OS_MAC
    this->relabelObjectAction->setShortcut(Qt::Key_F2);
#endif
    connect(this->relabelObjectAction, SIGNAL(triggered()),
            this, SLOT(onRelabelObject()));

    this->finishEditingAction = new QAction(this);
    connect(this->finishEditingAction, SIGNAL(triggered()),
            this, SLOT(onFinishEditing()));

    this->reloadDocAction = new QAction(this);
    connect(this->reloadDocAction, SIGNAL(triggered()),
            this, SLOT(onReloadDoc()));

    this->skipRecomputeAction = new QAction(this);
    this->skipRecomputeAction->setCheckable(true);
    connect(this->skipRecomputeAction, SIGNAL(toggled(bool)),
            this, SLOT(onSkipRecompute(bool)));

    this->allowPartialRecomputeAction = new QAction(this);
    this->allowPartialRecomputeAction->setCheckable(true);
    connect(this->allowPartialRecomputeAction, SIGNAL(toggled(bool)),
            this, SLOT(onAllowPartialRecompute(bool)));

    this->markRecomputeAction = new QAction(this);
    connect(this->markRecomputeAction, SIGNAL(triggered()),
            this, SLOT(onMarkRecompute()));

    this->recomputeObjectAction = new QAction(this);
    connect(this->recomputeObjectAction, SIGNAL(triggered()),
            this, SLOT(onRecomputeObject()));
    this->searchObjectsAction = new QAction(this);
    this->searchObjectsAction->setText(tr("Search..."));
    this->searchObjectsAction->setStatusTip(tr("Search for objects"));
    connect(this->searchObjectsAction, SIGNAL(triggered()),
            this, SLOT(onSearchObjects()));

    // Setup connections
    Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, _1));
    Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, _1));
    Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, _1));
    Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, _1));
    Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, _1));
    Application::Instance->signalShowHidden.connect(boost::bind(&TreeWidget::slotShowHidden, this, _1));
    App::GetApplication().signalStartOpenDocument.connect(
            boost::bind(&TreeWidget::slotStartOpenDocument,this));
    App::GetApplication().signalFinishOpenDocument.connect(
            boost::bind(&TreeWidget::slotFinishOpenDocument,this));

    App::GetApplication().signalFinishRestoreDocument.connect
        (boost::bind(&TreeWidget::slotFinishRestoreDocument, this, _1));
    
    // Gui::Document::signalChangedObject informs the App::Document property
    // change, not view provider's own property, which is what the signal below
    // for
    Application::Instance->signalChangedObject.connect(
            boost::bind(&TreeWidget::slotChangedViewObject, this, _1,_2));

    // make sure to show a horizontal scrollbar if needed
#if QT_VERSION >= 0x050000
    this->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
    this->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif
    this->header()->setStretchLastSection(false);

    // Add the first main label
    this->rootItem = new QTreeWidgetItem(this);
    this->rootItem->setFlags(Qt::ItemIsEnabled);
    this->expandItem(this->rootItem);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
#if QT_VERSION >= 0x040200
    // causes unexpected drop events (possibly only with Qt4.1.x)
    this->setMouseTracking(true); // needed for itemEntered() to work
#endif

    this->preselectTimer = new QTimer(this);
    this->preselectTimer->setSingleShot(true);

    this->statusTimer = new QTimer(this);
    this->statusTimer->setSingleShot(false);

    this->selectTimer = new QTimer(this);
    this->selectTimer->setSingleShot(true);

    connect(this->statusTimer, SIGNAL(timeout()),
            this, SLOT(onUpdateStatus()));
    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*, int)),
            this, SLOT(onItemEntered(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));
    connect(this->preselectTimer, SIGNAL(timeout()),
            this, SLOT(onPreSelectTimer()));
    connect(this->selectTimer, SIGNAL(timeout()),
            this, SLOT(onSelectTimer()));
    preselectTime.start();

    setupText();
    if(!documentPixmap) {
        documentPixmap.reset(new QPixmap(Gui::BitmapFactory().pixmap("Document")));
        QIcon icon(*documentPixmap);
        documentPartialPixmap.reset(new QPixmap(icon.pixmap(documentPixmap->size(),QIcon::Disabled)));
    }

    _updateStatus();
}

TreeWidget::~TreeWidget()
{
    Instances.erase(this);
}

const char *TreeWidget::getTreeName() const {
    return myName.c_str();
}

bool TreeWidget::isObjectShowable(App::DocumentObject *obj) {
    if(!obj || !obj->getNameInDocument())
        return true;
    Gui::Document *doc = Application::Instance->getDocument(obj->getDocument());
    if(!doc)
        return true;
    if(Instances.empty())
        return true;
    auto tree = *Instances.begin();
    auto it = tree->DocumentMap.find(doc);
    if(it != tree->DocumentMap.end())
        return it->second->isObjectShowable(obj);
    return true;
}

void TreeWidget::checkTopParent(App::DocumentObject *&obj, std::string &subname) {
    if(Instances.size() && obj && obj->getNameInDocument()) {
        auto tree = *Instances.begin();
        auto it = tree->DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
        if(it != tree->DocumentMap.end()) {
            auto parent = it->second->getTopParent(obj,subname);
            if(parent)
                obj = parent;
        }
    }
}

void TreeWidget::resetItemSearch() {
    if(!searchObject)
        return;
    auto it = ObjectTable.find(searchObject);
    if(it != ObjectTable.end()) {
        for(auto &data : it->second) {
            if(!data)
                continue;
            for(auto item : data->items)
                static_cast<DocumentObjectItem*>(item)->restoreBackground();
        }
    }
    searchObject = 0;
}

void TreeWidget::startItemSearch() {
    resetItemSearch();
    searchDoc = 0;
    searchContextDoc = 0;
    if (contextItem) {
        if(contextItem->type() == DocumentType) {
            searchDoc = static_cast<DocumentItem*>(contextItem)->document();
        } else if(contextItem->type() == ObjectType) {
            searchDoc = static_cast<DocumentObjectItem*>(contextItem)->object()->getDocument();
        }
    }else{
        auto sels = selectedItems();
        if(sels.size() == 1) {
            auto item = static_cast<DocumentObjectItem*>(sels.front());
            searchDoc = item->object()->getDocument();
            searchContextDoc = item->getOwnerDocument()->document();
        }
    }
}

void TreeWidget::itemSearch(const QString &text, bool select) {
    resetItemSearch();

    auto docItem = getDocumentItem(searchDoc);
    if(!docItem) {
        docItem = getDocumentItem(Application::Instance->activeDocument());
        if(!docItem) {
            FC_TRACE("item search no document");
            resetItemSearch();
            return;
        }
    }

    auto doc = docItem->document()->getDocument();
    const auto &objs = doc->getObjects();
    if(objs.empty()) {
        FC_TRACE("item search no objects");
        return;
    }
    std::string txt(text.toUtf8().constData());
    try {
        if(txt.empty())
            return;
        if(txt.find("<<") == std::string::npos) {
            auto pos = txt.find('.');
            if(pos==std::string::npos)
                txt += '.';
            else if(pos!=txt.size()-1) {
                txt.insert(pos+1,"<<");
                if(txt.back()!='.')
                    txt += '.';
                txt += ">>.";
            }
        }else if(txt.back() != '.')
            txt += '.';
        txt += "_self";
        auto path = App::ObjectIdentifier::parse(objs.front(),txt);
        if(path.getPropertyName() != "_self") {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        auto obj = path.getDocumentObject();
        if(!obj) {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        std::string subname = path.getSubObjectName();
        App::DocumentObject *parent = 0;
        if(searchContextDoc) {
            auto it = DocumentMap.find(searchContextDoc);
            if(it!=DocumentMap.end()) {
                parent = it->second->getTopParent(obj,subname);
                if(parent) {
                    obj = parent;
                    docItem = it->second;
                    doc = docItem->document()->getDocument();
                }
            }
        }
        if(!parent) {
            parent = docItem->getTopParent(obj,subname);
            while(!parent) {
                if(docItem->document()->getDocument() == obj->getDocument()) {
                    // this shouldn't happen
                    FC_LOG("Object " << txt << " not found in " << doc->getName());
                    return;
                }
                auto it = DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
                if(it==DocumentMap.end())
                    return;
                docItem = it->second;
                parent = docItem->getTopParent(obj,subname);
            }
            obj = parent;
        }
        auto item = docItem->findItemByObject(true,obj,subname.c_str());
        if(!item) {
            FC_TRACE("item " << txt << " not found in " << doc->getName());
            return;
        }
        scrollToItem(item);
        Selection().setPreselect(obj->getDocument()->getName(),
                obj->getNameInDocument(), subname.c_str(),0,0,0,2);
        if(select) {
            Gui::Selection().selStackPush();
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(),subname.c_str());
            Gui::Selection().selStackPush();
        }else{
            searchObject = item->object()->getObject();
            item->setBackground(0, QColor(255, 255, 0, 100));
        }
        FC_TRACE("found item " << txt);
    } catch(...)
    {
        FC_TRACE("item " << txt << " search exception in " << doc->getName());
    }
}

void TreeWidget::updateStatus(bool delay) {
    for(auto tree : Instances)
        tree->_updateStatus(delay);
}

void TreeWidget::_updateStatus(bool delay) {
    if(!statusTimer->isActive())
        statusTimer->start(150);
    else if(delay) {
        if(!statusUpdateDelay)
            statusUpdateDelay=1;
    }else
        statusUpdateDelay=-1;
}

void TreeWidget::contextMenuEvent (QContextMenuEvent * e)
{
    // ask workbenches and view provider, ...
    MenuItem view;
    view << "Std_TreeViewActions";

    Gui::Application::Instance->setupContextMenu("Tree", &view);

    Workbench::createLinkMenu(&view);
    view << "Std_Expressions";

    QMenu contextMenu;

    QMenu subMenu;
    QMenu editMenu;
    QActionGroup subMenuGroup(&subMenu);
    subMenuGroup.setExclusive(true);
    connect(&subMenuGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(onActivateDocument(QAction*)));
    MenuManager::getInstance()->setupContextMenu(&view, contextMenu);

    // get the current item
    this->contextItem = itemAt(e->pos());

    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        showHiddenAction->setChecked(docitem->showHidden());
        contextMenu.addAction(this->showHiddenAction);
        contextMenu.addAction(this->searchObjectsAction);
        if(doc->testStatus(App::Document::PartialDoc))
            contextMenu.addAction(this->reloadDocAction);
        else {
            for(auto d : doc->getDependentDocuments()) {
                if(d->testStatus(App::Document::PartialDoc)) {
                    contextMenu.addAction(this->reloadDocAction);
                    break;
                }
            }
            this->skipRecomputeAction->setChecked(doc->testStatus(App::Document::SkipRecompute));
            contextMenu.addAction(this->skipRecomputeAction);
            this->allowPartialRecomputeAction->setChecked(doc->testStatus(App::Document::AllowPartialRecompute));
            if(doc->testStatus(App::Document::SkipRecompute))
                contextMenu.addAction(this->allowPartialRecomputeAction);
            contextMenu.addAction(this->markRecomputeAction);
            contextMenu.addAction(this->createGroupAction);
        }
        contextMenu.addSeparator();
    }
    else if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);

        auto selItems = this->selectedItems();
        // if only one item is selected setup the edit menu
        if (selItems.size() == 1) {
            objitem->object()->setupContextMenu(&editMenu, this, SLOT(onStartEditing()));
            QList<QAction*> editAct = editMenu.actions();
            if (!editAct.isEmpty()) {
                for (QList<QAction*>::iterator it = editAct.begin(); it != editAct.end(); ++it)
                    contextMenu.addAction(*it);
                QAction* first = editAct.front();
                contextMenu.setDefaultAction(first);
                if (objitem->object()->isEditing())
                    contextMenu.addAction(finishEditingAction);
                contextMenu.addSeparator();
            }
        }

        App::Document* doc = objitem->object()->getObject()->getDocument();
        showHiddenAction->setChecked(doc->ShowHidden.getValue());
        contextMenu.addAction(this->showHiddenAction);

        hideInTreeAction->setChecked(!objitem->object()->showInTree());
        contextMenu.addAction(this->hideInTreeAction);

        contextMenu.addAction(this->searchObjectsAction);

        if (objitem->object()->getObject()->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId()))
            contextMenu.addAction(this->createGroupAction);

        contextMenu.addAction(this->markRecomputeAction);
        contextMenu.addAction(this->recomputeObjectAction);
        contextMenu.addAction(this->relabelObjectAction);
    }


    // add a submenu to active a document if two or more exist
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    if (docs.size() >= 2) {
        contextMenu.addSeparator();
        App::Document* activeDoc = App::GetApplication().getActiveDocument();
        subMenu.setTitle(tr("Activate document"));
        contextMenu.addMenu(&subMenu);
        QAction* active = 0;
        for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QAction* action = subMenuGroup.addAction(label);
            action->setCheckable(true);
            action->setStatusTip(tr("Activate document %1").arg(label));
            action->setData(QByteArray((*it)->getName()));
            if (*it == activeDoc) active = action;
        }

        if (active)
            active->setChecked(true);
        subMenu.addActions(subMenuGroup.actions());
    }

    if (contextMenu.actions().count() > 0) {
        contextMenu.exec(QCursor::pos());
        contextItem = 0;
    }
}

void TreeWidget::hideEvent(QHideEvent *ev) {
    TREE_TRACE("detaching selection observer");
    this->detachSelection();
    selectTimer->stop();
    QTreeWidget::hideEvent(ev);
}

void TreeWidget::showEvent(QShowEvent *ev) {
    TREE_TRACE("attaching selection observer");
    this->attachSelection();
    selectTimer->start(100);
    _updateStatus(false);
    QTreeWidget::showEvent(ev);
}

void TreeWidget::onCreateGroup()
{
    QString name = tr("Group");
    if (this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        QString cmd = QString::fromLatin1("App.getDocument(\"%1\").addObject"
                              "(\"App::DocumentObjectGroup\",\"%2\")")
                              .arg(QString::fromLatin1(doc->getName())).arg(name);
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Create group");
        Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
        gui->commitCommand();
    }
    else if (this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        App::DocumentObject* obj = objitem->object()->getObject();
        App::Document* doc = obj->getDocument();
        QString cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\")"
                              ".newObject(\"App::DocumentObjectGroup\",\"%3\")")
                              .arg(QString::fromLatin1(doc->getName()))
                              .arg(QString::fromLatin1(obj->getNameInDocument()))
                              .arg(name);
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Create group");
        Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
        gui->commitCommand();
    }
}

void TreeWidget::onRelabelObject()
{
    QTreeWidgetItem* item = currentItem();
    if (item)
        editItem(item);
}

void TreeWidget::onStartEditing()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        if (this->contextItem && this->contextItem->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
                (this->contextItem);
            int edit = action->data().toInt();

            App::DocumentObject* obj = objitem->object()->getObject();
            if (!obj || !obj->getNameInDocument()) 
                return;
            auto doc = const_cast<Document*>(objitem->getOwnerDocument()->document());
            MDIView *view = doc->getActiveView();
            if (view) getMainWindow()->setActiveWindow(view);

            // Always open a transaction here doesn't make much sense because:
            // - many objects open transactions when really changing some properties
            // - this leads to certain inconsistencies with the doubleClicked() method
            // So, only the view provider class should decide what to do
#if 0
            // open a transaction before starting edit mode
            std::string cmd("Edit ");
            cmd += obj->Label.getValue();
            doc->openCommand(cmd.c_str());
            bool ok = doc->setEdit(objitem->object(), edit);
            if (!ok) doc->abortCommand();
#else
            editingItem = objitem;
            if(!doc->setEdit(objitem->object(), edit))
                editingItem = 0;
#endif
        }
    }
}

void TreeWidget::onFinishEditing()
{
    if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        App::DocumentObject* obj = objitem->object()->getObject();
        if (!obj) return;
        Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
        doc->commitCommand();
        doc->resetEdit();
        doc->getDocument()->recompute();
    }
}

void TreeWidget::onSkipRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::SkipRecompute, on);
    }
}

void TreeWidget::onAllowPartialRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::AllowPartialRecompute, on);
    }
}

void TreeWidget::onMarkRecompute()
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        std::vector<App::DocumentObject*> obj = doc->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
            (*it)->enforceRecompute();
    }
    // mark all selected objects
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                App::DocumentObject* obj = objitem->object()->getObject();
                obj->enforceRecompute();
            }
        }
    }
}

void TreeWidget::onRecomputeObject() {
    std::vector<App::DocumentObject*> objs;
    for(auto ti : selectedItems()) {
        if (ti->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(ti);
            objs.push_back(objitem->object()->getObject());
            objs.back()->enforceRecompute();
        }
    }
    if(objs.empty())
        return;
    App::GetApplication().setActiveTransaction("Recompute object");
    std::string msg;
    try {
        objs.front()->getDocument()->recompute(objs,true);
    }catch (Base::Exception &e) {
        e.ReportException();
        msg = e.what();
    }catch (std::exception &e) {
        msg = e.what();
    }
    App::GetApplication().closeActiveTransaction();
    if(msg.size()) {
        QMessageBox::critical(getMainWindow(), QObject::tr("Recompute failed"),
                QString::fromUtf8(msg.c_str()));
    }
}


DocumentItem *TreeWidget::getDocumentItem(const Gui::Document *doc) const {
    auto it = DocumentMap.find(doc);
    if(it != DocumentMap.end())
        return it->second;
    return 0;
}

void TreeWidget::selectAllInstances(const ViewProviderDocumentObject &vpd) {
    if(!isConnectionAttached()) 
        return;

    for(const auto &v : DocumentMap) 
        v.second->selectAllInstances(vpd);
}

static TreeWidget *_LastSelectedTreeWidget;

std::vector<std::pair<ViewProviderDocumentObject*,ViewProviderDocumentObject*> > 
TreeWidget::getSelection(App::Document *doc)
{
    std::vector<std::pair<ViewProviderDocumentObject*,ViewProviderDocumentObject*> > ret;

    TreeWidget *tree = _LastSelectedTreeWidget;
    if(!tree || !tree->isConnectionAttached()) {
        for(auto pTree : Instances)
            if(pTree->isConnectionAttached()) {
                tree = pTree;
                break;
            }
    }
    if(!tree) return ret;

    for(auto ti : tree->selectedItems()) {
        if(ti->type() != ObjectType) continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        auto vp = item->object();
        auto obj = vp->getObject();
        if(!obj || !obj->getNameInDocument()) {
            FC_WARN("skip invalid object");
            continue;
        }
        if(doc && obj->getDocument()!=doc) {
            FC_LOG("skip objects not from current document");
            continue;
        }
        ViewProviderDocumentObject *parentVp = 0;
        auto parent = item->getParentItem();
        if(parent) {
            parentVp = parent->object();
            if(!parentVp->getObject()->getNameInDocument()) {
                FC_WARN("skip '" << obj->getFullName() << "' with invalid parent");
                continue;
            }
        }
        ret.push_back(std::make_pair(parentVp,vp));
    }
    return ret;
}

void TreeWidget::selectAllLinks(App::DocumentObject *obj) {
    if(!isConnectionAttached()) 
        return;

    if(!obj || !obj->getNameInDocument()) {
        TREE_ERR("invlaid object");
        return;
    }
    for(auto link: App::GetApplication().getLinksTo(obj,true)) {
        if(!link || !link->getNameInDocument()) {
            TREE_ERR("invalid linked object");
            continue;
        }
        auto vp = dynamic_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(link));
        if(!vp) {
            TREE_ERR("invalid view provider of the linked object");
            continue;
        }
        for(auto &v : DocumentMap)
            v.second->selectAllInstances(*vp);
    }
}

void TreeWidget::onSearchObjects()
{
    emitSearchObjects();
}

void TreeWidget::onActivateDocument(QAction* active)
{
    // activate the specified document
    QByteArray docname = active->data().toByteArray();
    Gui::Document* doc = Application::Instance->getDocument((const char*)docname);
    if (doc)
        doc->setActiveView();
}

Qt::DropActions TreeWidget::supportedDropActions () const
{
    return Qt::LinkAction | Qt::CopyAction | Qt::MoveAction;
}

bool TreeWidget::event(QEvent *e)
{
#if 0
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
            case Qt::Key_Delete:
                ke->accept();
        }
    }
#endif
    return QTreeWidget::event(e);
}

void TreeWidget::keyPressEvent(QKeyEvent *event)
{
#if 0
    if (event && event->matches(QKeySequence::Delete)) {
        event->ignore();
    }
#endif
    if(event && event->matches(QKeySequence::Find)) {
        event->accept();
        onSearchObjects();
        return;
    }
    QTreeWidget::keyPressEvent(event);
}

void TreeWidget::mouseDoubleClickEvent (QMouseEvent * event)
{
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item) return;
    if (item->type() == TreeWidget::DocumentType) {
        //QTreeWidget::mouseDoubleClickEvent(event);
        const Gui::Document* doc = static_cast<DocumentItem*>(item)->document();
        if (!doc) return;
        if(doc->getDocument()->testStatus(App::Document::PartialDoc)) {
            contextItem = item;
            onReloadDoc();
            return;
        }
        doc->setActiveView();
    }
    else if (item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(item);
        objitem->getOwnerDocument()->document()->setActiveView(objitem->object());
        auto manager = Application::Instance->macroManager();
        auto lines = manager->getLines();
        std::ostringstream ss;
        ss << Command::getObjectCmd(objitem->object()->getObject())
            << ".ViewObject.doubleClicked()";
        if (!objitem->object()->doubleClicked())
            QTreeWidget::mouseDoubleClickEvent(event);
        else if(lines == manager->getLines())
            manager->addLine(MacroManager::Gui,ss.str().c_str());
    }
}

void TreeWidget::startDragging() {
    if(state() != NoState)
        return;
    if(selectedItems().empty())
        return;

    setState(DraggingState);
    startDrag(model()->supportedDragActions());
    setState(NoState);
    stopAutoScroll();
}

void TreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QTreeWidget::startDrag(supportedActions);
}

QMimeData * TreeWidget::mimeData (const QList<QTreeWidgetItem *> items) const
{
    // all selected items must reference an object from the same document
    App::Document* doc=0;
    for (QList<QTreeWidgetItem *>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if ((*it)->type() != TreeWidget::ObjectType)
            return 0;
        App::DocumentObject* obj = static_cast<DocumentObjectItem *>(*it)->object()->getObject();
        if (!doc)
            doc = obj->getDocument();
        else if (doc != obj->getDocument())
            return 0;
    }
    return QTreeWidget::mimeData(items);
}

bool TreeWidget::dropMimeData(QTreeWidgetItem *parent, int index,
                              const QMimeData *data, Qt::DropAction action)
{
    return QTreeWidget::dropMimeData(parent, index, data, action);
}

void TreeWidget::dragEnterEvent(QDragEnterEvent * event)
{
    QTreeWidget::dragEnterEvent(event);
}

void TreeWidget::dragLeaveEvent(QDragLeaveEvent * event)
{
    QTreeWidget::dragLeaveEvent(event);
}

void TreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    QTreeWidgetItem* targetItem = itemAt(event->pos());
    if (!targetItem || this->isItemSelected(targetItem)) {
        leaveEvent(0);
        event->ignore();
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        leaveEvent(0);
    }
    else if (targetItem->type() == TreeWidget::ObjectType) {
        onItemEntered(targetItem);

        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();

        if (!vp->canDropObjects()) {
            TREE_TRACE("cannot drop");
            event->ignore();
            return;
        }

        for(auto ti : selectedItems()) {
            if (ti->type() != TreeWidget::ObjectType) {
                TREE_TRACE("cannot drop");
                event->ignore();
                return;
            }
            auto item = static_cast<DocumentObjectItem*>(ti);

            // To avoid a cylic dependency it must be made sure to not allow to
            // drag'n'drop a tree item onto a child or grandchild item of it.
            if (targetItemObj->isChildOfItem(item)) {
                TREE_TRACE("cannot drop");
                event->ignore();
                return;
            }

            // if the item is already a child of the target item there is nothing to do
            if (item->parent() == targetItem) {
                TREE_TRACE("cannot drop");
                event->ignore();
                return;
            }

            std::ostringstream str;
            auto owner = item->getRelativeParent(str,targetItemObj);
            auto subname = str.str();

            auto obj = item->object()->getObject();
            // let the view provider decide to accept the object or ignore it
            if (!vp->canDropObjectEx(obj,owner,subname.c_str(), item->mySubs)) {
                TREE_TRACE("cannot drop " << obj->getFullName() << ' '
                        << (owner?owner->getFullName():"<No Owner>") << '.' << subname);
                event->ignore();
                return;
            }
        }
    }
    else {
        leaveEvent(0);
        event->ignore();
    }
}

struct ItemInfo {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string ownerDoc;
    std::string owner;
    std::string subname;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
    std::vector<std::string> subs;
    bool dragging = false;
};
struct ItemInfo2 {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
};

void TreeWidget::dropEvent(QDropEvent *event)
{
    //FIXME: This should actually be done inside dropMimeData

    QTreeWidgetItem* targetItem = itemAt(event->pos());
    // not dropped onto an item
    if (!targetItem)
        return;
    // one of the source items is also the destination item, that's not allowed
    if (this->isItemSelected(targetItem))
        return;

    // filter out the selected items we cannot handle
    std::vector<std::pair<DocumentObjectItem*,std::vector<std::string> > > items;
    auto sels = selectedItems();
    items.reserve(sels.size());
    for(auto ti : sels) {
        if (ti->type() != TreeWidget::ObjectType)
            continue;
        // ignore child elements if the parent is selected
        if(sels.contains(ti->parent())) 
            continue;
        if (ti == targetItem)
            continue;
        if (ti->parent() == targetItem)
            continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        items.emplace_back();
        auto &info = items.back();
        info.first = item;
        info.second.insert(info.second.end(),item->mySubs.begin(),item->mySubs.end());
    }

    if (items.empty())
        return; // nothing needs to be done

    bool dropOnly = QApplication::keyboardModifiers()== Qt::ControlModifier;

    if (targetItem->type() == TreeWidget::ObjectType) {
        // add object to group
        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();
        if (!vp->canDropObjects()) {
            return; // no group like object
        }

        std::ostringstream targetSubname;
        App::DocumentObject *targetParent = 0;
        targetItemObj->getSubName(targetSubname,targetParent);
        Selection().selStackPush();
        Selection().clearCompleteSelection();
        if(targetParent) {
            targetSubname << vp->getObject()->getNameInDocument() << '.';
            Selection().addSelection(targetParent->getDocument()->getName(),
                    targetParent->getNameInDocument(), targetSubname.str().c_str());
        } else {
            targetParent = targetItemObj->object()->getObject();
            Selection().addSelection(targetParent->getDocument()->getName(),
                    targetParent->getNameInDocument());
        }

        bool syncPlacement = FC_TREEPARAM(SyncPlacement) && targetItemObj->isGroup();

        std::vector<ItemInfo> infos;
        // Only keep text names here, because you never know when doing drag
        // and drop some object may delete other objects.
        infos.reserve(items.size());
        for(auto &v : items) {
            infos.emplace_back();
            auto &info = infos.back();
            auto item = v.first;
            Gui::ViewProviderDocumentObject* vpc = item->object();
            App::DocumentObject* obj = vpc->getObject();
            std::ostringstream str;
            App::DocumentObject *topParent=0;
            auto owner = item->getRelativeParent(str,targetItemObj,&topParent,&info.topSubname);
            if(syncPlacement && topParent) {
                info.topDoc = topParent->getDocument()->getName();
                info.topObj = topParent->getNameInDocument();
            }
            info.subname = str.str();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if(owner) {
                info.ownerDoc = owner->getDocument()->getName();
                info.owner = owner->getNameInDocument();
            }

            info.subs.swap(v.second);

            // check if items can be dragged
            if(!dropOnly && 
               item->myOwner == targetItemObj->myOwner && 
               vp->canDragAndDropObject(item->object()->getObject()))
            {
                auto parentItem = item->getParentItem();
                if(parentItem &&
                   (!parentItem->object()->canDragObjects() || 
                    !parentItem->object()->canDragObject(item->object()->getObject())))
                {
                    TREE_ERR("'" << item->object()->getObject()->getFullName() << 
                           "' cannot be dragged out of '" << 
                           parentItem->object()->getObject()->getFullName() << "'");
                    return;
                }
                info.dragging = true;
                if(parentItem) {
                    auto vpp = parentItem->object();
                    info.parent = vpp->getObject()->getNameInDocument();
                    info.parentDoc = vpp->getObject()->getDocument()->getName();
                }
            }
        }

        // Open command
        Gui::Document* gui = vp->getDocument();
        gui->openCommand("Drop object");
        try {
            auto targetObj = targetItemObj->object()->getObject();
            std::string target = targetObj->getNameInDocument();
            auto targetDoc = targetObj->getDocument();
            for (auto &info : infos) {
                auto &subname = info.subname;
                targetObj = targetDoc->getObject(target.c_str());
                vp = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(targetObj));
                if(!vp) {
                    FC_ERR("Cannot find drop traget object " << target);
                    break;
                }

                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if(!doc) {
                    FC_WARN("Cannot find document " << info.doc);
                    continue;
                }
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(obj));
                if(!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                ViewProviderDocumentObject *vpp = 0;
                if(info.parentDoc.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if(parentDoc) {
                        auto parent = parentDoc->getObject(info.parent.c_str());
                        vpp = dynamic_cast<ViewProviderDocumentObject*>(
                                Application::Instance->getViewProvider(parent));
                    }
                    if(!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }
                }

                App::DocumentObject *owner = 0;
                if(info.ownerDoc.size()) {
                    auto ownerDoc = App::GetApplication().getDocument(info.ownerDoc.c_str());
                    if(ownerDoc) 
                        owner = ownerDoc->getObject(info.owner.c_str());
                    if(!owner) {
                        FC_WARN("Cannot find dragging object's top parent " << info.owner);
                        continue;
                    }
                }

                Base::Matrix4D mat;
                App::PropertyPlacement *propPlacement = 0;
                if(syncPlacement) {
                    if(info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if(doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if(topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(),0,&mat);
                                if(sobj == obj) {
                                    propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                            obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }else{
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                obj->getPropertyByName("Placement"));
                        if(propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                auto manager = Application::Instance->macroManager();
                std::ostringstream ss;
                if(vpp) {
                    auto lines = manager->getLines();
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    vpp->dragObject(obj);
                    if(manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui,ss.str().c_str());
                    owner = 0;
                    subname.clear();
                    ss.str("");

                    obj = doc->getObject(info.obj.c_str());
                    if(!obj || !obj->getNameInDocument()) {
                        FC_WARN("Dropping object deleted: " << info.doc << '@' << info.obj);
                        continue;
                    }
                }

                if(info.dragging) {
                    // Construct the subname pointing to the future dropped location
                    auto parentObj = targetObj;
                    std::string dropName = vp->getDropPrefix();
                    if(dropName.size()) 
                        parentObj = targetObj->getSubObject(dropName.c_str());

                    if(parentObj) {
                        // Try to adjust relative links to avoid cyclic dependency, may
                        // throw exception if failed
                        FCMD_OBJ_CMD(obj,"adjustRelativeLinks(" << Command::getObjectCmd(parentObj) << ")");
                    }
                }

                ss.str("");
                ss << Command::getObjectCmd(vp->getObject())
                    << ".ViewObject.dropObject(" << Command::getObjectCmd(obj);
                if(owner) {
                    ss << "," << Command::getObjectCmd(owner)
                        << ",'" << subname << "',[";
                }else
                    ss << ",None,'',[";
                for(auto &sub : info.subs)
                    ss << "'" << sub << "',";
                ss << "])";
                auto lines = manager->getLines();
                std::string dropName = vp->dropObjectEx(obj,owner,subname.c_str(),info.subs);
                if(manager->getLines() == lines)
                    manager->addLine(MacroManager::Gui,ss.str().c_str());

                // Construct the subname pointing to the dropped object
                auto pos = targetSubname.tellp();
                if(dropName.size())
                    targetSubname << dropName << std::ends;
                else 
                    targetSubname << obj->getNameInDocument() << '.';
                dropName = targetSubname.str();
                targetSubname.seekp(pos);

                Base::Matrix4D newMat;
                auto sobj = targetParent->getSubObject(dropName.c_str(),0,&newMat);
                if(!sobj) {
                    FC_LOG("failed to find dropped object " 
                            << targetParent->getFullName() << '.' << dropName);
                    continue;
                }

                if(propPlacement) {
                    // try to adjust placement
                    if((info.dragging && sobj==obj) || 
                       (!info.dragging && sobj->getLinkedObject(false)==obj)) 
                    {
                        if(!info.dragging)
                            propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                    sobj->getPropertyByName("Placement"));
                        if(propPlacement) {
                            newMat *= propPlacement->getValue().inverse().toMatrix();
                            newMat.inverse();
                            Base::Placement pla(newMat*mat);
                            propPlacement->setValue(pla);
                        }
                    }
                }
            }
        } catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                    QString::fromLatin1(e.what()));
            gui->abortCommand();
            e.ReportException();
            return;
        }
        gui->commitCommand();
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        std::vector<ItemInfo2> infos;
        infos.reserve(items.size());
        bool syncPlacement = FC_TREEPARAM(SyncPlacement);
        auto targetDocItem = static_cast<DocumentItem*>(targetItem);
        App::Document* thisDoc = targetDocItem->document()->getDocument();

        // check if items can be dragged
        for(auto &v : items) {
            auto item = v.first;
            auto obj = item->object()->getObject();
            auto parentItem = item->getParentItem();
            if(!parentItem) {
                if(obj->getDocument() == thisDoc)
                    continue;
            }else if(dropOnly || item->myOwner!=targetItem) {
                // We will not drag item out of parent if either, 1) the CTRL
                // key is held, or 2) the dragging item is not inside the
                // dropping document tree.
                parentItem = 0;
            }else if(!parentItem->object()->canDragObjects() || !parentItem->object()->canDragObject(obj)) {
                TREE_ERR("'" << obj->getFullName() << "' cannot be dragged out of '" << 
                    parentItem->object()->getObject()->getFullName() << "'");
                return;
            }
            infos.emplace_back();
            auto &info = infos.back();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if(parentItem) {
                auto parent = parentItem->object()->getObject();
                info.parentDoc = parent->getDocument()->getName();
                info.parent = parent->getNameInDocument();
            }
            if(syncPlacement) {
                std::ostringstream ss;
                App::DocumentObject *topParent=0;
                item->getSubName(ss,topParent);
                if(topParent) {
                    info.topDoc = topParent->getDocument()->getName();
                    info.topObj = topParent->getNameInDocument();
                    ss << obj->getNameInDocument() << '.';
                    info.topSubname = ss.str();
                }
            }
        }
        // Because the existence of subname, we must de-select the drag the
        // object manually. Just do a complete clear here for simplicity
        Selection().selStackPush();
        Selection().clearCompleteSelection();

        // Open command
        Gui::Document* gui = Gui::Application::Instance->getDocument(thisDoc);
        gui->openCommand("Move object");
        try {
            std::vector<App::DocumentObject*> droppedObjs;
            for (auto &info : infos) {
                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if(!doc) continue;
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(obj));
                if(!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                Base::Matrix4D mat;
                App::PropertyPlacement *propPlacement = 0;
                if(syncPlacement) {
                    if(info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if(doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if(topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(),0,&mat);
                                if(sobj == obj) {
                                    propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                            obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }else{
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                obj->getPropertyByName("Placement"));
                        if(propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                if(info.parent.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if(!parentDoc) {
                        FC_WARN("Canont find document " << info.parentDoc);
                        continue;
                    }
                    auto parent = parentDoc->getObject(info.parent.c_str());
                    auto vpp = dynamic_cast<ViewProviderDocumentObject*>(
                            Application::Instance->getViewProvider(parent));
                    if(!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }

                    std::ostringstream ss;
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    auto manager = Application::Instance->macroManager();
                    auto lines = manager->getLines();
                    vpp->dragObject(obj);
                    if(manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui,ss.str().c_str());

                    //make sure it is not part of a geofeaturegroup anymore.
                    //When this has happen we need to handle all removed
                    //objects
                    auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
                    if(grp) {
                        FCMD_OBJ_CMD(grp,"removeObject(" << Command::getObjectCmd(obj) << ")");
                    }

                    // check if the object has been deleted
                    obj = doc->getObject(info.obj.c_str());
                    if(!obj || !obj->getNameInDocument())
                        continue;
                    droppedObjs.push_back(obj);
                    if(propPlacement) 
                        propPlacement->setValue(Base::Placement(mat));
                }else{
                    std::string name = thisDoc->getUniqueObjectName("Link");
                    FCMD_DOC_CMD(thisDoc,"addObject('App::Link','" << name << "').setLink("
                            << Command::getObjectCmd(obj)  << ")");
                    auto link = thisDoc->getObject(name.c_str());
                    if(!link)
                        continue;
                    FCMD_OBJ_CMD(link,"Label='" << obj->getLinkedObject(true)->Label.getValue() << "'");
                    propPlacement = dynamic_cast<App::PropertyPlacement*>(link->getPropertyByName("Placement"));
                    if(propPlacement) 
                        propPlacement->setValue(Base::Placement(mat));
                    droppedObjs.push_back(link);
                }
            }
            DocumentObjectItem *scrollItem = 0;
            for(auto obj : droppedObjs) {
                auto it = targetDocItem->ObjectMap.find(obj);
                if(it==targetDocItem->ObjectMap.end())
                    continue;
                DocumentObjectItem *item = it->second->rootItem;
                if(!item && it->second->items.size())
                    item = *it->second->items.begin();
                if(item) {
                    if(!scrollItem)
                        scrollItem = item;
                    targetDocItem->showItem(item,true);
                    continue;
                }
                auto itDoc = DocumentMap.find(
                        Application::Instance->getDocument(obj->getDocument()));
                if(itDoc==DocumentMap.end())
                    continue;
                it = itDoc->second->ObjectMap.find(obj);
                if(it == itDoc->second->ObjectMap.end())
                    continue;
                item = it->second->rootItem;
                if(!item && it->second->items.size())
                    item = *it->second->items.begin();
                if(item) {
                    if(!scrollItem)
                        scrollItem = item;
                    itDoc->second->showItem(item,true);
                }
            }
            if(scrollItem)
                scrollToItem(scrollItem);

        } catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                    QString::fromLatin1(e.what()));
            gui->abortCommand();
            e.ReportException();
            return;
        }
        gui->commitCommand();
    }
}

void TreeWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &options, const QModelIndex &index) const
{
    QTreeWidget::drawRow(painter, options, index);
    // Set the text and highlighted text color of a hidden object to a dark
    //QTreeWidgetItem * item = itemFromIndex(index);
    //if (item->type() == ObjectType && !(static_cast<DocumentObjectItem*>(item)->previousStatus & 1)) {
    //    QStyleOptionViewItem opt(options);
    //    opt.state ^= QStyle::State_Enabled;
    //    QColor c = opt.palette.color(QPalette::Inactive, QPalette::Dark);
    //    opt.palette.setColor(QPalette::Inactive, QPalette::Text, c);
    //    opt.palette.setColor(QPalette::Inactive, QPalette::HighlightedText, c);
    //    QTreeWidget::drawRow(painter, opt, index);
    //}
    //else {
    //    QTreeWidget::drawRow(painter, options, index);
    //}
}

void TreeWidget::slotNewDocument(const Gui::Document& Doc)
{
    DocumentItem* item = new DocumentItem(&Doc, this->rootItem);
    this->expandItem(item);
    item->setIcon(0, *documentPixmap);
    item->setText(0, QString::fromUtf8(Doc.getDocument()->Label.getValue()));
    DocumentMap[ &Doc ] = item;
}

void TreeWidget::slotStartOpenDocument() {
    errItem = 0;
    setVisible(false);
}

void TreeWidget::slotFinishOpenDocument() {
    if(errItem) {
        scrollToItem(errItem);
        errItem = 0;
    }
    setVisible(true);
}

void TreeWidget::onReloadDoc() {
    if (!this->contextItem || this->contextItem->type() != DocumentType)
        return;
    DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
    App::Document* doc = docitem->document()->getDocument();
    std::string name = doc->FileName.getValue();
    Application::Instance->reopen(doc);
    for(auto &v : DocumentMap) {
        if(name == v.first->getDocument()->FileName.getValue()) {
            scrollToItem(v.second);
            break;
        }
    }
}

void TreeWidget::slotRenameDocument(const Gui::Document& Doc)
{
    // do nothing here
    Q_UNUSED(Doc);
}

void TreeWidget::slotChangedViewObject(const Gui::ViewProvider& vp, const App::Property &prop)
{
    if(App::GetApplication().isRestoring())
        return;

    _updateStatus(true);

    if(!vp.isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) 
        return;
    const auto &vpd = static_cast<const ViewProviderDocumentObject&>(vp);
    if(&prop == &vpd.ShowInTree) {
        for(auto &v : DocumentMap) 
            v.second->setItemVisibility(vpd);
    }else{
        for(auto &v : DocumentMap) 
            v.second->checkRemoveChildrenFromRoot(vpd);
    }
}

void TreeWidget::slotShowHidden(const Gui::Document& Doc)
{
    std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end())
        it->second->updateItemsVisibility(it->second,it->second->showHidden());
}

void TreeWidget::slotRelabelDocument(const Gui::Document& Doc)
{
    std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        it->second->setText(0, QString::fromUtf8(Doc.getDocument()->Label.getValue()));
    }
}

void TreeWidget::slotActiveDocument(const Gui::Document& Doc)
{
    std::map<const Gui::Document*, DocumentItem*>::iterator jt = DocumentMap.find(&Doc);
    if (jt == DocumentMap.end())
        return; // signal is emitted before the item gets created
    int displayMode = FC_TREEPARAM(DocumentMode);
    for (std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.begin();
         it != DocumentMap.end(); ++it)
    {
        QFont f = it->second->font(0);
        f.setBold(it == jt);
        it->second->setHidden(0 == displayMode && it != jt);
        if (2 == displayMode) {
            it->second->setExpanded(it == jt);
        }
        // this must be done as last step
        it->second->setFont(0, f);
    }
}

void TreeWidget::onUpdateStatus(void)
{
    if (statusUpdateDelay<=0) {
        statusTimer->stop();
        if(isVisible()) {
            FC_LOG("update item status");
            std::map<const Gui::Document*,DocumentItem*>::iterator pos;
            for (pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
                pos->second->testStatus();
            }
        }
    }
    statusUpdateDelay = 0;
}

void TreeWidget::onItemEntered(QTreeWidgetItem * item)
{
    // object item selected
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
        objItem->displayStatusInfo();

        if(FC_TREEPARAM(PreSelection)) {
            if(preselectTime.elapsed() < 700)
                onPreSelectTimer();
            else{
                preselectTimer->start(500);
                Selection().rmvPreselect();
            }
        }
    } else if(FC_TREEPARAM(PreSelection))
        Selection().rmvPreselect();
}

void TreeWidget::leaveEvent(QEvent *) {
    if(FC_TREEPARAM(PreSelection)) {
        preselectTimer->stop();
        Selection().rmvPreselect();
    }
}

void TreeWidget::onPreSelectTimer() {
    if(!FC_TREEPARAM(PreSelection))
        return;
    auto item = itemAt(viewport()->mapFromGlobal(QCursor::pos()));
    if(!item || item->type()!=TreeWidget::ObjectType) 
        return;

    preselectTime.restart();
    DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
    auto vp = objItem->object();
    auto obj = vp->getObject();
    std::ostringstream ss;
    App::DocumentObject *parent = 0;
    objItem->getSubName(ss,parent);
    if(!parent)
        parent = obj;
    else if(!obj->redirectSubName(ss,parent,0))
        ss << obj->getNameInDocument() << '.';
    Selection().setPreselect(parent->getDocument()->getName(),parent->getNameInDocument(),
            ss.str().c_str(),0,0,0,2);
}

void TreeWidget::onItemCollapsed(QTreeWidgetItem * item)
{
    // object item collapsed
    if (item && item->type() == TreeWidget::ObjectType) {
        static_cast<DocumentObjectItem*>(item)->setExpandedStatus(false);
    }
}

void TreeWidget::onItemExpanded(QTreeWidgetItem * item)
{
    // object item expanded
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
        objItem->getOwnerDocument()->populateItem(objItem);
        objItem->setExpandedStatus(true);
    }
}

void TreeWidget::scrollItemToTop()
{
    auto doc = Application::Instance->activeDocument();
    for(auto tree : Instances) {
        if(!tree->isConnectionAttached()) 
            continue;

        if(doc && Gui::Selection().hasSelection(doc->getDocument()->getName(),false)) {
            std::map<const Gui::Document*,DocumentItem*>::iterator it;
            it = tree->DocumentMap.find(doc);
            if (it != tree->DocumentMap.end()) {
                bool lock = tree->blockConnection(true);
                it->second->selectItems(true);
                tree->blockConnection(lock);
            }
        } else {
            tree->blockConnection(true);
            for (int i=0; i<tree->rootItem->childCount(); i++) {
                auto docItem = dynamic_cast<DocumentItem*>(tree->rootItem->child(i));
                if(!docItem)
                    continue;
                auto doc = docItem->document()->getDocument();
                if(Gui::Selection().hasSelection(doc->getName())) {
                    tree->currentDocItem = docItem;
                    docItem->selectItems(true);
                    tree->currentDocItem = 0;
                    break;
                }
            }
            tree->blockConnection(false);
        }
    }
}

void TreeWidget::expandSelectedItems(TreeItemMode mode)
{
    if(!isConnectionAttached()) 
        return;

    for(auto item : selectedItems()) {
        switch (mode) {
        case Gui::Expand:
            item->setExpanded(true);
            break;
        case Gui::Collapse:
            item->setExpanded(false);
            break;
        case Gui::Toggle:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;
        }
    }
}


void TreeWidget::setupText() {
    this->headerItem()->setText(0, tr("Labels & Attributes"));
    this->headerItem()->setText(1, tr("Description"));
    this->rootItem->setText(0, tr("Application"));

    this->showHiddenAction->setText(tr("Show hidden items"));
    this->showHiddenAction->setStatusTip(tr("Show hidden tree view items"));

    this->hideInTreeAction->setText(tr("Hide item"));
    this->hideInTreeAction->setStatusTip(tr("Hide the item in tree"));

    this->createGroupAction->setText(tr("Create group..."));
    this->createGroupAction->setStatusTip(tr("Create a group"));

    this->relabelObjectAction->setText(tr("Rename"));
    this->relabelObjectAction->setStatusTip(tr("Rename object"));

    this->finishEditingAction->setText(tr("Finish editing"));
    this->finishEditingAction->setStatusTip(tr("Finish editing object"));

    this->reloadDocAction->setText(tr("Reload document"));
    this->reloadDocAction->setStatusTip(tr("Reload a partially loaded document"));

    this->skipRecomputeAction->setText(tr("Skip recomputes"));
    this->skipRecomputeAction->setStatusTip(tr("Enable or disable recomputations of document"));

    this->allowPartialRecomputeAction->setText(tr("Allow partial recomputes"));
    this->allowPartialRecomputeAction->setStatusTip(
            tr("Enable or disable recomputating editing object when 'skip recomputation' is enabled"));

    this->markRecomputeAction->setText(tr("Mark to recompute"));
    this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));

    this->recomputeObjectAction->setText(tr("Recompute object"));
    this->recomputeObjectAction->setStatusTip(tr("Recompute the selected object"));
}

void TreeWidget::syncView(ViewProviderDocumentObject *vp) {
    if(currentDocItem && FC_TREEPARAM(SyncView))
        currentDocItem->document()->setActiveView(vp);
}

void TreeWidget::onShowHidden() {
    if (!this->contextItem) return;
    DocumentItem *docItem = nullptr;
    if(this->contextItem->type() == DocumentType)
        docItem = static_cast<DocumentItem*>(contextItem);
    else if(this->contextItem->type() == ObjectType)
        docItem = static_cast<DocumentObjectItem*>(contextItem)->getOwnerDocument();
    if(docItem)
        docItem->setShowHidden(showHiddenAction->isChecked());
}

void TreeWidget::onHideInTree() {
    if (this->contextItem && this->contextItem->type() == ObjectType) {
        auto item = static_cast<DocumentObjectItem*>(contextItem);
        item->object()->ShowInTree.setValue(!hideInTreeAction->isChecked());
    }
}


void TreeWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        setupText();

    QTreeWidget::changeEvent(e);
}

void TreeWidget::onItemSelectionChanged ()
{
    if (!this->isConnectionAttached() || this->isConnectionBlocked())
        return;

    _LastSelectedTreeWidget = this;

    // block tmp. the connection to avoid to notify us ourself
    bool lock = this->blockConnection(true);

    auto selItems = selectedItems();
    if(selItems.size()<=1) {
        if(FC_TREEPARAM(RecordSelection))
            Gui::Selection().selStackPush();

        // This special handling to deal with possible discrepency of
        // Gui.Selection and Tree view selection because of newly added
        // DocumentObject::redirectSubName()
        Selection().clearCompleteSelection();
        DocumentObjectItem *item=0;
        if(selItems.size())
            item = dynamic_cast<DocumentObjectItem*>(selItems.front());
        for(auto &v : DocumentMap) {
            currentDocItem = v.second;
            v.second->clearSelection(item);
            currentDocItem = 0;
        }
        if(FC_TREEPARAM(RecordSelection))
            Gui::Selection().selStackPush();
    }else{
        std::map<const Gui::Document*,DocumentItem*>::iterator pos;
        for (pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
            currentDocItem = pos->second;
            pos->second->updateSelection(pos->second);
            currentDocItem = 0;
        }
        if(FC_TREEPARAM(RecordSelection))
            Gui::Selection().selStackPush(true,true);
    }

    this->blockConnection(lock);
}

void TreeWidget::onSelectTimer() {
    bool syncSelect = FC_TREEPARAM(SyncSelection);
    this->blockConnection(true);
    if(Selection().hasSelection()) {
        for(auto &v : DocumentMap) {
            currentDocItem = v.second;
            v.second->selectItems(syncSelect);
            currentDocItem = 0;
        }
    }else{
        for(auto &v : DocumentMap)
            v.second->clearSelection();
    }
    this->blockConnection(false);
    return;
}

void TreeWidget::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
    case SelectionChanges::RmvSelection:
    case SelectionChanges::SetSelection:
    case SelectionChanges::ClrSelection:
        selectTimer->start(100);
        break;
    default:
        break;
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreePanel */

TreePanel::TreePanel(const char *name, QWidget* parent)
  : QWidget(parent)
{
    this->treeWidget = new TreeWidget(name, this);
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    this->treeWidget->setIndentation(hGrp->GetInt("Indentation", this->treeWidget->indentation()));

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    pLayout->addWidget(this->treeWidget);
    connect(this->treeWidget, SIGNAL(emitSearchObjects()),
            this, SLOT(showEditor()));

    this->searchBox = new Gui::ClearLineEdit(this);
    pLayout->addWidget(this->searchBox);
    this->searchBox->hide();
    this->searchBox->installEventFilter(this);
#if QT_VERSION >= 0x040700
    this->searchBox->setPlaceholderText(tr("Search"));
#endif
    connect(this->searchBox, SIGNAL(returnPressed()),
            this, SLOT(accept()));
    connect(this->searchBox, SIGNAL(textEdited(QString)),
            this, SLOT(itemSearch(QString)));
}

TreePanel::~TreePanel()
{
}

void TreePanel::accept()
{
    QString text = this->searchBox->text();
    hideEditor();
    this->treeWidget->itemSearch(text,true);
}

bool TreePanel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != this->searchBox)
        return false;

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key) {
        case Qt::Key_Escape:
            hideEditor();
            consumed = true;
            break;

        default:
            break;
        }

        return consumed;
    }

    return false;
}

void TreePanel::showEditor()
{
    this->searchBox->show();
    this->searchBox->setFocus();
    this->treeWidget->startItemSearch();
}

void TreePanel::hideEditor()
{
    this->searchBox->clear();
    this->searchBox->hide();
    this->treeWidget->resetItemSearch();
    auto sels = this->treeWidget->selectedItems();
    if(sels.size())
        this->treeWidget->scrollToItem(sels.front());
}

void TreePanel::itemSearch(const QString &text)
{
    this->treeWidget->itemSearch(text,false);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreeDockWidget */

TreeDockWidget::TreeDockWidget(Gui::Document* pcDocument,QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle(tr("Tree view"));
    this->treeWidget = new TreeWidget("TreeView",this);
    this->treeWidget->setRootIsDecorated(false);
    GET_TREEVIEW_PARAM(hGrp);
    this->treeWidget->setIndentation(hGrp->GetInt("Indentation", this->treeWidget->indentation()));

    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    pLayout->addWidget(this->treeWidget, 0, 0 );
}

TreeDockWidget::~TreeDockWidget()
{
}

void TreeWidget::selectLinkedObject(App::DocumentObject *linked) { 
    if(!isConnectionAttached()) 
        return;

    auto linkedVp = dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(linked));
    if(!linkedVp) {
        TREE_ERR("invalid linked view provider");
        return;
    }
    auto linkedDoc = getDocumentItem(linkedVp->getDocument());
    if(!linkedDoc) {
        TREE_ERR("cannot find document of linked object");
        return;
    }

    auto it = linkedDoc->ObjectMap.find(linked);
    if(it == linkedDoc->ObjectMap.end()) {
        TREE_ERR("cannot find tree item of linked object");
        return;
    }
    auto linkedItem = it->second->rootItem;
    if(!linkedItem) 
        linkedItem = *it->second->items.begin();

    if(linkedDoc->showItem(linkedItem,true))
        scrollToItem(linkedItem);

    if(linkedDoc->document()->getDocument() != App::GetApplication().getActiveDocument())
        linkedDoc->document()->setActiveView(linkedItem->object());
}

// ----------------------------------------------------------------------------

DocumentItem::DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent)
    : QTreeWidgetItem(parent, TreeWidget::DocumentType), pDocument(const_cast<Gui::Document*>(doc))
{
    // Setup connections
    connectNewObject = doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, _1));
    connectDelObject = doc->signalDeletedObject.connect(
            boost::bind(&TreeWidget::slotDeleteObject, getTree(), _1));
    if(!App::GetApplication().isRestoring())
        connectChgObject = doc->signalChangedObject.connect(
                boost::bind(&TreeWidget::slotChangeObject, getTree(), _1, _2, false));
    connectEdtObject = doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, _1));
    connectResObject = doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, _1));
    connectHltObject = doc->signalHighlightObject.connect(
            boost::bind(&DocumentItem::slotHighlightObject, this, _1,_2,_3,_4,_5));
    connectExpObject = doc->signalExpandObject.connect(boost::bind(&DocumentItem::slotExpandObject, this, _1,_2));
    connectScrObject = doc->signalScrollToObject.connect(boost::bind(&DocumentItem::slotScrollToObject, this, _1));
    auto adoc = doc->getDocument();
    connectRecomputed = adoc->signalRecomputed.connect(boost::bind(&DocumentItem::slotRecomputed, this, _1, _2));
    connectUndo = adoc->signalUndo.connect(boost::bind(&DocumentItem::slotTransactionDone, this, _1));
    connectRedo = adoc->signalRedo.connect(boost::bind(&DocumentItem::slotTransactionDone, this, _1));

    setFlags(Qt::ItemIsEnabled/*|Qt::ItemIsEditable*/);

    treeName = getTree()->getTreeName();
}

DocumentItem::~DocumentItem()
{
    connectNewObject.disconnect();
    connectDelObject.disconnect();
    connectChgObject.disconnect();
    connectEdtObject.disconnect();
    connectResObject.disconnect();
    connectHltObject.disconnect();
    connectExpObject.disconnect();
    connectScrObject.disconnect();
    connectRecomputed.disconnect();
    connectUndo.disconnect();
    connectRedo.disconnect();
}

TreeWidget *DocumentItem::getTree() const{
    return static_cast<TreeWidget*>(treeWidget());
}

const char *DocumentItem::getTreeName() const {
    return treeName;
}

#define FOREACH_ITEM(_item, _obj) \
    auto _it = ObjectMap.end();\
    if(_obj.getObject() && _obj.getObject()->getNameInDocument())\
        _it = ObjectMap.find(_obj.getObject());\
    if(_it != ObjectMap.end()) {\
        for(auto _item : _it->second->items) {

#define FOREACH_ITEM_ALL(_item) \
    for(auto _v : ObjectMap) {\
        for(auto _item : _v.second->items) {

#define END_FOREACH_ITEM }}


void DocumentItem::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    (void)v;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    unsigned long col = hGrp->GetUnsigned("TreeEditColor",4294902015);
    QColor color((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff);

    if(!getTree()->editingItem) {
        auto doc = Application::Instance->editDocument();
        if(!doc)
            return;
        ViewProviderDocumentObject *parentVp=0;
        std::string subname;
        auto vp = doc->getInEdit(&parentVp,&subname);
        if(!parentVp)
            parentVp = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if(parentVp)
            getTree()->editingItem = findItemByObject(true,parentVp->getObject(),subname.c_str());
    }

    if(getTree()->editingItem)
        getTree()->editingItem->setBackground(0,color);
    else{
        FOREACH_ITEM(item,v)
            item->setBackground(0,color);
        END_FOREACH_ITEM
    }
}

void DocumentItem::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    auto tree = getTree();
    FOREACH_ITEM_ALL(item)
        if(tree->editingItem) {
            if(item == tree->editingItem) {
                item->setData(0, Qt::BackgroundColorRole,QVariant());
                break;
            }
        }else if(item->object() == &v)
            item->setData(0, Qt::BackgroundColorRole,QVariant());
    END_FOREACH_ITEM
    tree->editingItem = 0;
}

void DocumentItem::slotNewObject(const Gui::ViewProviderDocumentObject& obj) {
    if(pDocument->getDocument()->isPerformingTransaction()) {
        // We have to delay item creation until undo/redo is done, because the
        // object re-creation while in transaction may break tree view item
        // update logic. For example, a parent object re-created before its
        // children, but the parent's link property already contains all the
        // (detached) children.
        TransactingObjects.push_back(obj.getObject()->getID());
    }else
        createNewItem(obj);
}

void DocumentItem::slotTransactionDone(const App::Document& doc) {
    for(auto id : TransactingObjects) {
        auto obj = doc.getObjectByID(id);
        if(!obj || ObjectMap.find(obj)!=ObjectMap.end())
            continue;
        auto vpd = dynamic_cast<ViewProviderDocumentObject*>(pDocument->getViewProvider(obj));
        if(vpd)
            createNewItem(*vpd);
    }
    TransactingObjects.clear();
}

bool DocumentItem::createNewItem(const Gui::ViewProviderDocumentObject& obj,
            QTreeWidgetItem *parent, int index, DocumentObjectDataPtr data)
{
    const char *name;
    if (!obj.getObject() || 
        !(name=obj.getObject()->getNameInDocument()) ||
        obj.getObject()->testStatus(App::PartialObject))
        return false;

    if(!data) {
        auto &pdata = ObjectMap[obj.getObject()];
        if(!pdata) {
            pdata = std::make_shared<DocumentObjectData>(
                    this, const_cast<ViewProviderDocumentObject*>(&obj));
            auto &entry = getTree()->ObjectTable[obj.getObject()];
            if(entry.size())
                pdata->updateChildren(*entry.begin());
            else
                pdata->updateChildren(true);
            entry.insert(pdata);
        }else if(pdata->rootItem && parent==NULL) {
            Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
            return false;
        }
        data = pdata;
    }

    DocumentObjectItem* item = new DocumentObjectItem(this,data);
    if(!parent || parent==this) {
        parent = this;
        data->rootItem = item;
    }
    if(index<0)
        parent->addChild(item);
    else
        parent->insertChild(index,item);
    assert(item->parent() == parent);
    item->setText(0, QString::fromUtf8(data->label.c_str()));
    if(data->label2.size())
        item->setText(1, QString::fromUtf8(data->label2.c_str()));
    if(!obj.showInTree() && !showHidden())
        item->setHidden(true);
    populateItem(item);

    // Not calling item testStatus below because there seems to have some delay
    // between new object, and its visual status update. Need to figure out why
    // item->testStatus(true);
    getTree()->updateStatus(true);
    return true;
}

ViewProviderDocumentObject *DocumentItem::getViewProvider(App::DocumentObject *obj) {
    // Note: It is possible that we receive an invalid pointer from
    // claimChildren(), e.g. if multiple properties were changed in
    // a transaction and slotChangedObject() is triggered by one
    // property being reset before the invalid pointer has been
    // removed from another. Currently this happens for
    // PartDesign::Body when cancelling a new feature in the dialog.
    // First the new feature is deleted, then the Tip property is
    // reset, but claimChildren() accesses the Model property which
    // still contains the pointer to the deleted feature
    //
    // return obj && obj->getNameInDocument() && pDocument->isIn(obj);
    //
    // TODO: is the above isIn() check still necessary? Will
    // getNameInDocument() check be sufficient?


    if(!obj || !obj->getNameInDocument()) return 0;

    ViewProvider *vp;
    if(obj->getDocument() == pDocument->getDocument()) 
        vp = pDocument->getViewProvider(obj);
    else 
        vp = Application::Instance->getViewProvider(obj);
    if(!vp || !vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
        return 0;
    return static_cast<ViewProviderDocumentObject*>(vp);
}

void TreeWidget::slotDeleteDocument(const Gui::Document& Doc)
{
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        auto docItem = it->second;
        for(auto &v : docItem->ObjectMap) {
            for(auto item : v.second->items)
                item->myOwner = 0;
            auto obj = v.second->viewObject->getObject();
            if(obj->getDocument() == Doc.getDocument()) {
                _slotDeleteObject(*v.second->viewObject, docItem);
                continue;
            }
            auto it = ObjectTable.find(obj);
            assert(it!=ObjectTable.end());
            assert(it->second.size()>1);
            it->second.erase(v.second);
        }
        this->rootItem->takeChild(this->rootItem->indexOfChild(docItem));
        delete docItem;
        DocumentMap.erase(it);
    }
}

void TreeWidget::slotFinishRestoreDocument(const App::Document& Doc)
{
    auto doc = Application::Instance->getDocument(&Doc);
    if(!doc) return;
    auto it = DocumentMap.find(doc);
    if(it==DocumentMap.end())
        return;

    auto docItem = it->second;
    docItem->connectChgObject = docItem->document()->signalChangedObject.connect(
            boost::bind(&TreeWidget::slotChangeObject, this, _1, _2, false));
    App::PropertyBool dummy;
    for(auto &v : docItem->ObjectMap)
        slotChangeObject(*v.second->viewObject,dummy,true);

    for(auto &v : docItem->ObjectMap) {
        if(v.second->viewObject->Visibility.getValue() &&
           !docItem->isObjectShowable(v.second->viewObject->getObject()))
        {
           v.second->viewObject->hide();
        }
        if(v.first->isError()
                && v.second->items.size() 
                && !Doc.testStatus(App::Document::PartialDoc))
        {
            auto item = v.second->rootItem;
            if(!item) {
                item = *v.second->items.begin();
                docItem->showItem(item,false,true);
            }
            if(!errItem)
                errItem = item;
        }
    }

    if(!Doc.testStatus(App::Document::PartialDoc))
        return;

    auto item = it->second;
    this->collapseItem(item);

    item->setIcon(0, *documentPartialPixmap);
}

void TreeWidget::slotDeleteObject(const Gui::ViewProviderDocumentObject& view) {
    _slotDeleteObject(view, 0);
}

void TreeWidget::_slotDeleteObject(const Gui::ViewProviderDocumentObject& view, DocumentItem *deletingDoc)
{
    auto obj = view.getObject();
    auto itEntry = ObjectTable.find(obj);
    if(itEntry == ObjectTable.end())
        return;

    if(itEntry->second.empty()) {
        ObjectTable.erase(itEntry);
        return;
    }

    TREE_LOG("delete object " << obj->getFullName());

    for(auto data : itEntry->second) {
        DocumentItem *docItem = data->docItem;
        if(docItem == deletingDoc)
            continue;

        auto doc = docItem->document()->getDocument();
        auto &items = data->items;

        if(obj->getDocument() == doc)
            docItem->_ParentMap.erase(obj);

        for(auto cit=items.begin(),citNext=cit;cit!=items.end();cit=citNext) {
            ++citNext;
            (*cit)->myOwner = 0;
            delete *cit;
        }

        // Check for any child of the deleted object that is not in the tree, and put it
        // under document item.
        for(auto child : data->children) {
            if(!child || !child->getNameInDocument() || child->getDocument()!=doc)
                continue;
            docItem->_ParentMap[child].erase(obj);
            auto cit = docItem->ObjectMap.find(child);
            if(cit==docItem->ObjectMap.end() || cit->second->items.empty()) {
                auto vpd = docItem->getViewProvider(child);
                if(!vpd) continue;
                docItem->createNewItem(*vpd);
            }else {
                auto childItem = *cit->second->items.begin();
                if(childItem->requiredAtRoot(false))
                    docItem->createNewItem(*childItem->object(),docItem,-1,childItem->myData);
            }
            if(child->Visibility.getValue() && !docItem->isObjectShowable(child))
                child->Visibility.setValue(false);
        }
        docItem->ObjectMap.erase(obj);
    }
    ObjectTable.erase(itEntry);
}

bool DocumentItem::populateObject(App::DocumentObject *obj) {
    // make sure at least one of the item corresponding to obj is populated
    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end())
        return false;
    auto &items = it->second->items;
    if(items.empty())
        return false;
    for(auto item : items) {
        if(item->populated)
            return true;
    }
    TREE_LOG("force populate object " << obj->getFullName());
    auto item = *items.begin();
    item->populated = true;
    populateItem(item,true);
    return true;
}

void DocumentItem::populateItem(DocumentObjectItem *item, bool refresh)
{
    if (item->populated && !refresh)
        return;

    // Lazy loading policy: We will create an item for each children object if
    // a) the item is expanded, or b) there is at least one free child, i.e.
    // child originally located at root.

    item->setChildIndicatorPolicy(item->myData->children.empty()?
            QTreeWidgetItem::DontShowIndicator:QTreeWidgetItem::ShowIndicator);

    if (!item->populated && !item->isExpanded()) {
        bool doPopulate = false;

        bool external = item->object()->getDocument()!=item->getOwnerDocument()->document();
        if(external)
            return;
        auto obj = item->object()->getObject();
        auto linked = obj->getLinkedObject(true);
        if (linked && linked->getDocument()!=obj->getDocument())
            return;
        for(auto child : item->myData->children) {
            auto it = ObjectMap.find(child);
            if(it == ObjectMap.end() || it->second->items.empty()) {
                auto vp = getViewProvider(child);
                if(!vp) continue;
                doPopulate = true;
                break;
            }
            if(item->myData->removeChildrenFromRoot) {
                if(it->second->rootItem) {
                    doPopulate = true;
                    break;
                }
            }
        }

        if (!doPopulate)
            return;
    }

    item->populated = true;
    bool checkHidden = !showHidden();

    int i=-1;
    // iterate through the claimed children, and try to synchronize them with the 
    // children tree item with the same order of apperance. 
    int childCount = item->childCount();
    for(auto child : item->myData->children) {

        ++i; // the current index of the claimed child

        bool found = false;
        for (int j=i;j<childCount;++j) {
            QTreeWidgetItem *ci = item->child(j);
            if (ci->type() != TreeWidget::ObjectType)
                continue;

            DocumentObjectItem *childItem = static_cast<DocumentObjectItem*>(ci);
            if (childItem->object()->getObject() != child)
                continue;

            found = true;
            if (j!=i) { // fix index if it is changed
                childItem->setHighlight(false);
                item->removeChild(ci);
                item->insertChild(i,ci);
                assert(ci->parent()==item);
                if(checkHidden)
                    updateItemsVisibility(ci,false);
            }

            // Check if the item just changed its policy of whether to remove
            // children item from the root. 
            if(item->myData->removeChildrenFromRoot) {
                if(childItem->myData->rootItem) {
                    assert(childItem != childItem->myData->rootItem);
                    bool lock = getTree()->blockConnection(true);
                    delete childItem->myData->rootItem;
                    getTree()->blockConnection(lock);
                }
            }else if(childItem->requiredAtRoot())
                createNewItem(*childItem->object(),this,-1,childItem->myData);
            break;
        }

        if (found)
            continue;

        // This algo will be recursively applied to newly created child items
        // through slotNewObject -> populateItem

        auto it = ObjectMap.find(child);
        if(it==ObjectMap.end() || it->second->items.empty()) {
            auto vp = getViewProvider(child);
            if(!vp || !createNewItem(*vp,item,i,it==ObjectMap.end()?DocumentObjectDataPtr():it->second))
                --i;
            continue;
        }

        if(!item->myData->removeChildrenFromRoot || !it->second->rootItem) {
            DocumentObjectItem *childItem = *it->second->items.begin();
            if(!createNewItem(*childItem->object(),item,i,it->second))
                --i;
        }else {
            DocumentObjectItem *childItem = it->second->rootItem;
            if(item->isChildOfItem(childItem)) {
                TREE_ERR("Cyclic dependency in " 
                    << item->object()->getObject()->getFullName()
                    << '.' << childItem->object()->getObject()->getFullName());
                --i;
                continue;
            }
            it->second->rootItem = 0;
            childItem->setHighlight(false);
            this->removeChild(childItem);
            item->insertChild(i,childItem);
            assert(childItem->parent()==item);
            if(checkHidden)
                updateItemsVisibility(childItem,false);
        }
    }

    for (++i;item->childCount()>i;) {
        QTreeWidgetItem *ci = item->child(i);
        if (ci->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* childItem = static_cast<DocumentObjectItem*>(ci);
            if(childItem->requiredAtRoot()) {
                item->removeChild(childItem);
                this->addChild(childItem);
                assert(childItem->parent()==this);
                if(checkHidden)
                    updateItemsVisibility(childItem,false);
                childItem->myData->rootItem = childItem;
                continue;
            }
        }

        bool lock = getTree()->blockConnection(true);
        delete ci;
        getTree()->blockConnection(lock);
    }
    getTree()->updateGeometries();
}

void DocumentItem::checkRemoveChildrenFromRoot(const Gui::ViewProviderDocumentObject& view)
{
    auto it = ObjectMap.find(view.getObject());
    if(it != ObjectMap.end() && 
       it->second->removeChildrenFromRoot!=view.canRemoveChildrenFromRoot()) 
    {
        it->second->removeChildrenFromRoot = !it->second->removeChildrenFromRoot;
        for(auto item : it->second->items)
            populateItem(item,true);
    }
}

void TreeWidget::slotChangeObject(
        const Gui::ViewProviderDocumentObject& view, const App::Property &prop, bool force) {

    auto obj = view.getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    _updateStatus(true);

    // Let's not waste time on the newly added Visibility property in
    // DocumentObject.
    if(&prop == &obj->Visibility)
        return;

    auto itEntry = ObjectTable.find(obj);
    if(itEntry == ObjectTable.end() || itEntry->second.empty())
        return;

    if(force || &prop == &obj->Label) {
        const char *label = obj->Label.getValue();
        auto firstData = *itEntry->second.begin();
        if(firstData->label != label) {
            for(auto data : itEntry->second) {
                data->label = label;
                auto displayName = QString::fromUtf8(label);
                for(auto item : data->items)
                    item->setText(0, displayName);
            }
        }
        if(!force)
            return;
    }

    if(force || &prop == &obj->Label2) {
        const char *label = obj->Label2.getValue();
        auto firstData = *itEntry->second.begin();
        if(firstData->label2 != label) {
            for(auto data : itEntry->second) {
                data->label2 = label;
                auto displayName = QString::fromUtf8(label);
                for(auto item : data->items)
                    item->setText(1, displayName);
            }
        }
        if(!force)
            return;
    }

    bool childrenChanged = false;
    std::vector<App::DocumentObject*> children;
    bool removeChildrenFromRoot = view.canRemoveChildrenFromRoot();

    DocumentObjectDataPtr found;
    for(auto data : itEntry->second) {
        if(!found) {
            found = data;
            childrenChanged = found->updateChildren(force);
            if(!childrenChanged && found->removeChildrenFromRoot==removeChildrenFromRoot)
                return;
        }else if(childrenChanged)
            data->updateChildren(found);
        data->removeChildrenFromRoot = removeChildrenFromRoot;
        DocumentItem* docItem = data->docItem;
        for(auto item : data->items)
            docItem->populateItem(item,true);
    }

    if(force)
        return;

    if(childrenChanged && prop.testStatus(App::Property::Output)) {
        // When a property is marked as output, it will not touch its object,
        // and thus, its property change will not be propagated through
        // recomputation. So we have to manually check for each links here.
        for(auto link : App::GetApplication().getLinksTo(obj,true)) {
            std::vector<App::DocumentObject*> linkedChildren;
            DocumentObjectDataPtr found;
            auto it = ObjectTable.find(link);
            if(it == ObjectTable.end())
                continue;
            for(auto data : it->second) {
                if(!found) {
                    found = data;
                    if(!found->updateChildren(false))
                        break;
                }
                data->updateChildren(found);
                DocumentItem* docItem = data->docItem;
                for(auto item : data->items)
                    docItem->populateItem(item,true);
            }
        }
    }

    if(childrenChanged) {
        //if the item is in a GeoFeatureGroup we may need to update that too, as the claim children 
        //of the geofeaturegroup depends on what the childs claim
        auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(view.getObject());
        if(grp) {
            auto vp = dynamic_cast<ViewProviderDocumentObject*>(
                    Application::Instance->getViewProvider(grp));
            if (vp) {
                App::PropertyBool dummy;
                slotChangeObject(*vp,dummy,false);
            }
        }
    }
}
    
void DocumentItem::slotHighlightObject (const Gui::ViewProviderDocumentObject& obj, 
    const Gui::HighlightMode& high, bool set, const App::DocumentObject *parent, const char *subname)
{
    if(parent && parent->getDocument()!=document()->getDocument()) {
        auto it = getTree()->DocumentMap.find(Application::Instance->getDocument(parent->getDocument()));
        if(it!=getTree()->DocumentMap.end())
            it->second->slotHighlightObject(obj,high,set,parent,subname);
        return;
    }
    FOREACH_ITEM(item,obj)
        if(parent) {
            App::DocumentObject *topParent = 0;
            std::ostringstream ss;
            item->getSubName(ss,topParent);
            if(!topParent) {
                if(parent!=obj.getObject())
                    continue;
            }else if(topParent!=parent)
                continue;
        }
        item->setHighlight(set,high);
        if(parent)
            return;
    END_FOREACH_ITEM
}

void DocumentItem::slotExpandObject (const Gui::ViewProviderDocumentObject& obj,const Gui::TreeItemMode& mode)
{
    // In the past it was checked if the parent item is collapsed and if yes nothing was done.
    // Now with the auto-expand mechanism of active part containers or bodies it must be made
    // sure to expand all parent items when expanding a child item.
    // Example:
    // When there are two nested part containers and if first the outer and then the inner is
    // activated the outer will be collapsed and thus hides the inner item.
    // Alternatively, this could be handled inside ActiveObjectList::setObject() but querying
    // the parent-children relationship of the view providers is rather inefficient.
    FOREACH_ITEM(item,obj)
        // All document object items must always have a parent, either another
        // object item or document item. If not, then there is a bug somewhere
        // else.
        assert(item->parent());
        switch (mode) {
        case Gui::Expand: {
            QTreeWidgetItem* parent = item->parent();
            while (parent) {
                parent->setExpanded(true);
                parent = parent->parent();
            }
            item->setExpanded(true);
        }   break;
        case Gui::Collapse:
            item->setExpanded(false);
            break;
        case Gui::Toggle:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;

        default:
            // not defined enum
            assert(0);
        }
        populateItem(item);
    END_FOREACH_ITEM
}

void DocumentItem::slotScrollToObject(const Gui::ViewProviderDocumentObject& obj)
{
    if(!obj.getObject() || !obj.getObject()->getNameInDocument())
        return;
    auto it = ObjectMap.find(obj.getObject());
    if(it == ObjectMap.end() || it->second->items.empty()) 
        return;
    auto item = it->second->rootItem;
    if(!item)
        item = *it->second->items.begin();
    getTree()->scrollToItem(item);
}

void DocumentItem::slotRecomputed(const App::Document &, const std::vector<App::DocumentObject*> &objs) {
    auto tree = getTree();
    if(!tree->isVisible()) return;
    bool scrolled = false;
    for(auto obj : objs) {
        if(obj->isValid()) 
            continue;
        auto it = ObjectMap.find(obj);
        if(it == ObjectMap.end() || it->second->items.empty()) {
            auto itDoc = getTree()->DocumentMap.find(
                    Application::Instance->getDocument(obj->getDocument()));
            if(itDoc!=getTree()->DocumentMap.end()) {
                it = itDoc->second->ObjectMap.find(obj);
                if(it == ObjectMap.end() || it->second->items.empty()) {
                    FC_ERR("Cannot find recompute failure object " << obj->getFullName());
                    continue;
                }
            }
        }
        auto item = it->second->rootItem;
        if(!item)
            item = *it->second->items.begin();
        showItem(item,false,true);
        if(!scrolled) {
            scrolled = true;
            tree->scrollToItem(item);
        }
    }
}

Gui::Document* DocumentItem::document() const
{
    return this->pDocument;
}

//void DocumentItem::markItem(const App::DocumentObject* Obj,bool mark)
//{
//    // never call without Object!
//    assert(Obj);
//
//
//    std::map<std::string,DocumentObjectItem*>::iterator pos;
//    pos = ObjectMap.find(Obj);
//    if (pos != ObjectMap.end()) {
//        QFont f = pos->second->font(0);
//        f.setUnderline(mark);
//        pos->second->setFont(0,f);
//    }
//}

void DocumentItem::testStatus(void)
{
    for(const auto &v : ObjectMap)
        v.second->testStatus();
}

void DocumentItem::setData (int column, int role, const QVariant & value)
{
    if (role == Qt::EditRole) {
        QString label = value.toString();
        pDocument->getDocument()->Label.setValue((const char*)label.toUtf8());
    }

    QTreeWidgetItem::setData(column, role, value);
}

void DocumentItem::clearSelection(DocumentObjectItem *exclude)
{
    // Block signals here otherwise we get a recursion and quadratic runtime
    bool ok = treeWidget()->blockSignals(true);
    FOREACH_ITEM_ALL(item);
        if(item==exclude) {
            item->selected = 0;
            updateItemSelection(item);
        }else{
            item->selected = 0;
            item->mySubs.clear();
            item->setSelected(false);
        }
    END_FOREACH_ITEM;
    treeWidget()->blockSignals(ok);
}

void DocumentItem::updateSelection(QTreeWidgetItem *ti, bool unselect) {
    for(int i=0,count=ti->childCount();i<count;++i) {
        auto child = ti->child(i);
        if(child && child->type()==TreeWidget::ObjectType) {
            auto childItem = static_cast<DocumentObjectItem*>(child);
            if(unselect) 
                childItem->setSelected(false);
            updateItemSelection(childItem);
            if(unselect && childItem->isGroup()) {
                // If the child item being force unselected by its group parent
                // is itself a group, propagate the unselection to its own
                // children
                updateSelection(childItem,true);
            }
        }
    }
        
    if(unselect) return;
    for(int i=0,count=ti->childCount();i<count;++i)
        updateSelection(ti->child(i));
}

void DocumentItem::updateItemSelection(DocumentObjectItem *item) {
    bool selected = item->isSelected();
    if((selected && item->selected) || (!selected && !item->selected)) 
        return;
    item->mySubs.clear();
    item->selected = selected;

    auto obj = item->object()->getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    std::ostringstream str;
    App::DocumentObject *topParent = 0;
    item->getSubName(str,topParent);
    if(topParent) {
        if(topParent->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            // remove legacy selection, i.e. those without subname
            Gui::Selection().rmvSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(),0);
        }
        if(!obj->redirectSubName(str,topParent,0))
            str << obj->getNameInDocument() << '.';
        obj = topParent;
    }
    const char *objname = obj->getNameInDocument();
    const char *docname = obj->getDocument()->getName();
    const auto &subname = str.str();

    if(subname.size()) {
        auto parentItem = item->getParentItem();
        assert(parentItem);
        if(selected && parentItem->selected) {
            // When a group item is selected, all its children objects are
            // highlighted in the 3D view. So, when an item of some group is
            // newly selected, we must force unselect its parent in order to
            // show the selection highlight. Besides, select both the parent
            // group and its children doesn't make much sense.
            //
            // UPDATE: There are legit use case of both parent and child
            // selection, for example, to disambiguate under which group to
            // operate on the child.
            //
            // TREE_TRACE("force unselect parent");
            // parentItem->setSelected(false);
            // updateItemSelection(parentItem);
        }
    }

    if(selected && item->isGroup()) {
        // Same reasoning as above. When a group item is newly selected, We
        // choose to force unselect all its children to void messing up the
        // selection highlight 
        //
        // UPDATE: same as above, child and parent selection is now re-enabled.
        //
        // TREE_TRACE("force unselect all children");
        // updateSelection(item,true);
    }

    if(!selected)
        Gui::Selection().rmvSelection(docname,objname,subname.c_str());
    else if(!Gui::Selection().addSelection(docname,objname,subname.c_str())) {
        item->selected = 0;
        item->setSelected(false);
        item->mySubs.clear();
    }else 
        getTree()->syncView(item->object());
}

App::DocumentObject *DocumentItem::getTopParent(App::DocumentObject *obj, std::string &subname) {
    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end() || it->second->items.empty())
        return 0;

    // already a top parent
    if(it->second->rootItem)
        return obj;

    for(auto item : it->second->items) {
        // non group object do not provide a cooridnate system, hence its
        // claimed child is still in the global coordinate space, so the
        // child can still be considered a top level object
        if(!item->isParentGroup())
            return obj;
    }

    // If no top level item, find an item that is closest to the top level
    std::multimap<int,DocumentObjectItem*> items;
    for(auto item : it->second->items) {
        int i=0;
        for(auto parent=item->parent();parent;++i,parent=parent->parent()) {
            if(parent->isHidden())
                i += 1000;
            ++i;
        }
        items.emplace(i,item);
    }

    App::DocumentObject *topParent = 0;
    std::ostringstream ss;
    items.begin()->second->getSubName(ss,topParent);
    if(!topParent) {
        // this shouldn't happen
        FC_WARN("No top parent for " << obj->getFullName() << '.' << subname);
        return obj;
    }
    ss << obj->getNameInDocument() << '.' << subname;
    FC_LOG("Subname correction " << obj->getFullName() << '.' << subname 
            << " -> " << topParent->getFullName() << '.' << ss.str());
    subname = ss.str();
    return topParent;
}

DocumentObjectItem *DocumentItem::findItemByObject(
        bool sync, App::DocumentObject *obj, const char *subname, bool select) 
{
    if(!subname)
        subname = "";

    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end() || it->second->items.empty())
        return 0;

    // prefer top level item of this object
    if(it->second->rootItem) 
        return findItem(sync,it->second->rootItem,subname,select);

    for(auto item : it->second->items) {
        // non group object do not provide a cooridnate system, hence its
        // claimed child is still in the global coordinate space, so the
        // child can still be considered a top level object
        if(!item->isParentGroup()) 
            return findItem(sync,item,subname,select);
    }

    // If no top level item, find an item that is closest to the top level
    std::multimap<int,DocumentObjectItem*> items;
    for(auto item : it->second->items) {
        int i=0;
        for(auto parent=item->parent();parent;++i,parent=parent->parent())
            ++i;
        items.emplace(i,item);
    }
    for(auto &v : items) {
        auto item = findItem(sync,v.second,subname,select);
        if(item)
            return item;
    }
    return 0;
}

DocumentObjectItem *DocumentItem::findItem(
        bool sync, DocumentObjectItem *item, const char *subname, bool select) 
{
    if(item->isHidden()) 
        item->setHidden(false);

    if(!subname || *subname==0) {
        if(select) {
            item->selected+=2;
            item->mySubs.clear();
        }
        return item;
    }

    TREE_TRACE("find next " << subname);

    // try to find the next level object name
    const char *nextsub = 0;
    const char *dot = 0;
    if((dot=strchr(subname,'.'))) 
        nextsub = dot+1;
    else {
        if(select) {
            item->selected+=2;
            if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    std::string name(subname,nextsub-subname);
    auto obj = item->object()->getObject();
    auto subObj = obj->getSubObject(name.c_str());
    if(!subObj || subObj==obj) {
        if(!subObj && !getTree()->searchDoc)
            TREE_WARN("sub object not found " << item->getName() << '.' << name.c_str());
        if(select) {
            item->selected += 2;
            if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    if(select)
        item->mySubs.clear();

    if(!item->populated && sync) {
        //force populate the item
        item->populated = true;
        populateItem(item,true);
    }

    for(int i=0,count=item->childCount();i<count;++i) {
        auto ti = item->child(i);
        if(!ti || ti->type()!=TreeWidget::ObjectType) continue;
        auto child = static_cast<DocumentObjectItem*>(ti);

        if(child->object()->getObject() == subObj) 
            return findItem(sync,child,nextsub,select);
    }

    // The sub object is not found. This could happen for geo group, since its
    // children may be in more than one hierarchy down.
    bool found = false;
    DocumentObjectItem *res=0;
    auto it = ObjectMap.find(subObj);
    if(it != ObjectMap.end()) {
        for(auto child : it->second->items) {
            if(child->isChildOfItem(item)) {
                found = true;
                res = findItem(sync,child,nextsub,select);
                if(!select)
                    return res;
            }
        }
    }

    if(select && !found) {
        // The sub object is still not found. Maybe it is a non-object sub-element.
        // Select the current object instead.
        TREE_TRACE("element " << subname << " not found");
        item->selected+=2;
        if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
            item->mySubs.push_back(subname);
    }
    return res;
}

void DocumentItem::selectItems(bool sync) {
    const auto &sels = Selection().getSelection(pDocument->getDocument()->getName(),false);
    for(const auto &sel : sels)
        findItemByObject(sync,sel.pObject,sel.SubName,true);

    DocumentObjectItem *first = 0;
    DocumentObjectItem *last = 0;

    FOREACH_ITEM_ALL(item)
        if(item->selected == 1) {
            // this means it is the old selection and is not in the current
            // selection
            item->selected = 0;
            item->setSelected(false);
        }else if(item->selected) {
            if(!first && item->selected==2) 
                first = item;
            item->selected = 1;
            item->setSelected(true);
            last = item;
        }
    END_FOREACH_ITEM;

    if(sync) {
        if(!first)
            first = last;
        if(first) {
            getTree()->scrollToItem(first);
            getTree()->syncView(first->object());
        }
    }
}

void DocumentItem::populateParents(const ViewProvider *vp, ViewParentMap &parentMap) {
    auto it = parentMap.find(vp);
    if(it == parentMap.end()) return;
    for(auto parent : it->second) {
        auto it = ObjectMap.find(parent->getObject());
        if(it==ObjectMap.end())
            continue;

        populateParents(parent,parentMap);
        for(auto item : it->second->items) {
            if(!item->isHidden() && !item->populated) {
                item->populated = true;
                populateItem(item,true);
            }
        }
    }
}

void DocumentItem::selectAllInstances(const ViewProviderDocumentObject &vpd) {
    ViewParentMap parentMap;
    auto pObject = vpd.getObject();
    if(ObjectMap.find(pObject) == ObjectMap.end())
        return;

    bool lock = getTree()->blockConnection(true);

    // We are trying to select all items corresponding to a given view
    // provider, i.e. all apperance of the object inside all its parent items
    //
    // Build a map of object to all its parent    
    for(auto &v : ObjectMap) {
        if(v.second->viewObject == &vpd) continue;
        for(auto child : v.second->viewObject->claimChildren()) {
            auto vp = getViewProvider(child);
            if(!vp) continue;
            parentMap[vp].push_back(v.second->viewObject);
        }
    }

    // now make sure all parent items are populated. In order to do that, we
    // need to populate the oldest parent first
    populateParents(&vpd,parentMap);

    DocumentObjectItem *first = 0;
    FOREACH_ITEM(item,vpd);
        if(showItem(item,true) && !first)
            first = item;
    END_FOREACH_ITEM;

    getTree()->blockConnection(lock);
    if(first) {
        treeWidget()->scrollToItem(first);
        updateSelection();
    }
}

bool DocumentItem::showHidden() const {
    return pDocument->getDocument()->ShowHidden.getValue();
}

void DocumentItem::setShowHidden(bool show) {
    pDocument->getDocument()->ShowHidden.setValue(show);
}

bool DocumentItem::showItem(DocumentObjectItem *item, bool select, bool force) {
    auto parent = item->parent();
    if(item->isHidden()) {
        if(!force)
            return false;
        item->setHidden(false);
    }
    
    if(parent->type()==TreeWidget::ObjectType && 
       !showItem(static_cast<DocumentObjectItem*>(parent),false))
        return false;

    parent->setExpanded(true);
    if(select) item->setSelected(true);
    return true;
}

void DocumentItem::setItemVisibility(const ViewProviderDocumentObject &vpd) {
    bool show = showHidden();
    FOREACH_ITEM(item,vpd);
        item->setHidden(!vpd.showInTree() && !show);
        item->testStatus(false);
    END_FOREACH_ITEM;
}

void DocumentItem::updateItemsVisibility(QTreeWidgetItem *item, bool show) {
    if(item->type() == TreeWidget::ObjectType) {
        auto objitem = static_cast<DocumentObjectItem*>(item);
        objitem->setHidden(!show && !objitem->object()->showInTree());
    }
    for(int i=0;i<item->childCount();++i) 
        updateItemsVisibility(item->child(i),show);
}

void DocumentItem::updateSelection() {
    bool lock = getTree()->blockConnection(true);
    updateSelection(this,false);
    getTree()->blockConnection(lock);
}

// ----------------------------------------------------------------------------

static int countItems;

DocumentObjectItem::DocumentObjectItem(DocumentItem *ownerDocItem, DocumentObjectDataPtr data)
    : QTreeWidgetItem(TreeWidget::ObjectType)
    , myOwner(ownerDocItem), myData(data), previousStatus(-1),selected(0),populated(false)
{
    setFlags(flags()|Qt::ItemIsEditable);
    myData->items.insert(this);
    ++countItems;
    TREE_LOG("Create item: " << countItems << ", " << object()->getObject()->getFullName());
}

DocumentObjectItem::~DocumentObjectItem()
{
    --countItems;
    TREE_LOG("Delete item: " << countItems << ", " << object()->getObject()->getFullName());
    auto it = myData->items.find(this);
    if(it == myData->items.end())
        assert(0);
    else
        myData->items.erase(it);

    if(myData->rootItem == this)
        myData->rootItem = 0;

    if(myOwner && myData->items.empty()) {
        auto it = myOwner->_ParentMap.find(object()->getObject());
        if(it!=myOwner->_ParentMap.end() && it->second.size())
            myOwner->populateObject(*it->second.begin());
    }
}

void DocumentObjectItem::restoreBackground() {
    this->setBackground(0,this->bgBrush);
}

void DocumentObjectItem::setHighlight(bool set, Gui::HighlightMode high) {
    QFont f = this->font(0);
    auto highlight = [=](const QColor& col){
        if (set)
            this->setBackground(0, col);
        else
            this->setBackground(0, QBrush());
        this->bgBrush = this->background(0);
    };

    switch (high) {
    case Gui::Bold:
        f.setBold(set);
        break;
    case Gui::Italic:
        f.setItalic(set);
        break;
    case Gui::Underlined:
        f.setUnderline(set);
        break;
    case Gui::Overlined:
        f.setOverline(set);
        break;
    case Gui::Blue:
        highlight(QColor(200,200,255));
        break;
    case Gui::LightBlue:
        highlight(QColor(230,230,255));
        break;
    case Gui::UserDefined:
    {
        QColor color(230,230,255);
        if (set) {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
            bool bold = hGrp->GetBool("TreeActiveBold",true);
            bool italic = hGrp->GetBool("TreeActiveItalic",false);
            bool underlined = hGrp->GetBool("TreeActiveUnderlined",false);
            bool overlined = hGrp->GetBool("TreeActiveOverlined",false);
            f.setBold(bold);
            f.setItalic(italic);
            f.setUnderline(underlined);
            f.setOverline(overlined);

            unsigned long col = hGrp->GetUnsigned("TreeActiveColor",3873898495);
            color = QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff);
        }
        else {
            f.setBold(false);
            f.setItalic(false);
            f.setUnderline(false);
            f.setOverline(false);
        }
        highlight(color);
    }   break;
    default:
        break;
    }
}

const char *DocumentObjectItem::getTreeName() const {
    return myData->getTreeName();
}

Gui::ViewProviderDocumentObject* DocumentObjectItem::object() const
{
    return myData->viewObject;
}

void DocumentObjectItem::testStatus(bool resetStatus) {
    QIcon icon,icon2;
    testStatus(resetStatus,icon,icon2);
}

void DocumentObjectItem::testStatus(bool resetStatus,QIcon &icon1, QIcon &icon2)
{
    App::DocumentObject* pObject = object()->getObject();

    int visible = -1;
    auto parentItem = getParentItem();
    if(parentItem)
        visible = parentItem->object()->getObject()->isElementVisible(
                pObject->getNameInDocument());
    if(visible<0)
        visible = object()->isShow()?1:0;

    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    bool external = object()->getDocument()!=getOwnerDocument()->document() ||
            (linked && linked->getDocument()!=obj->getDocument());

    int currentStatus =
        ((external?0:1)<<4) |
        ((object()->showInTree() ? 0 : 1) << 3) |
        ((pObject->isError()          ? 1 : 0) << 2) |
        ((pObject->isTouched()||pObject->mustExecute()== 1 ? 1 : 0) << 1) |
        (visible         ? 1 : 0);

    if (!resetStatus && previousStatus==currentStatus)
        return;

    previousStatus = currentStatus;

    QIcon::Mode mode = QIcon::Normal;
    if (currentStatus & 1) { // visible
        // Note: By default the foreground, i.e. text color is invalid
        // to make use of the default color of the tree widget's palette.
        // If we temporarily set this color to dark and reset to an invalid
        // color again we cannot do it with setTextColor() or setForeground(),
        // respectively, because for any reason the color would always switch
        // to black which will lead to unreadable text if the system background
        // hss already a dark color.
        // However, it works if we set the appropriate role to an empty QVariant().
#if QT_VERSION >= 0x040200
        this->setData(0, Qt::ForegroundRole,QVariant());
#else
        this->setData(0, Qt::TextColorRole,QVariant());
#endif
    }
    else { // invisible
        QStyleOptionViewItem opt;
        // it can happen that a tree item is not attached to the tree widget (#0003025)
        if (this->treeWidget())
            opt.initFrom(this->treeWidget());
#if QT_VERSION >= 0x040200
        this->setForeground(0, opt.palette.color(QPalette::Disabled,QPalette::Text));
#else
        this->setTextColor(0, opt.palette.color(QPalette::Disabled,QPalette::Text);
#endif
        mode = QIcon::Disabled;
    }

    QIcon &icon = mode==QIcon::Normal?icon1:icon2;

    if(icon.isNull()) {
        QPixmap px;
        if (currentStatus & 4) {
            static QPixmap pxError;
            if(pxError.isNull()) {
            // object is in error state
                const char * const feature_error_xpm[]={
                    "9 9 3 1",
                    ". c None",
                    "# c #ff0000",
                    "a c #ffffff",
                    "...###...",
                    ".##aaa##.",
                    ".##aaa##.",
                    "###aaa###",
                    "###aaa###",
                    "#########",
                    ".##aaa##.",
                    ".##aaa##.",
                    "...###..."};
                pxError = QPixmap(feature_error_xpm);
            }
            px = pxError;
        }
        else if (currentStatus & 2) {
            static QPixmap pxRecompute;
            if(pxRecompute.isNull()) {
                // object must be recomputed
                const char * const feature_recompute_xpm[]={
                    "9 9 3 1",
                    ". c None",
                    "# c #0000ff",
                    "a c #ffffff",
                    "...###...",
                    ".######aa",
                    ".#####aa.",
                    "#####aa##",
                    "#aa#aa###",
                    "#aaaa####",
                    ".#aa####.",
                    ".#######.",
                    "...###..."};
                pxRecompute = QPixmap(feature_recompute_xpm);
            }
            px = pxRecompute;
        }

        // get the original icon set
        QIcon icon_org = object()->getIcon();

        // Icon size from PM_ListViewIconSize is too big, and the TreeView will
        // automatically scale down the icon to fit, which in turn causes the
        // overlay status icon being scaled down too much. Use Qt standard icon
        // size instead
        //
        // int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
        static int w = -1;
        if(w < 0) w = QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon).width();

        QPixmap pxOn,pxOff;

        // if needed show small pixmap inside
        if (!px.isNull()) {
            pxOff = BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::Off),
                px,BitmapFactoryInst::TopRight);
            pxOn = BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::On ),
                px,BitmapFactoryInst::TopRight);
        } else {
            pxOff = icon_org.pixmap(w, w, mode, QIcon::Off);
            pxOn = icon_org.pixmap(w, w, mode, QIcon::On);
        }

        if(currentStatus & 8)  {// hidden item
            static QPixmap pxHidden;
            if(pxHidden.isNull()) {
                const char * const feature_hidden_xpm[]={
                    "9 7 3 1",
                    ". c None",
                    "# c #000000",
                    "a c #ffffff",
                    "...###...",
                    "..#aaa#..",
                    ".#a###a#.",
                    "#aa###aa#",
                    ".#a###a#.",
                    "..#aaa#..",
                    "...###..."};
                pxHidden = QPixmap(feature_hidden_xpm);
            }
            pxOff = BitmapFactory().merge(pxOff, pxHidden, BitmapFactoryInst::TopLeft);
            pxOn = BitmapFactory().merge(pxOn, pxHidden, BitmapFactoryInst::TopLeft);
        }

        if(external) {// external item
            static QPixmap pxExternal;
            if(pxExternal.isNull()) {
                const char * const feature_external_xpm[]={
                    "7 7 3 1",
                    ". c None",
                    "# c #000000",
                    "a c #ffffff",
                    "..###..",
                    ".#aa##.",
                    "..#aa##",
                    "..##aa#",
                    "..#aa##",
                    ".#aa##.",
                    "..###.."};
                pxExternal = QPixmap(feature_external_xpm);
            }
            pxOff = BitmapFactory().merge(pxOff, pxExternal, BitmapFactoryInst::BottomRight);
            pxOn = BitmapFactory().merge(pxOn, pxExternal, BitmapFactoryInst::BottomRight);
        }

        icon.addPixmap(pxOn, QIcon::Normal, QIcon::On);
        icon.addPixmap(pxOff, QIcon::Normal, QIcon::Off);
    }

    this->setIcon(0, icon);
}

void DocumentObjectItem::displayStatusInfo()
{
    App::DocumentObject* Obj = object()->getObject();

    QString info = QString::fromLatin1(Obj->getStatusString());
    if ( Obj->mustExecute() == 1 && !Obj->isError())
        info += QString::fromLatin1(" (but must be executed)");
    QString status = TreeWidget::tr("%1, Internal name: %2")
            .arg(info)
            .arg(QString::fromLatin1(Obj->getNameInDocument()));
    getMainWindow()->showMessage(status);

    if (Obj->isError()) {
        QTreeWidget* tree = this->treeWidget();
        QPoint pos = tree->visualItemRect(this).topRight();
        QToolTip::showText(tree->mapToGlobal(pos), info);
    }
}

void DocumentObjectItem::setExpandedStatus(bool on)
{
    if(getOwnerDocument()->document() == object()->getDocument())
        object()->getObject()->setStatus(App::Expand, on);
}

void DocumentObjectItem::setData (int column, int role, const QVariant & value)
{
    QVariant myValue(value);
    if (role == Qt::EditRole && column<=1) {
        auto transacting = App::GetApplication().getActiveTransaction();
        auto obj = object()->getObject();
        auto &label = column?obj->Label2:obj->Label;
        if(!transacting) {
            std::ostringstream ss;
            ss << "Change " << getName() << '.' << label.getName();
            App::GetApplication().setActiveTransaction(ss.str().c_str());
        }
        label.setValue((const char *)value.toString().toUtf8());
        if(!transacting)
            App::GetApplication().closeActiveTransaction();
        myValue = QString::fromUtf8(label.getValue());
    }
    QTreeWidgetItem::setData(column, role, myValue);
}

bool DocumentObjectItem::isChildOfItem(DocumentObjectItem* item)
{
    for(auto pitem=parent();pitem;pitem=pitem->parent())
        if(pitem == item)
            return true;
    return false;
}

bool DocumentObjectItem::requiredAtRoot(bool excludeSelf) const{
    if(myData->rootItem || object()->getDocument()!=getOwnerDocument()->document()) 
        return false;
    bool checkMap = true;
    for(auto item : myData->items) {
        if(excludeSelf && item == this) continue;
        auto pi = item->getParentItem();
        if(!pi || pi->myData->removeChildrenFromRoot)
            return false;
        checkMap = false;
    }
    if(checkMap && myOwner) {
        auto it = myOwner->_ParentMap.find(object()->getObject());
        if(it!=myOwner->_ParentMap.end()) {
            // Reaching here means all items of this corresponding object is
            // going to be deleted, but the object itself is not deleted and
            // still being refered to by some parent item that is not expanded
            // yet. So, we force populate at least one item of the parent
            // object to make sure that there is at least one corresponding
            // item for each object. 
            //
            // PS: practically speaking, it won't hurt much to delete all the
            // items, because the item will be auto created once the user
            // expand its parent item. It only causes minor problems, such as,
            // tree scroll to object command won't work properly.

            for(auto parent : it->second) {
                if(getOwnerDocument()->populateObject(parent))
                    return false;
            }
        }
    }
    return true;
}

bool DocumentObjectItem::isLink() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && obj!=linked;
}

bool DocumentObjectItem::isLinkFinal() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && linked == linked->getLinkedObject(true);
}


bool DocumentObjectItem::isParentLink() const {
    auto pi = getParentItem();
    return pi && pi->isLink();
}

enum GroupType {
    NotGroup = 0,
    LinkGroup = 1,
    PartGroup = 2,
    SuperGroup = 3, //reversed for future
};

int DocumentObjectItem::isGroup() const {
    auto obj = object()->getObject();
    if(obj->hasChildElement())
        return LinkGroup;
    auto linked = obj->getLinkedObject(true);
    if(!linked) return NotGroup;
    auto vp = Application::Instance->getViewProvider(linked);
    if(vp && vp->getChildRoot())
        return PartGroup;
    return NotGroup;
}

bool DocumentItem::isObjectShowable(App::DocumentObject *obj) {
    auto itParents = _ParentMap.find(obj);
    if(itParents == _ParentMap.end() || itParents->second.empty())
        return true;
    for(auto parent : itParents->second) {  
        if(!parent->hasChildElement() &&
            parent->getLinkedObject(false)==parent)
            return true;
    }
    return false;
}

int DocumentObjectItem::isParentGroup() const {
    auto pi = getParentItem();
    return pi?pi->isGroup():0;
}

DocumentObjectItem *DocumentObjectItem::getParentItem() const{
    if(parent()->type()!=TreeWidget::ObjectType)
        return 0;
    return static_cast<DocumentObjectItem*>(parent());
}

const char *DocumentObjectItem::getName() const {
    const char *name = object()->getObject()->getNameInDocument();
    return name?name:"";
}

int DocumentObjectItem::getSubName(std::ostringstream &str, App::DocumentObject *&topParent) const
{
    auto parent = getParentItem();
    if(!parent)
        return NotGroup;
    int ret = parent->getSubName(str,topParent);
    if(ret != SuperGroup) {
        int group = parent->isGroup();
        if(group == NotGroup) {
            if(ret!=PartGroup) {
                // Handle this situation,
                //
                // LinkGroup
                //    |--PartExtrude
                //           |--Sketch
                //
                // This function traverse from top down, so, when seeing a
                // non-group object 'PartExtrude', its following children should
                // not be grouped, so must reset any previous parents here.
                topParent = 0;
                str.str(""); //reset the current subname
                return NotGroup;
            }
            group = PartGroup;
        }
        ret = group;
    }

    auto obj = parent->object()->getObject();
    if(!obj || !obj->getNameInDocument()) {
        topParent = 0;
        str.str("");
        return NotGroup;
    }
    if(!topParent)
        topParent = obj;
    else if(!obj->redirectSubName(str,topParent,object()->getObject()))
        str << obj->getNameInDocument() << '.';
    return ret;
}

App::DocumentObject *DocumentObjectItem::getFullSubName(
        std::ostringstream &str, DocumentObjectItem *parent) const 
{
    auto pi = getParentItem();
    if(this==parent || !pi || (!parent && !pi->isGroup()))
        return object()->getObject();

    auto ret = pi->getFullSubName(str,parent);
    str << getName() << '.';
    return ret;
}

App::DocumentObject *DocumentObjectItem::getRelativeParent(
        std::ostringstream &str, DocumentObjectItem *cousin,
        App::DocumentObject **topParent, std::string *topSubname) const
{
    std::ostringstream str2;
    App::DocumentObject *top=0,*top2=0;
    getSubName(str,top);
    if(topParent)
        *topParent = top;
    if(!top)
        return 0;
    if(topSubname)
        *topSubname = str.str() + getName() + '.';
    cousin->getSubName(str2,top2);
    if(top!=top2) {
        str << getName() << '.';
        return top;
    }

    auto subname = str.str();
    auto subname2 = str2.str();
    const char *sub = subname.c_str();
    const char *sub2 = subname2.c_str();
    while(1) {
        const char *dot = strchr(sub,'.');
        if(!dot) {
            str.str("");
            return 0;
        }
        const char *dot2 = strchr(sub2,'.');
        if(!dot2 || dot-sub!=dot2-sub2 || strncmp(sub,sub2,dot-sub)!=0) {
            auto substr = subname.substr(0,dot-subname.c_str()+1);
            auto ret = top->getSubObject(substr.c_str());
            if(!top) {
                FC_ERR("invalid subname " << top->getFullName() << '.' << substr);
                str.str("");
                return 0;
            }
            str.str("");
            str << dot+1 << getName() << '.';
            return ret;
        }
        sub = dot+1;
        sub2 = dot2+1;
    }
    str.str("");
    return 0;
}

DocumentItem *DocumentObjectItem::getParentDocument() const {
    return getTree()->getDocumentItem(object()->getDocument());
}

DocumentItem *DocumentObjectItem::getOwnerDocument() const {
    return myOwner;
}

TreeWidget *DocumentObjectItem::getTree() const{
    return static_cast<TreeWidget*>(treeWidget());
}

#include "moc_Tree.cpp"

