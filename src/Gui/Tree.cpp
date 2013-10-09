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
#endif

#include <Base/Console.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>

#include "Tree.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"
#include "MenuManager.h"
#include "Application.h"
#include "MainWindow.h"

using namespace Gui;

QPixmap*  TreeWidget::documentPixmap = 0;
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;


/* TRANSLATOR Gui::TreeWidget */
TreeWidget::TreeWidget(QWidget* parent)
    : QTreeWidget(parent), fromOutside(false)
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
    this->relabelObjectAction->setShortcut(Qt::Key_F2);
    connect(this->relabelObjectAction, SIGNAL(triggered()),
            this, SLOT(onRelabelObject()));
    this->finishEditingAction = new QAction(this);
    this->finishEditingAction->setText(tr("Finish editing"));
    this->finishEditingAction->setStatusTip(tr("Finish editing object"));
    connect(this->finishEditingAction, SIGNAL(triggered()),
            this, SLOT(onFinishEditing()));

    // Setup connections
    Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, _1));
    Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, _1));
    Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, _1));
    Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, _1));
    Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, _1));

    QStringList labels;
    labels << tr("Labels & Attributes");
    this->setHeaderLabels(labels);

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
}

TreeWidget::~TreeWidget()
{
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
        contextMenu.addAction(this->createGroupAction);
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
        if (active) active->setChecked(true);
        active = subMenuGroup.checkedAction();
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
        QString cmd = QString::fromAscii("App.getDocument(\"%1\").addObject"
                              "(\"App::DocumentObjectGroup\",\"%2\")")
                              .arg(QString::fromAscii(doc->getName())).arg(name);
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Create group");
        Gui::Application::Instance->runPythonCode(cmd.toUtf8());
        gui->commitCommand();
    }
    else if (this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        App::DocumentObject* obj = objitem->object()->getObject();
        App::Document* doc = obj->getDocument();
        QString cmd = QString::fromAscii("App.getDocument(\"%1\").getObject(\"%2\")"
                              ".newObject(\"App::DocumentObjectGroup\",\"%3\")")
                              .arg(QString::fromAscii(doc->getName()))
                              .arg(QString::fromAscii(obj->getNameInDocument()))
                              .arg(name);
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Create group");
        Gui::Application::Instance->runPythonCode(cmd.toUtf8());
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

bool TreeWidget::dropMimeData(QTreeWidgetItem *parent, int index,
                              const QMimeData *data, Qt::DropAction action)
{
    return QTreeWidget::dropMimeData(parent, index, data, action);
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
        // Now check for object with a parent that is an ObjectType, too.
        // If this object is *not* a group we are not allowed to remove
        // its child (e.g. the sketch of a pad).
        QTreeWidgetItem* parent = (*it)->parent();
        if (parent && parent->type() == TreeWidget::ObjectType) {
            App::DocumentObject* par = static_cast<DocumentObjectItem *>(parent)->object()->getObject();
            if (!par->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId()))
                return 0;
        }
    }
    return QTreeWidget::mimeData(items);
}

void TreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    QTreeWidgetItem* targetitem = itemAt(event->pos());
    if (!targetitem || this->isItemSelected(targetitem)) {
        event->ignore();
    }
    else if (targetitem->type() == TreeWidget::DocumentType) {
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
        App::DocumentObject* grp = static_cast<DocumentObjectItem*>(targetitem)->
            object()->getObject();
        if (!grp->getTypeId().isDerivedFrom(App::DocumentObjectGroup::
            getClassTypeId()))
            event->ignore();
        App::Document* doc = grp->getDocument();
        QList<QModelIndex> idxs = selectedIndexes();
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
            if (obj->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
                if (static_cast<App::DocumentObjectGroup*>(grp)->isChildOf(
                    static_cast<App::DocumentObjectGroup*>(obj))) {
                    event->ignore();
                    return;
                }
            }
        }
    }
    else {
        event->ignore();
    }
}

void TreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem* targetitem = itemAt(event->pos());
    // not dropped onto an item
    if (!targetitem)
        return;
    // one of the source items is also the destination item, that's not allowed
    if (this->isItemSelected(targetitem))
        return;
    
    // filter out the selected items we cannot handle
    QList<QTreeWidgetItem*> items;
    QList<QModelIndex> idxs = selectedIndexes();
    for (QList<QModelIndex>::Iterator it = idxs.begin(); it != idxs.end(); ++it) {
        QTreeWidgetItem* item = itemFromIndex(*it);
        if (item == targetitem)
            continue;
        if (item->parent() == targetitem)
            continue;
        items.push_back(item);
    }

    if (items.isEmpty())
        return; // nothing needs to be done

    if (targetitem->type() == TreeWidget::ObjectType) {
        // add object to group
        App::DocumentObject* grp = static_cast<DocumentObjectItem*>(targetitem)
            ->object()->getObject();
        if (!grp->getTypeId().isDerivedFrom(App::DocumentObjectGroup::getClassTypeId()))
            return; // no group object

        // Open command
        App::Document* doc = grp->getDocument();
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Move object");
        for (QList<QTreeWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
            // get document object
            App::DocumentObject* obj = static_cast<DocumentObjectItem*>(*it)
                ->object()->getObject();
            App::DocumentObjectGroup* par = App::DocumentObjectGroup
                ::getGroupOfObject(obj);
            if (par) {
                // allow an object to be in one group only
                QString cmd;
                cmd = QString::fromAscii("App.getDocument(\"%1\").getObject(\"%2\").removeObject("
                                  "App.getDocument(\"%1\").getObject(\"%3\"))")
                                  .arg(QString::fromAscii(doc->getName()))
                                  .arg(QString::fromAscii(par->getNameInDocument()))
                                  .arg(QString::fromAscii(obj->getNameInDocument()));
                Gui::Application::Instance->runPythonCode(cmd.toUtf8());
            }

            // build Python command for execution
            QString cmd;
            cmd = QString::fromAscii("App.getDocument(\"%1\").getObject(\"%2\").addObject("
                              "App.getDocument(\"%1\").getObject(\"%3\"))")
                              .arg(QString::fromAscii(doc->getName()))
                              .arg(QString::fromAscii(grp->getNameInDocument()))
                              .arg(QString::fromAscii(obj->getNameInDocument()));
            Gui::Application::Instance->runPythonCode(cmd.toUtf8());
        }
        gui->commitCommand();
    }
    else if (targetitem->type() == TreeWidget::DocumentType) {
        // Open command
        App::Document* doc = static_cast<DocumentItem*>(targetitem)->document()->getDocument();
        Gui::Document* gui = Gui::Application::Instance->getDocument(doc);
        gui->openCommand("Move object");
        for (QList<QTreeWidgetItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
            // get document object
            // there must be a group that references this object
            App::DocumentObject* obj = static_cast<DocumentObjectItem*>(*it)
                ->object()->getObject();
            App::DocumentObjectGroup* grp = App::DocumentObjectGroup
                ::getGroupOfObject(obj);
            if (grp) {
                QString cmd = QString::fromAscii("App.getDocument(\"%1\").getObject(\"%2\").removeObject("
                                      "App.getDocument(\"%1\").getObject(\"%3\"))")
                                      .arg(QString::fromAscii(doc->getName()))
                                      .arg(QString::fromAscii(grp->getNameInDocument()))
                                      .arg(QString::fromAscii(obj->getNameInDocument()));
                Gui::Application::Instance->runPythonCode(cmd.toUtf8());
            }
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
    doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, _1));
    doc->signalDeletedObject.connect(boost::bind(&DocumentItem::slotDeleteObject, this, _1));
    doc->signalChangedObject.connect(boost::bind(&DocumentItem::slotChangeObject, this, _1));
    doc->signalRenamedObject.connect(boost::bind(&DocumentItem::slotRenameObject, this, _1));
    doc->signalActivatedObject.connect(boost::bind(&DocumentItem::slotActiveObject, this, _1));
    doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, _1));
    doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, _1));
    doc->signalHighlightObject.connect(boost::bind(&DocumentItem::slotHighlightObject, this, _1,_2,_3));
    doc->signalExpandObject.connect(boost::bind(&DocumentItem::slotExpandObject, this, _1,_2));

    setFlags(Qt::ItemIsEnabled/*|Qt::ItemIsEditable*/);
}

DocumentItem::~DocumentItem()
{
}

void DocumentItem::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    std::string name (v.getObject()->getNameInDocument());
    std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.find(name);
    if (it != ObjectMap.end()) 
        it->second->setBackgroundColor(0,Qt::yellow);
}

void DocumentItem::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    std::string name (v.getObject()->getNameInDocument());
    std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.find(name);
    if (it != ObjectMap.end()) {
        it->second->setData(0, Qt::BackgroundColorRole,QVariant());
    }
}

void DocumentItem::slotNewObject(const Gui::ViewProviderDocumentObject& obj)
{
    std::string displayName = obj.getObject()->Label.getValue();
    std::string objectName = obj.getObject()->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.find(objectName);
    if (it == ObjectMap.end()) {
        // cast to non-const object
        DocumentObjectItem* item = new DocumentObjectItem(
            const_cast<Gui::ViewProviderDocumentObject*>(&obj), this);
        item->setIcon(0, obj.getIcon());
        item->setText(0, QString::fromUtf8(displayName.c_str()));
        ObjectMap[objectName] = item;
    } else {
        Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
    }
}

void DocumentItem::slotDeleteObject(const Gui::ViewProviderDocumentObject& obj)
{
    std::string objectName = obj.getObject()->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.find(objectName);
    if (it != ObjectMap.end()) {
        QTreeWidgetItem* parent = it->second->parent();
        if (it->second->childCount() > 0) {
            QList<QTreeWidgetItem*> children = it->second->takeChildren();
            parent->addChildren(children);
        }

        parent->takeChild(parent->indexOfChild(it->second));
        delete it->second;
        ObjectMap.erase(it);
    }
}

void DocumentItem::slotChangeObject(const Gui::ViewProviderDocumentObject& view)
{
    // As we immediately add a newly created object to the tree we check here which
    // item (this or a DocumentObjectItem) is the parent of the associated item of 'view'
    App::DocumentObject* obj = view.getObject();
    std::string objectName = obj->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.find(objectName);
    if (it != ObjectMap.end()) {
        // use new grouping style
        std::set<QTreeWidgetItem*> children;
        std::vector<App::DocumentObject*> group = view.claimChildren();
        for (std::vector<App::DocumentObject*>::iterator jt = group.begin(); jt != group.end(); ++jt) {
            if (*jt) {
                const char* internalName = (*jt)->getNameInDocument();
                if (internalName) {
                    std::map<std::string, DocumentObjectItem*>::iterator kt = ObjectMap.find(internalName);
                    if (kt != ObjectMap.end()) {
                        children.insert(kt->second);
                        QTreeWidgetItem* parent = kt->second->parent();
                        if (parent && parent != it->second) {
                            if (it->second != kt->second) {
                                int index = parent->indexOfChild(kt->second);
                                parent->takeChild(index);
                                it->second->addChild(kt->second);
                            }
                            else {
                                Base::Console().Warning("Gui::DocumentItem::slotChangedObject(): Object references to itself.\n");
                            }
                        }
                    }
                    else {
                        Base::Console().Warning("Gui::DocumentItem::slotChangedObject(): Cannot reparent unknown object.\n");
                    }
                }
                else {
                    Base::Console().Warning("Gui::DocumentItem::slotChangedObject(): Group references unknown object.\n");
                }
            }
        }
        // move all children which are not part of the group anymore to this item
        int count = it->second->childCount();
        for (int i=0; i < count; i++) {
            QTreeWidgetItem* child = it->second->child(i);
            if (children.find(child) == children.end()) {
                it->second->takeChild(i);
                this->addChild(child);
            }
        }

        // set the text label
        std::string displayName = obj->Label.getValue();
        it->second->setText(0, QString::fromUtf8(displayName.c_str()));
    }
    else {
        Base::Console().Warning("Gui::DocumentItem::slotChangedObject(): Cannot change unknown object.\n");
    }
}

void DocumentItem::slotRenameObject(const Gui::ViewProviderDocumentObject& obj)
{
    for (std::map<std::string,DocumentObjectItem*>::iterator it = ObjectMap.begin(); it != ObjectMap.end(); ++it) {
        if (it->second->object() == &obj) {
            DocumentObjectItem* item = it->second;
            ObjectMap.erase(it);
            std::string objectName = obj.getObject()->getNameInDocument();
            ObjectMap[objectName] = item;
            return;
        }
    }

    // no such object found
    Base::Console().Warning("DocumentItem::slotRenamedObject: Cannot rename unknown object.\n");
}

void DocumentItem::slotActiveObject(const Gui::ViewProviderDocumentObject& obj)
{
    std::string objectName = obj.getObject()->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator jt = ObjectMap.find(objectName);
    if (jt == ObjectMap.end())
        return; // signal is emitted before the item gets created
    for (std::map<std::string, DocumentObjectItem*>::iterator it = ObjectMap.begin();
         it != ObjectMap.end(); ++it)
    {
        QFont f = it->second->font(0);
        f.setBold(it == jt);
        it->second->setFont(0,f);
    }
}

void DocumentItem::slotHighlightObject (const Gui::ViewProviderDocumentObject& obj,const Gui::HighlightMode& high,bool set)
{
    std::string objectName = obj.getObject()->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator jt = ObjectMap.find(objectName);
    if (jt == ObjectMap.end())
        return; // signal is emitted before the item gets created

    QFont f = jt->second->font(0);
    switch (high) {
    case Gui::Bold: f.setBold(set);             break;
    case Gui::Italic: f.setItalic(set);         break;
    case Gui::Underlined: f.setUnderline(set);  break;
    case Gui::Overlined: f.setOverline(set);    break;
    case Gui::Blue:
        if(set)
            jt->second->setBackgroundColor(0,QColor(200,200,255));
        else
            jt->second->setData(0, Qt::BackgroundColorRole,QVariant());
        break;
    default:
        break;
    }

    jt->second->setFont(0,f);
}

void DocumentItem::slotExpandObject (const Gui::ViewProviderDocumentObject& obj,const Gui::TreeItemMode& mode)
{
    std::string objectName = obj.getObject()->getNameInDocument();
    std::map<std::string, DocumentObjectItem*>::iterator jt = ObjectMap.find(objectName);
    if (jt == ObjectMap.end())
        return; // signal is emitted before the item gets created

    switch (mode) {
    case Gui::Expand:
        jt->second->setExpanded(true);
        break;
    case Gui::Collapse:
        jt->second->setExpanded(false);
        break;
    case Gui::Toggle:
        if (jt->second->isExpanded())
            jt->second->setExpanded(false);
        else
            jt->second->setExpanded(true);
        break;

    default:
        // not defined enum
        assert(0);
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
//    pos = ObjectMap.find(Obj->getNameInDocument());
//    if (pos != ObjectMap.end()) {
//        QFont f = pos->second->font(0);
//        f.setUnderline(mark);
//        pos->second->setFont(0,f);
//    }
//}

void DocumentItem::testStatus(void)
{
    for (std::map<std::string,DocumentObjectItem*>::iterator pos = ObjectMap.begin();pos!=ObjectMap.end();++pos) {
        pos->second->testStatus();
    }
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
    std::map<std::string,DocumentObjectItem*>::iterator pos;
    pos = ObjectMap.find(name);
    if (pos != ObjectMap.end()) {
        //pos->second->setData(0, Qt::TextColorRole, QVariant(Qt::red));
        //treeWidget()->setItemSelected(pos->second, select);
    }
}

void DocumentItem::setObjectSelected(const char* name, bool select)
{
    std::map<std::string,DocumentObjectItem*>::iterator pos;
    pos = ObjectMap.find(name);
    if (pos != ObjectMap.end()) {
        treeWidget()->setItemSelected(pos->second, select);
    }
}

void DocumentItem::clearSelection(void)
{
    // Block signals here otherwise we get a recursion and quadratic runtime
    bool ok = treeWidget()->blockSignals(true);
    for (std::map<std::string,DocumentObjectItem*>::iterator pos = ObjectMap.begin();pos!=ObjectMap.end();++pos) {
        pos->second->setSelected(false);
    }
    treeWidget()->blockSignals(ok);
}

void DocumentItem::updateSelection(void)
{
    std::vector<App::DocumentObject*> sel;
    for (std::map<std::string,DocumentObjectItem*>::iterator pos = ObjectMap.begin();pos!=ObjectMap.end();++pos) {
        if (treeWidget()->isItemSelected(pos->second)) {
            sel.push_back(pos->second->object()->getObject());
        }
    }

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
    for (std::map<std::string,DocumentObjectItem*>::iterator it = ObjectMap.begin(); it != ObjectMap.end(); ++it) {
        items.push_back(it->second);
    }
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
                                       QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent, TreeWidget::ObjectType), previousStatus(-1), viewObject(pcViewProvider)
{
    setFlags(flags()|Qt::ItemIsEditable);
    // Setup connections
    connectIcon = pcViewProvider->signalChangeIcon.connect(boost::bind(&DocumentObjectItem::slotChangeIcon, this));
    connectTool = pcViewProvider->signalChangeToolTip.connect(boost::bind(&DocumentObjectItem::slotChangeToolTip, this, _1));
    connectStat = pcViewProvider->signalChangeStatusTip.connect(boost::bind(&DocumentObjectItem::slotChangeStatusTip, this, _1));
}

DocumentObjectItem::~DocumentObjectItem()
{
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
        ((pObject->isError()          ? 1 : 0) << 2) |
        ((pObject->mustExecute() == 1 ? 1 : 0) << 1) |
        (viewObject->isShow()         ? 1 : 0);
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
        opt.initFrom(this->treeWidget());
#if QT_VERSION >= 0x040200
        this->setForeground(0, opt.palette.color(QPalette::Disabled,QPalette::Dark));
#else
        this->setTextColor(0, opt.palette.color(QPalette::Disabled,QPalette::Dark));
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

    QString info = QString::fromAscii(Obj->getStatusString());
    if ( Obj->mustExecute() == 1 )
        info += QString::fromAscii(" (but must be executed)");
    getMainWindow()->showMessage( info );
   
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
        viewObject->getObject()->Label.setValue((const char*)label.toUtf8());
    }
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

