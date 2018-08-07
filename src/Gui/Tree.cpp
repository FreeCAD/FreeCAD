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
# include <boost/signals.hpp>
# include <boost/bind.hpp>
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <qcursor.h>
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
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;

/* TRANSLATOR Gui::TreeWidget */
TreeWidget::TreeWidget(const char *name, QWidget* parent)
    : QTreeWidget(parent), SelectionObserver(false,false), contextItem(0)
    , editingItem(0), currentDocItem(0),fromOutside(false)
    ,statusUpdateDelay(0),myName(name)
{
    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);
    this->setRootIsDecorated(false);

    GET_TREEVIEW_PARAM(hGrp);
    this->syncSelectionAction = new QAction(this);
    this->syncSelectionAction->setCheckable(true);
    this->syncSelectionAction->setChecked(hGrp->GetBool("SyncSelection",true));
    connect(this->syncSelectionAction, SIGNAL(triggered()),
            this, SLOT(onSyncSelection()));

    this->preSelectionAction = new QAction(this);
    this->preSelectionAction->setCheckable(true);
    this->preSelectionAction->setChecked(hGrp->GetBool("PreSelection",true));
    connect(this->preSelectionAction, SIGNAL(triggered()),
            this, SLOT(onPreSelection()));

    this->syncViewAction = new QAction(this);
    this->syncViewAction->setCheckable(true);
    this->syncViewAction->setChecked(hGrp->GetBool("SyncView",false));
    connect(this->syncViewAction, SIGNAL(triggered()),
            this, SLOT(onSyncView()));

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

    this->markRecomputeAction = new QAction(this);
    connect(this->markRecomputeAction, SIGNAL(triggered()),
            this, SLOT(onMarkRecompute()));

    this->recomputeObjectAction = new QAction(this);
    connect(this->recomputeObjectAction, SIGNAL(triggered()),
            this, SLOT(onRecomputeObject()));

    // Setup connections
    Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, _1));
    Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, _1));
    Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, _1));
    Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, _1));
    Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, _1));
    Application::Instance->signalShowHidden.connect(boost::bind(&TreeWidget::slotShowHidden, this, _1));

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
}

const char *TreeWidget::getTreeName() const {
    return myName.c_str();
}

void TreeWidget::updateStatus(bool delay) {
    for(auto tree : getMainWindow()->findChildren<TreeWidget*>())
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
    Gui::Application::Instance->setupContextMenu("Tree", &view);

    QMenu contextMenu;

    QMenu subMenu;
    QMenu editMenu;
    QActionGroup subMenuGroup(&subMenu);
    subMenuGroup.setExclusive(true);
    connect(&subMenuGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(onActivateDocument(QAction*)));
    MenuManager::getInstance()->setupContextMenu(&view, contextMenu);

    QAction* topact = 0;
    auto actions = contextMenu.actions();
    if(actions.size()) {
        contextMenu.addSeparator();
        topact = actions.front();
    }
    QMenu optionsMenu;
    optionsMenu.setTitle(tr("Tree view options"));
    optionsMenu.addAction(this->preSelectionAction);
    optionsMenu.addAction(this->syncSelectionAction);
    optionsMenu.addAction(this->syncViewAction);
    contextMenu.insertMenu(topact,&optionsMenu);
    contextMenu.insertSeparator(topact);

    // get the current item
    this->contextItem = itemAt(e->pos());

    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        showHiddenAction->setChecked(docitem->showHidden());
        contextMenu.addAction(this->showHiddenAction);
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

    if (contextMenu.actions().count() > 0)
        contextMenu.exec(QCursor::pos());
}

void TreeWidget::hideEvent(QHideEvent *ev) {
    TREE_TRACE("detaching selection observer");
    this->detachSelection();
    QTreeWidget::hideEvent(ev);
}

void TreeWidget::showEvent(QShowEvent *ev) {
    TREE_TRACE("attaching selection observer");
    this->attachSelection();
    this->syncSelection();
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

void TreeWidget::onMarkRecompute()
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        std::vector<App::DocumentObject*> obj = doc->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
            (*it)->touch();
    }
    // mark all selected objects
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                App::DocumentObject* obj = objitem->object()->getObject();
                obj->touch();
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
            objs.back()->touch();
        }
    }
    if(objs.empty())
        return;
    App::GetApplication().setActiveTransaction("Recompute");
    objs.front()->getDocument()->recompute(objs,true);
    App::GetApplication().closeActiveTransaction();
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
        for(auto pTree : getMainWindow()->findChildren<TreeWidget*>())
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
                FC_WARN("skip '" << obj->getNameInDocument() << "' with invalid parent");
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

void TreeWidget::onActivateDocument(QAction* active)
{
    // activate the specified document
    QByteArray docname = active->data().toByteArray();
    Gui::Document* doc = Application::Instance->getDocument((const char*)docname);
    if (!doc) return;
    MDIView *view = doc->getActiveView();
    if (!view) return;
    getMainWindow()->setActiveWindow(view);
}

Qt::DropActions TreeWidget::supportedDropActions () const
{
    return QTreeWidget::supportedDropActions();
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
        MDIView *view = doc->getActiveView();
        if (!view) return;
        getMainWindow()->setActiveWindow(view);
    }
    else if (item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(item);
        auto view = objitem->getOwnerDocument()->document()->getActiveView();
        if (view) getMainWindow()->setActiveWindow(view);
        if (!objitem->object()->doubleClicked())
            QTreeWidget::mouseDoubleClickEvent(event);
    }
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

    QTreeWidgetItem* targetitem = itemAt(event->pos());
    if (!targetitem || this->isItemSelected(targetitem)) {
        leaveEvent(0);
        event->ignore();
    }
    else if (targetitem->type() == TreeWidget::DocumentType) {
        leaveEvent(0);
        QList<QModelIndex> idxs = selectedIndexes();
        App::Document* doc = static_cast<DocumentItem*>(targetitem)->
            document()->getDocument();
        for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
            QTreeWidgetItem* item = itemFromIndex(*it);
            if (item->type() != TreeWidget::ObjectType) {
                event->ignore();
                return;
            }
            App::DocumentObject* obj = static_cast<DocumentObjectItem*>(item)->
                object()->getObject();
            if (doc != obj->getDocument()) {
                event->ignore();
                return;
            }
        }
    }
    else if (targetitem->type() == TreeWidget::ObjectType) {
        onItemEntered(targetitem);

        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetitem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();

        if (!vp->canDropObjects()) {
            TREE_TRACE("cannot drop");
            event->ignore();
            return;
        }

        QList<QModelIndex> idxs = selectedIndexes();
        for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
            QTreeWidgetItem* ti = itemFromIndex(*it);
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
            if (item->parent() == targetitem) {
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
                TREE_TRACE("cannot drop " << obj->getNameInDocument() << ' '
                        << (owner?owner->getNameInDocument():"<No Owner>") << '.' << subname);
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
    std::vector<std::string> subs;
};
struct ItemInfo2 {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
};

void TreeWidget::dropEvent(QDropEvent *event)
{
    //FIXME: This should actually be done inside dropMimeData

    QTreeWidgetItem* targetitem = itemAt(event->pos());
    // not dropped onto an item
    if (!targetitem)
        return;
    // one of the source items is also the destination item, that's not allowed
    if (this->isItemSelected(targetitem))
        return;

    // filter out the selected items we cannot handle
    std::vector<std::pair<DocumentObjectItem*,std::vector<std::string> > > items;
    QList<QModelIndex> idxs = selectedIndexes();
    items.reserve(idxs.size());
    for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
        // ignore child elements if the parent is selected
        QModelIndex parent = (*it).parent();
        if (idxs.contains(parent))
            continue;
        auto *item = dynamic_cast<DocumentObjectItem*>(itemFromIndex(*it));
        if (!item)
            continue;
        if (item == targetitem)
            continue;
        if (item->parent() == targetitem)
            continue;
        items.emplace_back();
        auto &info = items.back();
        info.first = item;
        info.second.insert(info.second.end(),item->mySubs.begin(),item->mySubs.end());
    }

    if (items.empty())
        return; // nothing needs to be done

    if (targetitem->type() == TreeWidget::ObjectType) {
        // add object to group
        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetitem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();
        if (!vp->canDropObjects()) {
            return; // no group like object
        }

        bool dropOnly = QApplication::keyboardModifiers()== Qt::ControlModifier;
        if(!dropOnly) {
            // check if items can be dragged
            for(auto &v : items) {
                auto item = v.first;
                auto parentItem = item->getParentItem();
                if(!parentItem || !vp->canDragAndDropObject(item->object()->getObject()))
                    continue;
                if(!parentItem->object()->canDragObjects() || 
                   !parentItem->object()->canDragObject(item->object()->getObject()))
                {
                    TREE_ERR("'" << item->object()->getObject()->getNameInDocument() << 
                           "' cannot be dragged out of '" << 
                           parentItem->object()->getObject()->getNameInDocument() << "'");
                    return;
                }
            }
        }

        std::ostringstream selSubname;
        App::DocumentObject *selObj = 0;
        targetItemObj->getSubName(selSubname,selObj);
        Selection().clearCompleteSelection();
        if(selObj) {
            selSubname << vp->getObject()->getNameInDocument() << '.';
            Selection().addSelection(selObj->getDocument()->getName(),
                    selObj->getNameInDocument(), selSubname.str().c_str());
        } else {
            selObj = targetItemObj->object()->getObject();
            Selection().addSelection(selObj->getDocument()->getName(),
                    selObj->getNameInDocument());
        }
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
            auto owner = item->getRelativeParent(str,targetItemObj);
            info.subname = str.str();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if(owner) {
                info.ownerDoc = owner->getDocument()->getName();
                info.owner = owner->getNameInDocument();
            }
            QTreeWidgetItem* parent = item->parent();
            if (parent && parent->type() == TreeWidget::ObjectType) {
                auto vpp = static_cast<DocumentObjectItem *>(parent)->object();
                info.parent = vpp->getObject()->getNameInDocument();
                info.parentDoc = vpp->getObject()->getDocument()->getName();
            }
            info.subs.swap(v.second);
        }

        // Open command
        Gui::Document* gui = vp->getDocument();
        gui->openCommand("Drag object");
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

                if(!dropOnly && vpp && vp->canDragAndDropObject(obj)) {
                    vpp->dragObject(obj);
                    owner = 0;
                    subname.clear();
                }
                vp->dropObjectEx(obj,owner,subname.c_str(),info.subs);
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
    else if (targetitem->type() == TreeWidget::DocumentType) {
        std::vector<ItemInfo2> infos;
        infos.reserve(items.size());

        // check if items can be dragged
        for(auto &v : items) {
            auto item = v.first;
            auto parentItem = item->getParentItem();
            if(!parentItem) 
                continue;
            if(!parentItem->object()->canDragObjects() || 
               !parentItem->object()->canDragObject(item->object()->getObject()))
            {
                TREE_ERR("'" << item->object()->getObject()->getNameInDocument() << 
                       "' cannot be dragged out of '" << 
                       parentItem->object()->getObject()->getNameInDocument() << "'");
                return;
            }
            infos.emplace_back();
            auto &info = infos.back();
            auto obj = item->object()->getObject();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            auto parent = parentItem->object()->getObject();
            info.parentDoc = parent->getDocument()->getName();
            info.parent = parent->getNameInDocument();
        }
        // Because the existence of subname, we must de-select the drag the
        // object manually. Just do a complete clear here for simplicity
        Selection().clearCompleteSelection();

        // Open command
        App::Document* doc = static_cast<DocumentItem*>(targetitem)->document()->getDocument();
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Move object");
        try {
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

                vpp->dragObject(obj);

                //make sure it is not part of a geofeaturegroup anymore. When this has happen we need to handle 
                //all removed objects
                auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
                if(grp) {
                    grp->getExtensionByType<App::GeoFeatureGroupExtension>()->removeObject(obj);
                    // children view provider maintainace is now handled by
                    // Gui::Document::handleChildren3D()
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

void TreeWidget::slotFinishRestoreDocument(const App::Document& Doc)
{
    if(!Doc.testStatus(App::Document::PartialDoc))
        return;
    auto doc = Application::Instance->getDocument(&Doc);
    if(!doc) return;

    auto it = DocumentMap.find(doc);
    if(it==DocumentMap.end())
        return;

    auto item = it->second;
    this->collapseItem(item);

    item->setIcon(0, *documentPartialPixmap);
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

void TreeWidget::slotDeleteDocument(const Gui::Document& Doc)
{
    std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        for(auto &v : DocumentMap)
            v.second->onDeleteDocument(it->second);
        this->rootItem->takeChild(this->rootItem->indexOfChild(it->second));
        delete it->second;
        DocumentMap.erase(it);
    }
}

void TreeWidget::slotRenameDocument(const Gui::Document& Doc)
{
    // do nothing here
    Q_UNUSED(Doc); 
}

void TreeWidget::slotChangedViewObject(const Gui::ViewProvider& vp, const App::Property &prop)
{
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
    for (std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.begin();
         it != DocumentMap.end(); ++it)
    {
        QFont f = it->second->font(0);
        f.setBold(it == jt);
        it->second->setFont(0,f);
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

        if(preSelectionAction->isChecked()) {
            if(preselectTime.elapsed() < 700)
                onPreSelectTimer();
            else{
                preselectTimer->start(500);
                Selection().rmvPreselect();
            }
        }
    } else if(preSelectionAction->isChecked())
        Selection().rmvPreselect();
}

void TreeWidget::leaveEvent(QEvent *) {
    if(preSelectionAction->isChecked()) {
        preselectTimer->stop();
        Selection().rmvPreselect();
    }
}

void TreeWidget::onPreSelectTimer() {
    if(!preSelectionAction->isChecked())
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

void TreeWidget::scrollItemToTop(Gui::Document* doc)
{
    if(!isConnectionAttached()) 
        return;

    std::map<const Gui::Document*,DocumentItem*>::iterator it;
    it = DocumentMap.find(doc);
    if (it != DocumentMap.end()) {
        bool lock = this->blockConnection(true);
        it->second->selectItems(true);
        this->blockConnection(lock);
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
    this->rootItem->setText(0, tr("Application"));

    this->preSelectionAction->setText(tr("Pre-selection"));
    this->preSelectionAction->setStatusTip(tr("Preselect the object in 3D view when mouse over the tree item"));

    this->syncSelectionAction->setText(tr("Sync selection"));
    this->syncSelectionAction->setStatusTip(tr("Auto expand item when selected in 3D view"));

    this->syncViewAction->setText(tr("Sync view"));
    this->syncViewAction->setStatusTip(tr("Auto switch to the 3D view containing the selected item"));

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

    this->markRecomputeAction->setText(tr("Mark to recompute"));
    this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));

    this->recomputeObjectAction->setText(tr("Recompute object"));
    this->recomputeObjectAction->setStatusTip(tr("Recompute the selected object"));
}

void TreeWidget::onSyncSelection() {
    GET_TREEVIEW_PARAM(hGrp);
    hGrp->SetBool("SyncSelection",syncSelectionAction->isChecked());
}

void TreeWidget::onPreSelection() {
    GET_TREEVIEW_PARAM(hGrp);
    hGrp->SetBool("PreSelection",preSelectionAction->isChecked());
}


void TreeWidget::onSyncView() {
    GET_TREEVIEW_PARAM(hGrp);
    hGrp->SetBool("SyncView",syncViewAction->isChecked());
}

void TreeWidget::syncView() {
    if(currentDocItem && syncViewAction->isChecked()) {
        MDIView *view = currentDocItem->document()->getActiveView();
        if (view) getMainWindow()->setActiveWindow(view);
    }
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
    }else{
        std::map<const Gui::Document*,DocumentItem*>::iterator pos;
        for (pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
            currentDocItem = pos->second;
            pos->second->updateSelection(pos->second);
            currentDocItem = 0;
        }
    }
    this->blockConnection(lock);
}

void TreeWidget::syncSelection(const char *pDocName) {
    if(this->isConnectionBlocked()) {
        TREE_TRACE("connection blocked");
        return;
    }
    if (!pDocName || *pDocName==0 || strcmp(pDocName,"*")==0) {
        if(Selection().hasSelection()) {
            for(auto &v : DocumentMap) {
                bool lock = this->blockConnection(true);
                currentDocItem = v.second;
                v.second->selectItems(syncSelectionAction->isChecked());
                currentDocItem = 0;
                this->blockConnection(lock);
            }
        }else{
            for(auto &v : DocumentMap)
                v.second->clearSelection();
        }
        return;
    }
    Gui::Document* pDoc = Application::Instance->getDocument(pDocName);
    std::map<const Gui::Document*, DocumentItem*>::iterator it;
    it = DocumentMap.find(pDoc);
    if (it != DocumentMap.end()) {
        if(Selection().hasSelection()) {
            bool lock = this->blockConnection(true);
            currentDocItem = it->second;
            it->second->selectItems(syncSelectionAction->isChecked());
            currentDocItem = 0;
            this->blockConnection(lock);
        }else
            it->second->clearSelection();
    }
}

void TreeWidget::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
    case SelectionChanges::RmvSelection:
    case SelectionChanges::SetSelection:
    case SelectionChanges::ClrSelection:
        syncSelection(msg.pDocName);
        break;
    default:
        break;
    }
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

// ---------------------------------------------------------------------------

typedef std::set<DocumentObjectItem*> DocumentObjectItems;

static std::map<App::DocumentObject*, std::set<App::DocumentObject*> > _ParentMap;

class Gui::DocumentObjectData {
public:
    const char *treeName;
    DocumentObjectItems items;
    ViewProviderDocumentObject *viewObject;
    DocumentObjectItem *rootItem;
    std::vector<App::DocumentObject*> children;
    std::set<App::DocumentObject*> childSet;
    bool removeChildrenFromRoot;
    std::string label;

    typedef boost::BOOST_SIGNALS_NAMESPACE::scoped_connection Connection;

    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    DocumentObjectData(TreeWidget *tree, ViewProviderDocumentObject* vpd)
        : viewObject(vpd),rootItem(0)
    {
        treeName = tree->getTreeName();
        // Setup connections
        connectIcon = viewObject->signalChangeIcon.connect(
                boost::bind(&DocumentObjectData::slotChangeIcon, this));
        connectTool = viewObject->signalChangeToolTip.connect(
                boost::bind(&DocumentObjectData::slotChangeToolTip, this, _1));
        connectStat = viewObject->signalChangeStatusTip.connect(
                boost::bind(&DocumentObjectData::slotChangeStatusTip, this, _1));

        removeChildrenFromRoot = viewObject->canRemoveChildrenFromRoot();
        label = viewObject->getObject()->Label.getValue();
        updateChildren();
    }

    const char *getTreeName() const {
        return treeName;
    }

    void updateChildren(DocumentObjectDataPtr other) {
        children = other->children;
        childSet = other->childSet;
    }

    bool updateChildren() {
        auto newChildren = viewObject->claimChildren();
        auto obj = viewObject->getObject();
        std::set<App::DocumentObject *> newSet;
        bool updated = false;
        for (auto child : newChildren) {
            if(child && child->getNameInDocument()) {
                if(!newSet.insert(child).second) {
                    TREE_WARN("duplicate child item " << obj->getNameInDocument() 
                        << '.' << child->getNameInDocument());
                }else if(!childSet.erase(child)) {
                    // this means new child detected
                    updated = true;
                    if(child->getDocument()==obj->getDocument())
                        _ParentMap[child].insert(obj);
                }
            }
        }
        for (auto child : childSet) {
            if(newSet.find(child) == newSet.end()) {
                // this means old child removed
                updated = true;
                _ParentMap[child].erase(obj);
            }
        }
        // We still need to check the order of the children
        updated = updated || children!=newChildren;
        children.swap(newChildren);
        childSet.swap(newSet);
        return updated;
    }

    void clearChildren() {
        auto obj = viewObject->getObject();
        for (auto child : children) {
            if(child && child->getDocument() == obj->getDocument())
                _ParentMap[child].erase(obj);
        }
        _ParentMap.erase(obj);
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

    if(linkedDoc->document()->getDocument() != App::GetApplication().getActiveDocument()) {
        MDIView *view = linkedDoc->pDocument->getActiveView();
        if (view) getMainWindow()->setActiveWindow(view);
    }
}

// ----------------------------------------------------------------------------

DocumentItem::DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent)
    : QTreeWidgetItem(parent, TreeWidget::DocumentType), pDocument(doc)
{
    // Setup connections
    connectNewObject = doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, _1));
    connectDelObject = doc->signalDeletedObject.connect(boost::bind(&DocumentItem::slotDeleteObject, this, _1, true));
    connectChgObject = doc->signalChangedObject.connect(boost::bind(&DocumentItem::slotChangeObject, this, _1, _2));
    connectRenObject = doc->signalRelabelObject.connect(boost::bind(&DocumentItem::slotRenameObject, this, _1));
    connectActObject = doc->signalActivatedObject.connect(boost::bind(&DocumentItem::slotActiveObject, this, _1));
    connectEdtObject = doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, _1));
    connectResObject = doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, _1));
    connectHltObject = doc->signalHighlightObject.connect(boost::bind(&DocumentItem::slotHighlightObject, this, _1,_2,_3));
    connectExpObject = doc->signalExpandObject.connect(boost::bind(&DocumentItem::slotExpandObject, this, _1,_2));
    connectScrObject = doc->signalScrollToObject.connect(boost::bind(&DocumentItem::slotScrollToObject, this, _1));
    connectRecomputed = doc->getDocument()->signalRecomputed.connect(boost::bind(&DocumentItem::slotRecomputed, this, _1, _2));

    setFlags(Qt::ItemIsEnabled/*|Qt::ItemIsEditable*/);

    treeName = getTree()->getTreeName();
}

DocumentItem::~DocumentItem()
{
    connectNewObject.disconnect();
    connectDelObject.disconnect();
    connectChgObject.disconnect();
    connectRenObject.disconnect();
    connectActObject.disconnect();
    connectEdtObject.disconnect();
    connectResObject.disconnect();
    connectHltObject.disconnect();
    connectExpObject.disconnect();
    connectScrObject.disconnect();
    connectRecomputed.disconnect();
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
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    unsigned long col = hGrp->GetUnsigned("TreeEditColor",4294902015);
    QColor color((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff);
    if(getTree()->editingItem)
        getTree()->editingItem->setBackgroundColor(0,color);
    else{
        FOREACH_ITEM(item,v)
            item->setBackgroundColor(0,color);
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
    createNewItem(obj);
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
                    getTree(), const_cast<ViewProviderDocumentObject*>(&obj));
        }else if(pdata->rootItem && parent==NULL) {
            Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
            return false;
        }
        data = pdata;
    }

    std::string displayName = obj.getObject()->Label.getValue();
    std::string objectName = obj.getObject()->getNameInDocument();
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
    item->setText(0, QString::fromUtf8(displayName.c_str()));
    item->setHidden(!obj.showInTree() && !showHidden());
    populateItem(item);

    // Not calling item testStatus below because there seems to have some delay
    // between new object, and its visual status update. Need to figure out why
    // item->testStatus(true);
    getTree()->updateStatus(true);
    return true;
}

void DocumentItem::onDeleteDocument(DocumentItem *docItem) {
    if(docItem == this) {
        FOREACH_ITEM_ALL(item);
            item->myOwner = 0;
        END_FOREACH_ITEM;
        return;
    }
    for(auto it=ObjectMap.begin(),itNext=it;it!=ObjectMap.end();it=itNext) {
        ++itNext;
        auto data = it->second;
        if(data->viewObject->getDocument() == docItem->document())
            slotDeleteObject(*data->viewObject,false);
    }
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

void DocumentItem::slotDeleteObject(const Gui::ViewProviderDocumentObject& view, bool broadcast)
{
    if(broadcast) {
        for(auto &v : getTree()->DocumentMap)
            v.second->slotDeleteObject(view,false);
        return;
    }

    auto it = ObjectMap.find(view.getObject());
    if(it == ObjectMap.end())
        return;

    TREE_LOG("delete object " << view.getObject()->getNameInDocument());

    it->second->clearChildren();

    auto &items = it->second->items;
    for(auto cit=items.begin(),citNext=cit;cit!=items.end();cit=citNext) {
        ++citNext;
        (*cit)->myOwner = 0;
        delete *cit;
    }
    
    // Check for any child of the deleted object that is not in the tree, and put it
    // under document item.
    for(auto child : it->second->children) {
        if(!child || !child->getNameInDocument() || 
           child->getDocument()!=document()->getDocument())
            continue;
        auto cit = ObjectMap.find(child);
        if(cit==ObjectMap.end() || cit->second->items.empty()) {
            auto vpd = getViewProvider(child);
            if(!vpd) continue;
            createNewItem(*vpd);
        }else {
            auto childItem = *cit->second->items.begin();
            if(childItem->requiredAtRoot(false))
                createNewItem(*childItem->object(),this,-1,childItem->myData);
        }
    }

    if(items.empty())
        ObjectMap.erase(it);
}

void DocumentItem::populateObject(App::DocumentObject *obj) {
    // make sure at least one of the item corresponding to obj is populated
    auto it = ObjectMap.find(obj);
    if(it == ObjectMap.end())
        return;
    auto &items = it->second->items;
    if(items.empty())
        return;
    for(auto item : items) {
        if(item->populated)
            return;
    }
    TREE_LOG("force populate object " << obj->getNameInDocument());
    auto item = *items.begin();
    item->populated = true;
    populateItem(item,true);
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
                item->removeChild(ci);
                item->insertChild(i,ci);
                assert(ci->parent()==item);
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
                    << item->object()->getObject()->getNameInDocument()
                    << '.' << childItem->object()->getObject()->getNameInDocument());
                --i;
                continue;
            }
            it->second->rootItem = 0;
            this->removeChild(childItem);
            item->insertChild(i,childItem);
            assert(childItem->parent()==item);
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

void DocumentItem::slotChangeObject(const Gui::ViewProviderDocumentObject& view, const App::Property &prop) {
    auto obj = view.getObject();
    if(!obj || !obj->getNameInDocument())
        return;

    getTree()->_updateStatus(true);

    // Let's not waste time on the newly added Visibility property in
    // DocumentObject.
    if(&prop == &obj->Visibility)
        return;

    if(&prop == &obj->Label) {
        const char *label = obj->Label.getValue();
        for(auto &v : getTree()->DocumentMap) {
            auto it = v.second->ObjectMap.find(obj);
            if(it == v.second->ObjectMap.end())
                continue;
            if(it->second->label!=label) {
                it->second->label = label;
                auto displayName = QString::fromUtf8(label);
                for(auto item : it->second->items)
                    item->setText(0, displayName);
            }
        }
        return;
    }

    bool childrenChanged = false;
    std::vector<App::DocumentObject*> children;
    bool removeChildrenFromRoot = view.canRemoveChildrenFromRoot();
    DocumentObjectDataPtr found;
    for(auto &v : getTree()->DocumentMap) {
        auto docItem = v.second;
        auto it = docItem->ObjectMap.find(obj);
        if(it == docItem->ObjectMap.end())
            continue;
        if(!found) {
            found = it->second;
            childrenChanged = found->updateChildren();
            if(!childrenChanged && it->second->removeChildrenFromRoot==removeChildrenFromRoot)
                return;
        }
        it->second->removeChildrenFromRoot = removeChildrenFromRoot;
        if(childrenChanged)
            it->second->updateChildren(found);
        for(auto item : it->second->items)
            docItem->populateItem(item,true);
    }

    if(childrenChanged && prop.testStatus(App::Property::Output)) {
        // When a property is marked as output, it will not touch its object,
        // and thus, its property change will not be propagated through
        // recomputation. So we have to manually check for each links here.
        for(auto link : App::GetApplication().getLinksTo(obj,true)) {
            std::vector<App::DocumentObject*> linkedChildren;
            DocumentObjectDataPtr found;
            for(auto &v : getTree()->DocumentMap) {
                auto docItem = v.second;
                auto it = docItem->ObjectMap.find(link);
                if(it == docItem->ObjectMap.end())
                    continue;
                if(!found) {
                    found = it->second;
                    if(!found->updateChildren())
                        break;
                }
                it->second->updateChildren(found);
                for(auto item : it->second->items)
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
                slotChangeObject(*vp,dummy);
            }
        }
    }
}
    
void DocumentItem::slotRenameObject(const Gui::ViewProviderDocumentObject& obj)
{
    // Do nothing here because the Label is set in slotChangeObject
    Q_UNUSED(obj); 
}

void DocumentItem::slotActiveObject(const Gui::ViewProviderDocumentObject& obj)
{
    if(ObjectMap.find(obj.getObject()) == ObjectMap.end())
        return; // signal is emitted before the item gets created
    FOREACH_ITEM_ALL(item);
        QFont f = item->font(0);
        f.setBold(item->object() == &obj);
        item->setFont(0,f);
    END_FOREACH_ITEM
}

void DocumentItem::slotHighlightObject (const Gui::ViewProviderDocumentObject& obj, const Gui::HighlightMode& high, bool set)
{
    FOREACH_ITEM(item,obj)
        QFont f = item->font(0);
        switch (high) {
        case Gui::Bold: f.setBold(set);             break;
        case Gui::Italic: f.setItalic(set);         break;
        case Gui::Underlined: f.setUnderline(set);  break;
        case Gui::Overlined: f.setOverline(set);    break;
        case Gui::Blue:
            if (set)
                item->setBackgroundColor(0,QColor(200,200,255));
            else
                item->setData(0, Qt::BackgroundColorRole,QVariant());
            break;
        case Gui::LightBlue:
            if (set) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
                unsigned long col = hGrp->GetUnsigned("TreeActiveColor",3873898495);
                item->setBackgroundColor(0,QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff));
            }
            else
                item->setData(0, Qt::BackgroundColorRole,QVariant());
            break;
        default:
            break;
        }

        item->setFont(0,f);
    END_FOREACH_ITEM
}

void DocumentItem::slotExpandObject (const Gui::ViewProviderDocumentObject& obj,const Gui::TreeItemMode& mode)
{
    FOREACH_ITEM(item,obj)
        // All document object items must always have a parent, either another
        // object item or document item. If not, then there is a bug somewhere
        // else.
        assert(item->parent());
        if (!item->parent()->isExpanded()) continue;
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
        if(obj->isValid()) continue;
        auto it = ObjectMap.find(obj);
        if(it == ObjectMap.end() || it->second->items.empty()) 
            continue;
        auto item = it->second->rootItem;
        if(!item) {
            item = *it->second->items.begin();
            showItem(item,false);
        }
        if(!scrolled) {
            scrolled = true;
            tree->scrollToItem(item);
        }
    }
}

const Gui::Document* DocumentItem::document() const
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
        getTree()->syncView();
}

void DocumentItem::findSelection(bool sync, DocumentObjectItem *item, const char *subname) 
{
    if(item->isHidden())
        return;

    if(!subname || *subname==0) {
        item->selected+=2;
        item->mySubs.clear();
        return;
    }

    TREE_TRACE("find next " << subname);

    // try to find the next level object name
    const char *nextsub = 0;
    const char *dot = 0;
    if((dot=strchr(subname,'.'))) 
        nextsub = dot+1;
    else {
        item->selected+=2;
        if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
            item->mySubs.push_back(subname);
        return;
    }

    std::string name(subname,nextsub-subname);
    auto obj = item->object()->getObject();
    auto subObj = obj->getSubObject(name.c_str());
    if(!subObj || subObj==obj) {
        if(!subObj)
            TREE_WARN("sub object not found " << item->getName() << '.' << name.c_str());
        item->selected += 2;
        if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
            item->mySubs.push_back(subname);
        return;
    }

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

        if(child->object()->getObject() == subObj) {
            findSelection(sync,child,nextsub);
            return;
        }
    }

    // The sub object is not found. This could happen for geo group, since its
    // children may be in more than one hierarchy down.
    bool found = false;
    auto it = ObjectMap.find(subObj);
    if(it != ObjectMap.end()) {
        for(auto child : it->second->items) {
            if(child->isChildOfItem(item)) {
                found = true;
                findSelection(sync,child,nextsub);
            }
        }
    }

    if(!found) {
        // The sub object is still not found. Maybe it is a non-object sub-element.
        // Select the current object instead.
        TREE_TRACE("element " << subname << " not found");
        item->selected+=2;
        if(std::find(item->mySubs.begin(),item->mySubs.end(),subname)==item->mySubs.end())
            item->mySubs.push_back(subname);
    }
}

void DocumentItem::selectItems(bool sync) {
    const auto &sels = Selection().getSelection(pDocument->getDocument()->getName(),false);
    for(const auto &sel : sels) {
        auto it = ObjectMap.find(sel.pObject);
        if(it == ObjectMap.end()) continue;
        TREE_TRACE("find select " << sel.FeatName);
        for(auto item : it->second->items) {
            // If the parent is a group, then we have full quanlified
            // selection, which means this item can never be selected directly
            // in the 3D view,  only as element of the parent object
            if(item->isParentGroup())
                continue;

            findSelection(sync,item,sel.SubName);
        }
    }

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
            getTree()->syncView();
        }
    }
}

void DocumentItem::populateParents(const ViewProvider *vp, ParentMap &parentMap) {
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
    ParentMap parentMap;
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

bool DocumentItem::showItem(DocumentObjectItem *item, bool select) {
    auto parent = item->parent();
    if(item->isHidden() ||
       (parent->type()==TreeWidget::ObjectType && 
        !showItem(static_cast<DocumentObjectItem*>(parent),false)))
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
    for(int i=0;i<item->childCount();++i) {
        auto child = item->child(i);
        if(child->type()!=TreeWidget::ObjectType) continue;
        auto childItem = static_cast<DocumentObjectItem*>(child);
        childItem->setHidden(!show && !childItem->object()->showInTree());
        updateItemsVisibility(childItem,show);
    }
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
    TREE_LOG("Create item: " << countItems << ", " << object()->getObject()->getNameInDocument());
}

DocumentObjectItem::~DocumentObjectItem()
{
    --countItems;
    TREE_LOG("Delete item: " << countItems << ", " << object()->getObject()->getNameInDocument());
    auto it = myData->items.find(this);
    if(it == myData->items.end())
        assert(0);
    else
        myData->items.erase(it);

    if(myData->rootItem == this)
        myData->rootItem = 0;

    if(myOwner && myData->items.empty()) {
        auto it = _ParentMap.find(object()->getObject());
        if(it!=_ParentMap.end() && it->second.size())
            myOwner->populateObject(*it->second.begin());
    }
}

const char *DocumentObjectItem::getTreeName() const {
    return myData->treeName;
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
    QTreeWidgetItem::setData(column, role, value);
    if (role == Qt::EditRole) {
        QString label = value.toString();
        object()->getObject()->Label.setValue((const char*)label.toUtf8());
    }
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
    if(checkMap) {
        auto it = _ParentMap.find(object()->getObject());
        if(it!=_ParentMap.end() && it->second.size()) {
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

            getOwnerDocument()->populateObject(*it->second.begin());
            return false;
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
        std::ostringstream &str, DocumentObjectItem *cousin) const
{
    std::ostringstream str2;
    App::DocumentObject *top=0,*top2=0;
    getSubName(str,top);
    if(!top)
        return 0;
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
                FC_ERR("invalid subname " << top->getNameInDocument() << '.' << substr);
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

