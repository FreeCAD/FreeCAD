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
#include "Widgets.h"

using namespace Gui;

QPixmap*  TreeWidget::documentPixmap = 0;
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;


/* TRANSLATOR Gui::TreeWidget */
TreeWidget::TreeWidget(QWidget* parent)
    : QTreeWidget(parent), contextItem(0), fromOutside(false)
{
    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);
    this->setRootIsDecorated(false);

    this->createGroupAction = new QAction(this);
    this->createGroupAction->setText(tr("Create group..."));
    this->createGroupAction->setStatusTip(tr("Create a group"));
    connect(this->createGroupAction, SIGNAL(triggered()),
            this, SLOT(onCreateGroup()));

    this->relabelObjectAction = new QAction(this);
    this->relabelObjectAction->setText(tr("Rename"));
    this->relabelObjectAction->setStatusTip(tr("Rename object"));
#ifndef Q_OS_MAC
    this->relabelObjectAction->setShortcut(Qt::Key_F2);
#endif
    connect(this->relabelObjectAction, SIGNAL(triggered()),
            this, SLOT(onRelabelObject()));

    this->finishEditingAction = new QAction(this);
    this->finishEditingAction->setText(tr("Finish editing"));
    this->finishEditingAction->setStatusTip(tr("Finish editing object"));
    connect(this->finishEditingAction, SIGNAL(triggered()),
            this, SLOT(onFinishEditing()));

    this->skipRecomputeAction = new QAction(this);
    this->skipRecomputeAction->setCheckable(true);
    this->skipRecomputeAction->setText(tr("Skip recomputes"));
    this->skipRecomputeAction->setStatusTip(tr("Enable or disable recomputations of document"));
    connect(this->skipRecomputeAction, SIGNAL(toggled(bool)),
            this, SLOT(onSkipRecompute(bool)));

    this->markRecomputeAction = new QAction(this);
    this->markRecomputeAction->setText(tr("Mark to recompute"));
    this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));
    connect(this->markRecomputeAction, SIGNAL(triggered()),
            this, SLOT(onMarkRecompute()));

    this->searchObjectsAction = new QAction(this);
    this->searchObjectsAction->setText(tr("Search..."));
    this->searchObjectsAction->setStatusTip(tr("Search for objects"));
    connect(this->searchObjectsAction, SIGNAL(triggered()),
            this, SLOT(onSearchObjects()));

    // Setup connections
    connectNewDocument = Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, _1));
    connectDelDocument = Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, _1));
    connectRenDocument = Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, _1));
    connectActDocument = Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, _1));
    connectRelDocument = Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, _1));

    QStringList labels;
    labels << tr("Labels & Attributes");
    this->setHeaderLabels(labels);
    // make sure to show a horizontal scrollbar if needed
#if QT_VERSION >= 0x050000
    this->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
    this->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif
    this->header()->setStretchLastSection(false);

    // Add the first main label
    this->rootItem = new QTreeWidgetItem(this);
    this->rootItem->setText(0, tr("Application"));
    this->rootItem->setFlags(Qt::ItemIsEnabled);
    this->expandItem(this->rootItem);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
#if QT_VERSION >= 0x040200
    // causes unexpected drop events (possibly only with Qt4.1.x)
    this->setMouseTracking(true); // needed for itemEntered() to work
#endif

    this->statusTimer = new QTimer(this);

    connect(this->statusTimer, SIGNAL(timeout()),
            this, SLOT(onTestStatus()));
    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*, int)),
            this, SLOT(onItemEntered(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    this->statusTimer->setSingleShot(true);
    this->statusTimer->start(300);
    documentPixmap = new QPixmap(Gui::BitmapFactory().pixmap("Document"));

    this->setDefaultDropAction(Qt::MoveAction);
}

TreeWidget::~TreeWidget()
{
    connectNewDocument.disconnect();
    connectDelDocument.disconnect();
    connectRenDocument.disconnect();
    connectActDocument.disconnect();
    connectRelDocument.disconnect();
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

    // get the current item
    this->contextItem = itemAt(e->pos());
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        if (!contextMenu.actions().isEmpty())
            contextMenu.addSeparator();
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        this->skipRecomputeAction->setChecked(doc->testStatus(App::Document::SkipRecompute));
        contextMenu.addAction(this->skipRecomputeAction);
        contextMenu.addAction(this->markRecomputeAction);
        contextMenu.addAction(this->createGroupAction);
        contextMenu.addAction(this->searchObjectsAction);
    }
    else if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        if (objitem->object()->getObject()->isDerivedFrom(App::DocumentObjectGroup
            ::getClassTypeId())) {
            QList<QAction*> acts = contextMenu.actions();
            if (!acts.isEmpty()) {
                QAction* first = acts.front();
                QAction* sep = contextMenu.insertSeparator(first);
                contextMenu.insertAction(sep, this->createGroupAction);
            }
            else
                contextMenu.addAction(this->createGroupAction);
        }
        if (!contextMenu.actions().isEmpty())
            contextMenu.addSeparator();
        contextMenu.addAction(this->markRecomputeAction);
        contextMenu.addAction(this->relabelObjectAction);

        // if only one item is selected setup the edit menu
        if (this->selectedItems().size() == 1) {
            objitem->object()->setupContextMenu(&editMenu, this, SLOT(onStartEditing()));
            QList<QAction*> editAct = editMenu.actions();
            if (!editAct.isEmpty()) {
                QAction* topact = contextMenu.actions().front();
                for (QList<QAction*>::iterator it = editAct.begin(); it != editAct.end(); ++it)
                    contextMenu.insertAction(topact, *it);
                QAction* first = editAct.front();
                contextMenu.setDefaultAction(first);
                if (objitem->object()->isEditing())
                    contextMenu.insertAction(topact, this->finishEditingAction);
                contextMenu.insertSeparator(topact);
            }
        }
    }

    // add a submenu to active a document if two or more exist
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    if (docs.size() >= 2) {
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

void TreeWidget::onCreateGroup()
{
    QString name = tr("Group");
    if (this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        QString cmd = QString::fromLatin1("App.getDocument(\"%1\").addObject"
                              "(\"App::DocumentObjectGroup\",\"%2\")")
                              .arg(QString::fromLatin1(doc->getName()), name);
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
                              .arg(QString::fromLatin1(doc->getName()),
                                   QString::fromLatin1(obj->getNameInDocument()),
                                   name);
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
            if (!obj) return;
            Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
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
            doc->setEdit(objitem->object(), edit);
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

void TreeWidget::onSearchObjects()
{
    emitSearchObjects();
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
    Qt::DropActions da = QTreeWidget::supportedDropActions();
    QList<QModelIndex> idxs = selectedIndexes();

    if (idxs.size() == 1) {
        // as we can't know here where the drop action will occur that is the best we can do
        da |= Qt::LinkAction;
    }

    for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
        QTreeWidgetItem* item = itemFromIndex(*it);
        if (item->type() != TreeWidget::ObjectType) {
            return Qt::IgnoreAction;
        }
        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            if (parent->type() != TreeWidget::ObjectType) {
                da &= ~Qt::MoveAction;
            }
            else {
                // Now check for object with a parent that is an ObjectType, too.
                // If this object is *not* selected and *not* a group we are not allowed to remove
                // its child (e.g. the sketch of a pad).
                Gui::ViewProvider* vp = static_cast<DocumentObjectItem *>(parent)->object();
                App::DocumentObject* obj = static_cast<DocumentObjectItem*>(item)->
                    object()->getObject();
                if (!vp->canDragObjects() || !vp->canDragObject(obj)) {
                    da &= ~Qt::MoveAction;
                }
            }
        }
        else {
            da &= ~Qt::MoveAction;
        }
    }

    return da;
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
        MDIView *view = doc->getActiveView();
        if (!view) return;
        getMainWindow()->setActiveWindow(view);
    }
    else if (item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(item);
        App::DocumentObject* obj = objitem->object()->getObject();
        Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
        MDIView *view = doc->getActiveView();
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

QList<App::DocumentObject *> TreeWidget::buildListChildren(QTreeWidgetItem* targetitem, Gui::ViewProviderDocumentObject* vp)
{
    // to check we do not drop the part on itself
    QList<App::DocumentObject *> children;
    children << vp->getObject();
    for (int i=0; i<targetitem->childCount(); i++) {
        Gui::ViewProviderDocumentObject* vpc = static_cast<DocumentObjectItem*>(targetitem->child(i))->object();
        children << vpc->getObject();
    }
    return children;
}

void TreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    QTreeWidgetItem* targetItem = itemAt(event->pos());
    if (!targetItem || this->isItemSelected(targetItem)) {
        event->ignore();
    }
    else {
        QTreeWidgetItem* parentTarget = targetItem->parent();
        Qt::DropAction da = event->proposedAction();

        if (targetItem->type() == TreeWidget::DocumentType) {
            QList<QModelIndex> idxs = selectedIndexes();
            App::Document* doc = static_cast<DocumentItem*>(targetItem)->
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

                // Dropping an object on the document that is not part of a composed object is non-sense
                // Dropping an object on document means to remove the object of a composed object
                QTreeWidgetItem* parent = item->parent();
                if (!parent || parent->type() != TreeWidget::ObjectType) {
                    event->ignore();
                    return;
                }
            }

            // Link is non-sense on a document.
            if (da == Qt::LinkAction) {
                if (event->possibleActions() & Qt::MoveAction) {
                    event->setDropAction(Qt::MoveAction);
                    event->accept();
                }
                else {
                    event->ignore();
                }
            }
        }
        else if (targetItem->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
            Gui::ViewProviderDocumentObject* vpTarget = targetItemObj->object();
            QList<App::DocumentObject *> children = buildListChildren(targetItem, vpTarget);
            App::DocumentObject* targetObj = vpTarget->getObject();
            App::Document* docTarget = targetObj->getDocument();

            QList<QModelIndex> idxsSelected = selectedIndexes();

            std::vector<const App::DocumentObject*> dropObjects;
            dropObjects.reserve(idxsSelected.size());

            for (QList<QModelIndex>::Iterator it = idxsSelected.begin(); it != idxsSelected.end(); ++it) {
                QTreeWidgetItem* item = itemFromIndex(*it);
                if (item->type() != TreeWidget::ObjectType) {
                    event->ignore();
                    return;
                }

                Gui::ViewProviderDocumentObject* vpDropped = static_cast<DocumentObjectItem*>(item)->object();
                App::DocumentObject* objDropped = vpDropped->getObject();
                if (docTarget != objDropped->getDocument()) {
                    event->ignore();
                    return;
                }

                dropObjects.push_back(objDropped);

                // To avoid a cylic dependency it must be made sure to not allow to
                // drag'n'drop a tree item onto a child or grandchild item of it.
                bool canDrop = !targetObj->isInInListRecursive(objDropped);

                // if the item is already a child of the target item there is nothing to do
                canDrop = canDrop && vpTarget->canDropObjects() && !children.contains(objDropped);

                if (!canDrop) {
                    if (idxsSelected.size() == 1) {
                        if (parentTarget) {
                            if (parentTarget->type() == TreeWidget::ObjectType) {
                                DocumentObjectItem* parentTargetItemObj =
                                    static_cast<DocumentObjectItem*>(parentTarget);
                                Gui::ViewProviderDocumentObject* vpParent = parentTargetItemObj->object();
                                App::DocumentObject* objParentTarget = vpParent->getObject();

                                if (!buildListChildren(parentTarget, vpParent).contains(objDropped) &&
                                    !objParentTarget->isInInListRecursive(objDropped)) {
                                    if (da == Qt::CopyAction || da == Qt::MoveAction) {
                                        event->setDropAction(Qt::LinkAction);
                                        event->accept();
                                        return;
                                    }
                                    else if (da == Qt::LinkAction) {
                                        event->acceptProposedAction();
                                        return;
                                    }
                                }
                            }
                        }
                    }

                    event->ignore();
                    return;
                }
                else {
                    // let the view provider decide to accept the object or ignore it
                    if (!vpTarget->canDropObject(objDropped)) {
                        event->ignore();
                        return;
                    }
                    else if (da == Qt::LinkAction) {
                        if (!parentTarget || parentTarget->type() != TreeWidget::ObjectType) {
                            // no need to test compatibility between parent and objDropped and objParentTarget
                            // as test between objDropped and targetObj has been tested before
                            event->ignore();
                            return;
                        }
                    }
                }
            }
            event->acceptProposedAction();
        }
        else {
            event->ignore();
        }
    }
}

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
    QList<QTreeWidgetItem*> items;
    QList<QModelIndex> idxs = selectedIndexes();
    for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
        // ignore child elements if the parent is selected (issue #0001456)
        QModelIndex parent = (*it).parent();
        if (idxs.contains(parent))
            continue;
        QTreeWidgetItem* item = itemFromIndex(*it);
        if (item == targetItem)
            continue;
        if (item->parent() == targetItem)
            continue;
        items.push_back(item);
    }

    if (items.isEmpty())
        return; // nothing needs to be done

    if (targetItem->type() == TreeWidget::ObjectType) {
        // add object to group
        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);

        Qt::DropAction da = event->dropAction();

        Gui::ViewProviderDocumentObject* vpTarget = targetItemObj->object();
        Gui::Document* gui = vpTarget->getDocument();

        if (da == Qt::LinkAction) {
            // Open command
            gui->openCommand("Drop object");
            for (QList<QTreeWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
                Gui::ViewProviderDocumentObject* vpDropped = static_cast<DocumentObjectItem*>(*it)->object();
                App::DocumentObject* objDropped = vpDropped->getObject();

                // does this have a parent object
                QTreeWidgetItem* parent = targetItemObj->parent();

                if (parent && parent->type() == TreeWidget::ObjectType) {
                    Gui::ViewProvider* vpParent = static_cast<DocumentObjectItem *>(parent)->object();
                    // now add the object to the target object
                    vpParent->replaceObject(vpTarget->getObject(), objDropped);
                }

            }
            gui->commitCommand();
        }
        else {
            if (!vpTarget->canDropObjects()) {
                return; // no group like object
            }

            bool dropOnly = da == Qt::CopyAction;

            // Open command
            gui->openCommand("Drag object");
            for (QList<QTreeWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
                Gui::ViewProviderDocumentObject* vpDropped = static_cast<DocumentObjectItem*>(*it)->object();
                App::DocumentObject* objDropped = vpDropped->getObject();

                if (!dropOnly) {
                    // does this have a parent object
                    QTreeWidgetItem* parent = (*it)->parent();
                    if (parent && parent->type() == TreeWidget::ObjectType) {
                        Gui::ViewProvider* vpParent = static_cast<DocumentObjectItem *>(parent)->object();
                        vpParent->dragObject(objDropped);
                    }
                }

                // now add the object to the target object
                vpTarget->dropObject(objDropped);
            }
            gui->commitCommand();
        }
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        // Open command
        bool commit = false;
        App::Document* doc = static_cast<DocumentItem*>(targetItem)->document()->getDocument();
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Move object");
        for (QList<QTreeWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
            Gui::ViewProviderDocumentObject* vpDropped = static_cast<DocumentObjectItem*>(*it)->object();
            App::DocumentObject* objDropped = vpDropped->getObject();

            // does this have a parent object
            QTreeWidgetItem* parent = (*it)->parent();
            if (parent && parent->type() == TreeWidget::ObjectType) {
                Gui::ViewProvider* vpParent = static_cast<DocumentObjectItem *>(parent)->object();
                if (vpParent->canDragObject(objDropped)) {
                    vpParent->dragObject(objDropped);
                    commit = true;
                }
            }
        }

        if (commit)
            gui->commitCommand();
        else
            gui->abortCommand();
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

void TreeWidget::slotDeleteDocument(const Gui::Document& Doc)
{
    std::map<const Gui::Document*, DocumentItem*>::iterator it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
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
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int displayMode = hGrp->GetInt("TreeViewDocument", 0);
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


void TreeWidget::onTestStatus(void)
{
    if (isVisible()) {
        std::map<const Gui::Document*,DocumentItem*>::iterator pos;
        for (pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
            pos->second->testStatus();
        }
    }

    this->statusTimer->setSingleShot(true);
    this->statusTimer->start(300);
}

void TreeWidget::onItemEntered(QTreeWidgetItem * item)
{
    // object item selected
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* obj = static_cast<DocumentObjectItem*>(item);
        obj->displayStatusInfo();
    }
}

void TreeWidget::onItemCollapsed(QTreeWidgetItem * item)
{
    // object item collapsed
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* obj = static_cast<DocumentObjectItem*>(item);
        obj->setExpandedStatus(false);
    }
}

void TreeWidget::onItemExpanded(QTreeWidgetItem * item)
{
    // object item expanded
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* obj = static_cast<DocumentObjectItem*>(item);
        obj->setExpandedStatus(true);
        auto it = DocumentMap.find(obj->object()->getDocument());
        if(it==DocumentMap.end())
            Base::Console().Warning("DocumentItem::onItemExpanded: cannot find object document\n");
        else
            it->second->populateItem(obj);
    }
}

void TreeWidget::scrollItemToTop(Gui::Document* doc)
{
    std::map<const Gui::Document*,DocumentItem*>::iterator it;
    it = DocumentMap.find(doc);
    if (it != DocumentMap.end()) {
        DocumentItem* root = it->second;
        QTreeWidgetItemIterator it(root, QTreeWidgetItemIterator::Selected);
        for (; *it; ++it) {
            if ((*it)->type() == TreeWidget::ObjectType) {
                this->scrollToItem(*it, QAbstractItemView::PositionAtTop);
                break;
            }
        }
    }
}

void TreeWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->headerItem()->setText(0, tr("Labels & Attributes"));
        this->rootItem->setText(0, tr("Application"));

        this->createGroupAction->setText(tr("Create group..."));
        this->createGroupAction->setStatusTip(tr("Create a group"));

        this->relabelObjectAction->setText(tr("Rename"));
        this->relabelObjectAction->setStatusTip(tr("Rename object"));

        this->finishEditingAction->setText(tr("Finish editing"));
        this->finishEditingAction->setStatusTip(tr("Finish editing object"));

        this->skipRecomputeAction->setText(tr("Skip recomputes"));
        this->skipRecomputeAction->setStatusTip(tr("Enable or disable recomputations of document"));

        this->markRecomputeAction->setText(tr("Mark to recompute"));
        this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));
    }

    QTreeWidget::changeEvent(e);
}

void TreeWidget::onItemSelectionChanged ()
{
    // we already got notified by the selection to update the tree items
    if (this->isConnectionBlocked())
        return;

    // block tmp. the connection to avoid to notify us ourself
    bool lock = this->blockConnection(true);
    std::map<const Gui::Document*,DocumentItem*>::iterator pos;
    for (pos = DocumentMap.begin();pos!=DocumentMap.end();++pos) {
        pos->second->updateSelection();
    }
    this->blockConnection(lock);
}

void TreeWidget::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
        {
            Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
            std::map<const Gui::Document*, DocumentItem*>::iterator it;
            it = DocumentMap.find(pDoc);
            bool lock = this->blockConnection(true);
            if (it!= DocumentMap.end())
                it->second->setObjectSelected(msg.pObjectName,true);
            this->blockConnection(lock);
        }   break;
    case SelectionChanges::RmvSelection:
        {
            Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
            std::map<const Gui::Document*, DocumentItem*>::iterator it;
            it = DocumentMap.find(pDoc);
            bool lock = this->blockConnection(true);
            if (it!= DocumentMap.end())
                it->second->setObjectSelected(msg.pObjectName,false);
            this->blockConnection(lock);
        }   break;
    case SelectionChanges::SetSelection:
        {
            Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
            std::map<const Gui::Document*, DocumentItem*>::iterator it;
            it = DocumentMap.find(pDoc);
            // we get notified from the selection and must only update the selection on the tree,
            // thus no need to notify again the selection. See also onItemSelectionChanged().
            if (it != DocumentMap.end()) {
                bool lock = this->blockConnection(true);
                it->second->selectItems();
                this->blockConnection(lock);
            }
        }   break;
    case SelectionChanges::ClrSelection:
        {
            // clears the complete selection
            if (strcmp(msg.pDocName,"") == 0) {
                this->clearSelection ();
            }
            else {
                // clears the selection of the given document
                Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
                std::map<const Gui::Document*, DocumentItem*>::iterator it;
                it = DocumentMap.find(pDoc);
                if (it != DocumentMap.end()) {
                    it->second->clearSelection();
                }
            }
            this->update();
        }   break;
    case SelectionChanges::SetPreselect:
        {
            Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
            std::map<const Gui::Document*, DocumentItem*>::iterator it;
            it = DocumentMap.find(pDoc);
            if (it!= DocumentMap.end())
                it->second->setObjectHighlighted(msg.pObjectName,true);
        }   break;
    case SelectionChanges::RmvPreselect:
        {
            Gui::Document* pDoc = Application::Instance->getDocument(msg.pDocName);
            std::map<const Gui::Document*, DocumentItem*>::iterator it;
            it = DocumentMap.find(pDoc);
            if (it!= DocumentMap.end())
                it->second->setObjectHighlighted(msg.pObjectName,false);
        }   break;
    default:
        break;
    }
}

void TreeWidget::setItemsSelected (const QList<QTreeWidgetItem *> items, bool select)
{
    if (items.isEmpty())
        return;
    QItemSelection range;
    for (QList<QTreeWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it)
        range.select(this->indexFromItem(*it),this->indexFromItem(*it));
    selectionModel()->select(range, select ?
        QItemSelectionModel::Select :
        QItemSelectionModel::Deselect);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreePanel */

TreePanel::TreePanel(QWidget* parent)
  : QWidget(parent)
{
    this->treeWidget = new TreeWidget(this);
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
            this, SLOT(findMatchingItems(QString)));
}

TreePanel::~TreePanel()
{
}

void TreePanel::accept()
{
    QString text = this->searchBox->text();
    if (!text.isEmpty()) {
        for (int i=0; i<treeWidget->topLevelItemCount(); i++) {
            selectTreeItem(treeWidget->topLevelItem(i), text);
        }
    }
    hideEditor();
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
}

void TreePanel::hideEditor()
{
    this->searchBox->clear();
    this->searchBox->hide();
    for (int i=0; i<treeWidget->topLevelItemCount(); i++) {
        resetBackground(treeWidget->topLevelItem(i));
    }
}

void TreePanel::findMatchingItems(const QString& text)
{
    if (text.isEmpty()) {
        for (int i=0; i<treeWidget->topLevelItemCount(); i++) {
            resetBackground(treeWidget->topLevelItem(i));
        }
    }
    else {
        for (int i=0; i<treeWidget->topLevelItemCount(); i++) {
            searchTreeItem(treeWidget->topLevelItem(i), text);
        }
    }
}

void TreePanel::searchTreeItem(QTreeWidgetItem* item, const QString& text)
{
    for (int i=0; i<item->childCount(); i++) {
        QTreeWidgetItem* child = item->child(i);
        child->setBackground(0, QBrush());
        child->setExpanded(false);
        if (child->text(0).indexOf(text, 0, Qt::CaseInsensitive) >= 0) {
            child->setBackground(0, QColor(255, 255, 0, 100));
            QTreeWidgetItem* parent = child->parent();
            while (parent) {
                parent->setExpanded(true);
                parent = parent->parent();
            }
        }
        searchTreeItem(child, text);
    }
}

void TreePanel::selectTreeItem(QTreeWidgetItem* item, const QString& text)
{
    for (int i=0; i<item->childCount(); i++) {
        QTreeWidgetItem* child = item->child(i);
        if (child->text(0).indexOf(text, 0, Qt::CaseInsensitive) >= 0) {
            child->setSelected(true);
        }
        selectTreeItem(child, text);
    }
}

void TreePanel::resetBackground(QTreeWidgetItem* item)
{
    for (int i=0; i<item->childCount(); i++) {
        QTreeWidgetItem* child = item->child(i);
        child->setBackground(0, QBrush());
        resetBackground(child);
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreeDockWidget */

TreeDockWidget::TreeDockWidget(Gui::Document* pcDocument,QWidget *parent)
  : DockWindow(pcDocument,parent)
{
    setWindowTitle(tr("Tree view"));
    this->treeWidget = new TreeWidget(this);
    this->treeWidget->setRootIsDecorated(false);
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    this->treeWidget->setIndentation(hGrp->GetInt("Indentation", this->treeWidget->indentation()));

    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin (0);
    pLayout->addWidget(this->treeWidget, 0, 0 );
}

TreeDockWidget::~TreeDockWidget()
{
}

// ----------------------------------------------------------------------------

DocumentItem::DocumentItem(const Gui::Document* doc, QTreeWidgetItem * parent)
    : QTreeWidgetItem(parent, TreeWidget::DocumentType), pDocument(doc)
{
    // Setup connections
    connectNewObject = doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, _1));
    connectDelObject = doc->signalDeletedObject.connect(boost::bind(&DocumentItem::slotDeleteObject, this, _1));
    connectChgObject = doc->signalChangedObject.connect(boost::bind(&DocumentItem::slotChangeObject, this, _1));
    connectRenObject = doc->signalRelabelObject.connect(boost::bind(&DocumentItem::slotRenameObject, this, _1));
    connectActObject = doc->signalActivatedObject.connect(boost::bind(&DocumentItem::slotActiveObject, this, _1));
    connectEdtObject = doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, _1));
    connectResObject = doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, _1));
    connectHltObject = doc->signalHighlightObject.connect(boost::bind(&DocumentItem::slotHighlightObject, this, _1,_2,_3));
    connectExpObject = doc->signalExpandObject.connect(boost::bind(&DocumentItem::slotExpandObject, this, _1,_2));
    connectScrObject = doc->signalScrollToObject.connect(boost::bind(&DocumentItem::slotScrollToObject, this, _1));

    setFlags(Qt::ItemIsEnabled/*|Qt::ItemIsEditable*/);
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
}

#define FOREACH_ITEM(_item, _obj) \
    auto _it = ObjectMap.find(std::string(_obj.getObject()->getNameInDocument()));\
    if(_it == ObjectMap.end() || _it->second->empty()) return;\
    for(auto _item : *_it->second){{

#define FOREACH_ITEM_ALL(_item) \
    for(auto _v : ObjectMap) {\
        for(auto _item : *_v.second) {

#define FOREACH_ITEM_NAME(_item,_name) \
    auto _it = ObjectMap.find(_name);\
    if(_it != ObjectMap.end()) {\
        for(auto _item : *_it->second) {

#define END_FOREACH_ITEM }}


void DocumentItem::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    unsigned long col = hGrp->GetUnsigned("TreeEditColor",4294902015);
    FOREACH_ITEM(item,v)
        item->setBackgroundColor(0,QColor((col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff));
    END_FOREACH_ITEM
}

void DocumentItem::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    FOREACH_ITEM(item,v)
        item->setData(0, Qt::BackgroundColorRole,QVariant());
    END_FOREACH_ITEM
}

void DocumentItem::slotNewObject(const Gui::ViewProviderDocumentObject& obj) {
    createNewItem(obj);
}

bool DocumentItem::createNewItem(const Gui::ViewProviderDocumentObject& obj,
            QTreeWidgetItem *parent, int index, DocumentObjectItemsPtr ptrs)
{
    const char *name;
    if (!obj.showInTree() || !(name=obj.getObject()->getNameInDocument()))
        return false;

    if (!ptrs) {
        auto &items = ObjectMap[name];
        if (!items) {
            items.reset(new DocumentObjectItems);
        }
        else if(items->size() && parent==NULL) {
            Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
            return false;
        }
        ptrs = items;
    }

    std::string displayName = obj.getObject()->Label.getValue();
    DocumentObjectItem* item = new DocumentObjectItem(
        const_cast<Gui::ViewProviderDocumentObject*>(&obj), ptrs);

    if (!parent)
        parent = this;
    if (index<0)
        parent->addChild(item);
    else
        parent->insertChild(index,item);

    // Couldn't be added and thus don't continue populating it
    // and delete it again
    if (!item->parent()) {
        delete item;
    }
    else {
        item->setIcon(0, obj.getIcon());
        item->setText(0, QString::fromUtf8(displayName.c_str()));
        populateItem(item);
    }

    return true;
}

static bool canCreateItem(const App::DocumentObject *obj, const Document *doc)
{
    // Note: It is possible that we receive an invalid pointer from
    // claimChildren(), e.g. if multiple properties were changed in
    // a transaction and slotChangedObject() is triggered by one
    // property being reset before the invalid pointer has been
    // removed from another. Currently this happens for
    // PartDesign::Body when cancelling a new feature in the dialog.
    // First the new feature is deleted, then the Tip property is
    // reset, but claimChildren() accesses the Model property which
    // still contains the pointer to the deleted feature
    return obj && obj->getNameInDocument() && doc->getDocument()->isIn(obj);
}

void DocumentItem::slotDeleteObject(const Gui::ViewProviderDocumentObject& view)
{
    auto it = ObjectMap.find(std::string(view.getObject()->getNameInDocument()));
    if (it == ObjectMap.end() || it->second->empty())
        return;

    auto &items = *(it->second);
    for (auto cit=items.begin(),citNext=cit;cit!=items.end();cit=citNext) {
        ++citNext;
        delete *cit;
    }

    if (items.empty())
        ObjectMap.erase(it);

    // Check for any child of the deleted object is not in the tree, and put it
    // under document item.
    const auto &children = view.claimChildren();
    for (auto child : children) {
        if (!canCreateItem(child,pDocument))
            continue;
        auto it = ObjectMap.find(child->getNameInDocument());
        if (it==ObjectMap.end() || it->second->empty()) {
            ViewProvider* vp = pDocument->getViewProvider(child);
            if (!vp || !vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                continue;
            createNewItem(static_cast<ViewProviderDocumentObject&>(*vp));
        }
    }
}

void DocumentItem::populateItem(DocumentObjectItem *item, bool refresh)
{
    if (item->populated && !refresh)
        return;

    // Lazy loading policy: We will create an item for each children object if
    // a) the item is expanded, or b) there is at least one free child, i.e.
    // child originally located at root.

    const auto &children = item->object()->claimChildren();

    item->setChildIndicatorPolicy(children.empty()?
            QTreeWidgetItem::DontShowIndicator:QTreeWidgetItem::ShowIndicator);

    if (!item->populated && !item->isExpanded()) {
        bool doPopulate = false;
        for (auto child : children) {
            if (!canCreateItem(child,pDocument))
                continue;
            auto it = ObjectMap.find(child->getNameInDocument());
            if (it == ObjectMap.end() || it->second->empty()) {
                ViewProvider* vp = pDocument->getViewProvider(child);
                if (!vp || !vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
                    continue;
                doPopulate = true;
                break;
            }

            if ((*it->second->begin())->parent() == this) {
                doPopulate = true;
                break;
            }
        }

        if (!doPopulate)
            return;
    }

    item->populated = true;

    int i=-1;
    // iterate through the claimed children, and try to synchronize them with the
    // children tree item with the same order of appearance.
    for (auto child : children) {
        if (!canCreateItem(child,pDocument))
            continue;

        ++i; // the current index of the claimed child

        bool found = false;
        for (int j=0,count=item->childCount();j<count;++j) {
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
                if (!ci->parent()) {
                    delete ci;
                }
            }
            break;
        }

        if (found)
            continue;

        // This algo will be recursively applied to newly created child items
        // through slotNewObject -> populateItem

        auto it = ObjectMap.find(child->getNameInDocument());
        if (it==ObjectMap.end() || it->second->empty()) {
            ViewProvider* vp = pDocument->getViewProvider(child);
            if (!vp || !vp->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()) ||
                !createNewItem(static_cast<ViewProviderDocumentObject&>(*vp),item,i,
                        it==ObjectMap.end()?DocumentObjectItemsPtr():it->second))
                --i;
            continue;
        }

        DocumentObjectItem *childItem = *it->second->begin();
        if (childItem->parent() != this) {
            if(!createNewItem(*childItem->object(),item,i,it->second))
                --i;
        }
        else {
            if (item==childItem || item->isChildOfItem(childItem)) {
                Base::Console().Error("Gui::DocumentItem::populateItem(): Cyclic dependency in %s and %s\n",
                        item->object()->getObject()->Label.getValue(),
                        childItem->object()->getObject()->Label.getValue());
                --i;
                continue;
            }

            this->removeChild(childItem);
            item->insertChild(i,childItem);
            if (!childItem->parent()) {
                delete childItem;
            }
        }
    }

    App::GeoFeatureGroupExtension* grp = nullptr;
    if (item->object()->getObject()->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
        grp = item->object()->getObject()->getExtensionByType<App::GeoFeatureGroupExtension>();

    // When removing a child element then it must either be moved to a new
    // parent or deleted. Just removing and leaving breaks the Qt internal
    // notification (See #0003201).
    for (++i;item->childCount()>i;) {
        QTreeWidgetItem *childItem = item->child(i);
        item->removeChild(childItem);
        if (childItem->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* obj = static_cast<DocumentObjectItem*>(childItem);
            // Add the child item back to document root if it is the only
            // instance.  Now, because of the lazy loading strategy, this may
            // not truly be the last instance of the object. It may belong to
            // other parents not expanded yet. We don't want to traverse the
            // whole tree to confirm that. Just let it be. If the other
            // parent(s) later expanded, this child item will be moved from
            // root to its parent.
            if (obj->myselves->size()==1) {
                // We only make a difference for geofeaturegroups,
                // as otherwise it comes to confusing behavior to the user when things
                // get claimed within the group (e.g. pad/sketch, or group)
                if (!grp || !grp->hasObject(obj->object()->getObject(), true)) {
                    this->addChild(childItem);
                    continue;
                }
            }
        }

        delete childItem;
    }
}

void DocumentItem::slotChangeObject(const Gui::ViewProviderDocumentObject& view)
{
    QString displayName = QString::fromUtf8(view.getObject()->Label.getValue());
    FOREACH_ITEM(item,view)
        item->setText(0, displayName);
        populateItem(item, true);
    END_FOREACH_ITEM

    //if the item is in a GeoFeatureGroup we may need to update that too, as the claim children
    //of the geofeaturegroup depends on what the childs claim
    auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(view.getObject());
    if (grp) {
        FOREACH_ITEM_NAME(item, grp->getNameInDocument())
            populateItem(item, true);
        END_FOREACH_ITEM
    }
}

void DocumentItem::slotRenameObject(const Gui::ViewProviderDocumentObject& obj)
{
    // Do nothing here because the Label is set in slotChangeObject
    Q_UNUSED(obj);
}

void DocumentItem::slotActiveObject(const Gui::ViewProviderDocumentObject& obj)
{
#if 0
    std::string objectName = obj.getObject()->getNameInDocument();
    if (ObjectMap.find(objectName) == ObjectMap.end())
        return; // signal is emitted before the item gets created

    for (auto v : ObjectMap) {
        for (auto item : *v.second) {
            QFont f = item->font(0);
            f.setBold(item->object() == &obj);
            item->setFont(0,f);
        }
    }
#else
    Q_UNUSED(obj);
#endif
}

void DocumentItem::slotHighlightObject (const Gui::ViewProviderDocumentObject& obj, const Gui::HighlightMode& high, bool set)
{
    FOREACH_ITEM(item,obj)
        QFont f = item->font(0);
        auto highlight = [&item, &set](const QColor& col){
            if (set)
                item->setBackgroundColor(0, col);
            else
                item->setData(0, Qt::BackgroundColorRole,QVariant());
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

        item->setFont(0,f);
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
        switch (mode) {
        case Gui::ExpandPath: {
            QTreeWidgetItem* parent = item->parent();
            while (parent) {
                parent->setExpanded(true);
                parent = parent->parent();
            }
            item->setExpanded(true);
        }   break;
        case Gui::ExpandItem:
            item->setExpanded(true);
            break;
        case Gui::CollapseItem:
            item->setExpanded(false);
            break;
        case Gui::ToggleItem:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;

        default:
            break;
        }
        populateItem(item);
    END_FOREACH_ITEM
}

void DocumentItem::slotScrollToObject(const Gui::ViewProviderDocumentObject& obj)
{
    FOREACH_ITEM(item,obj)
        QTreeWidget* tree = item->treeWidget();
        tree->scrollToItem(item, QAbstractItemView::PositionAtTop);
    END_FOREACH_ITEM
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
//    pos = ObjectMap.find(Obj->getNameInDocument());
//    if (pos != ObjectMap.end()) {
//        QFont f = pos->second->font(0);
//        f.setUnderline(mark);
//        pos->second->setFont(0,f);
//    }
//}

void DocumentItem::testStatus(void)
{
    FOREACH_ITEM_ALL(item);
        item->testStatus();
    END_FOREACH_ITEM;
}

void DocumentItem::setData (int column, int role, const QVariant & value)
{
    if (role == Qt::EditRole) {
        QString label = value.toString();
        pDocument->getDocument()->Label.setValue((const char*)label.toUtf8());
    }

    QTreeWidgetItem::setData(column, role, value);
}

void DocumentItem::setObjectHighlighted(const char* name, bool select)
{
    Q_UNUSED(select);
    Q_UNUSED(name);
    // FOREACH_ITEM_NAME(item,name);
        //pos->second->setData(0, Qt::TextColorRole, QVariant(Qt::red));
        //treeWidget()->setItemSelected(pos->second, select);
    // END_FOREACH_ITEM;
}

void DocumentItem::setObjectSelected(const char* name, bool select)
{
    FOREACH_ITEM_NAME(item,name);
        treeWidget()->setItemSelected(item, select);
    END_FOREACH_ITEM;
}

void DocumentItem::clearSelection(void)
{
    // Block signals here otherwise we get a recursion and quadratic runtime
    bool ok = treeWidget()->blockSignals(true);
    FOREACH_ITEM_ALL(item);
        item->setSelected(false);
    END_FOREACH_ITEM;
    treeWidget()->blockSignals(ok);
}

void DocumentItem::updateSelection(void)
{
    std::vector<App::DocumentObject*> sel;
    FOREACH_ITEM_ALL(item);
        if (treeWidget()->isItemSelected(item))
            sel.push_back(item->object()->getObject());
    END_FOREACH_ITEM;

    Gui::Selection().setSelection(pDocument->getDocument()->getName(), sel);
}

namespace Gui {
struct ObjectItem_Less : public std::binary_function<DocumentObjectItem*,
                                                     DocumentObjectItem*, bool>
{
    bool operator()(DocumentObjectItem* x, DocumentObjectItem* y) const
    {
        return x->object()->getObject() < y->object()->getObject();
    }
};

struct ObjectItem_Equal : public std::binary_function<DocumentObjectItem*,
                                                      App::DocumentObject*, bool>
{
    bool operator()(DocumentObjectItem* x, App::DocumentObject* y) const
    {
        return x->object()->getObject() == y;
    }
};
}

void DocumentItem::selectItems(void)
{
    // get an array of all tree items of the document and sort it in ascending order
    // with regard to their document object
    std::vector<DocumentObjectItem*> items;
    FOREACH_ITEM_ALL(item);
        items.push_back(item);
    END_FOREACH_ITEM;
    std::sort(items.begin(), items.end(), ObjectItem_Less());

    // get and sort all selected document objects of the given document
    std::vector<App::DocumentObject*> objs;
    std::vector<SelectionSingleton::SelObj> obj = Selection().getSelection(pDocument->getDocument()->getName());
    for (std::vector<SelectionSingleton::SelObj>::iterator jt = obj.begin(); jt != obj.end(); ++jt) {
        objs.push_back(jt->pObject);
    }
    std::sort(objs.begin(), objs.end());

    // The document objects in 'objs' is a subset of the document objects stored
    // in 'items'. Since both arrays are sorted we get the wanted tree items in
    // linear time.
    std::vector<DocumentObjectItem*> common;
    std::vector<DocumentObjectItem*>::iterator item_it = items.begin();
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        item_it = std::find_if(item_it, items.end(), std::bind2nd(ObjectItem_Equal(), *it));
        if (item_it == items.end())
            break; // should never ever happen
        common.push_back(*item_it);
    }

    // get all unselected items of the given document
    std::sort(common.begin(), common.end());
    std::sort(items.begin(), items.end());
    std::vector<DocumentObjectItem*> diff;
    std::back_insert_iterator<std::vector<DocumentObjectItem*> > biit(diff);
    std::set_difference(items.begin(), items.end(), common.begin(), common.end(), biit);

    // select the appropriate items
    QList<QTreeWidgetItem *> selitems;
    for (std::vector<DocumentObjectItem*>::iterator it = common.begin(); it != common.end(); ++it)
        selitems.append(*it);
    static_cast<TreeWidget*>(treeWidget())->setItemsSelected(selitems, true);
    // deselect the appropriate items
    QList<QTreeWidgetItem *> deselitems;
    for (std::vector<DocumentObjectItem*>::iterator it = diff.begin(); it != diff.end(); ++it)
        deselitems.append(*it);
    static_cast<TreeWidget*>(treeWidget())->setItemsSelected(deselitems, false);
}

// ----------------------------------------------------------------------------

DocumentObjectItem::DocumentObjectItem(Gui::ViewProviderDocumentObject* pcViewProvider,
                                       DocumentObjectItemsPtr selves)
    : QTreeWidgetItem(TreeWidget::ObjectType), previousStatus(-1), viewObject(pcViewProvider)
    , myselves(selves), populated(false)
{
    setFlags(flags()|Qt::ItemIsEditable);
    // Setup connections
    connectIcon = pcViewProvider->signalChangeIcon.connect(boost::bind(&DocumentObjectItem::slotChangeIcon, this));
    connectTool = pcViewProvider->signalChangeToolTip.connect(boost::bind(&DocumentObjectItem::slotChangeToolTip, this, _1));
    connectStat = pcViewProvider->signalChangeStatusTip.connect(boost::bind(&DocumentObjectItem::slotChangeStatusTip, this, _1));
    myselves->insert(this);
}

DocumentObjectItem::~DocumentObjectItem()
{
    auto it = myselves->find(this);
    if(it == myselves->end())
        assert(0);
    else
        myselves->erase(it);

    connectIcon.disconnect();
    connectTool.disconnect();
    connectStat.disconnect();
}

Gui::ViewProviderDocumentObject* DocumentObjectItem::object() const
{
    return viewObject;
}

void DocumentObjectItem::testStatus()
{
    App::DocumentObject* pObject = viewObject->getObject();

    // if status has changed then continue
    int currentStatus =
        ((pObject->isError()            ? 1 : 0) << 2) |
        ((pObject->mustRecompute() == 1 ? 1 : 0) << 1) |
        (viewObject->isShow()           ? 1 : 0);
    if (previousStatus == currentStatus)
        return;
    previousStatus = currentStatus;

    QPixmap px;
    if (currentStatus & 4) {
        // object is in error state
        static const char * const feature_error_xpm[]={
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
        px = QPixmap(feature_error_xpm);
    }
    else if (currentStatus & 2) {
        // object must be recomputed
        static const char * const feature_recompute_xpm[]={
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
        px = QPixmap(feature_recompute_xpm);
    }

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

    // get the original icon set
    QIcon icon_org = viewObject->getIcon();
    QIcon icon_mod;
    int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);

    // if needed show small pixmap inside
    if (!px.isNull()) {
        icon_mod.addPixmap(BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::Off),
            px,BitmapFactoryInst::TopRight), QIcon::Normal, QIcon::Off);
        icon_mod.addPixmap(BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::On ),
            px,BitmapFactoryInst::TopRight), QIcon::Normal, QIcon::Off);
    }
    else {
        icon_mod.addPixmap(icon_org.pixmap(w, w, mode, QIcon::Off), QIcon::Normal, QIcon::Off);
        icon_mod.addPixmap(icon_org.pixmap(w, w, mode, QIcon::On ), QIcon::Normal, QIcon::On );
    }

    this->setIcon(0, icon_mod);
}

void DocumentObjectItem::displayStatusInfo()
{
    App::DocumentObject* Obj = viewObject->getObject();

    QString info = QString::fromLatin1(Obj->getStatusString());
    if ( Obj->mustExecute() == 1 )
        info += QString::fromLatin1(" (but must be executed)");
    QString status = TreeWidget::tr("%1, Internal name: %2")
            .arg(info,
                 QString::fromLatin1(Obj->getNameInDocument()));
    getMainWindow()->showMessage(status);

    if (Obj->isError()) {
        QTreeWidget* tree = this->treeWidget();
        QPoint pos = tree->visualItemRect(this).topRight();
        QToolTip::showText(tree->mapToGlobal(pos), info);
    }
}

void DocumentObjectItem::setExpandedStatus(bool on)
{
    App::DocumentObject* Obj = viewObject->getObject();
    Obj->setStatus(App::Expand, on);
}

void DocumentObjectItem::setData (int column, int role, const QVariant & value)
{
    QTreeWidgetItem::setData(column, role, value);
    if (role == Qt::EditRole) {
        QString label = value.toString();
        App::DocumentObject* obj = viewObject->getObject();
        App::Document* doc = obj->getDocument();
        doc->openTransaction(TreeWidget::tr("Rename object").toUtf8());
        obj->Label.setValue((const char*)label.toUtf8());
        doc->commitTransaction();
    }
}

bool DocumentObjectItem::isChildOfItem(DocumentObjectItem* item)
{
    int numChild = item->childCount();
    for (int i=0; i<numChild; i++) {
        QTreeWidgetItem* child = item->child(i);
        if (child == this)
            return true;
        if (child->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* obj = static_cast<DocumentObjectItem*>(child);
            if (this->isChildOfItem(obj))
                return true;
        }
    }

    return false;
}

void DocumentObjectItem::slotChangeIcon()
{
    previousStatus = -1;
    testStatus();
}

void DocumentObjectItem::slotChangeToolTip(const QString& tip)
{
    this->setToolTip(0, tip);
}

void DocumentObjectItem::slotChangeStatusTip(const QString& tip)
{
    this->setStatusTip(0, tip);
}

#include "moc_Tree.cpp"

