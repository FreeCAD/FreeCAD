/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <algorithm>
# include <boost/signals.hpp>
# include <boost/bind.hpp>
#endif

#include <boost/unordered_set.hpp>

#include "DocumentModel.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "ViewProviderDocumentObject.h"
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

using namespace Gui;

namespace Gui {
    // forward declaration
    class ViewProviderIndex;

    // Base class
    class DocumentModelIndex : public Base::BaseClass
    {
        TYPESYSTEM_HEADER();

    public:
        virtual ~DocumentModelIndex()
        { qDeleteAll(childItems); }

        void setParent(DocumentModelIndex* parent)
        { parentItem = parent; }
        DocumentModelIndex *parent() const
        { return parentItem; }
        void appendChild(DocumentModelIndex *child)
        { childItems.append(child); child->setParent(this); }
        void removeChild(int row)
        { childItems.removeAt(row); }
        QList<DocumentModelIndex*> removeAll()
        {
            QList<DocumentModelIndex*> list = childItems;
            childItems.clear();
            return list;
        }
        DocumentModelIndex *child(int row)
        { return childItems.value(row); }
        int row() const
        {
            if (parentItem)
                return parentItem->childItems.indexOf
                    (const_cast<DocumentModelIndex*>(this));
            return 0;
        }
        int childCount() const
        { return childItems.count(); }
        virtual QVariant data(int role) const
        { return QVariant(); }
        virtual bool setData (const QVariant & value, int role)
        {
            if (role == Qt::EditRole) {
                return true;
            }

            return true;
        }
        virtual Qt::ItemFlags flags() const
        {
            return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
        }

    protected:
        void reset()
        { qDeleteAll(childItems); childItems.clear(); }

    protected:
        DocumentModelIndex() : parentItem(0) {}
        DocumentModelIndex *parentItem;
        QList<DocumentModelIndex*> childItems;
    };

    // ------------------------------------------------------------------------

    // Root node
    class ApplicationIndex : public DocumentModelIndex
    {
        TYPESYSTEM_HEADER();

    public:
        ApplicationIndex(){}
        int findChild(const Gui::Document& d) const;
        Qt::ItemFlags flags() const;
        QVariant data(int role) const;
    };

    // ------------------------------------------------------------------------

    // Document nodes
    class DocumentIndex : public DocumentModelIndex
    {
        friend class ViewProviderIndex;
        TYPESYSTEM_HEADER();
        static QIcon* documentIcon;
        typedef boost::unordered_set<ViewProviderIndex*> IndexSet;
        std::map<const ViewProviderDocumentObject*, IndexSet> vp_nodes;
        void addToDocument(ViewProviderIndex*);
        void removeFromDocument(ViewProviderIndex*);

    public:
        const Gui::Document& d;
        DocumentIndex(const Gui::Document& d) : d(d)
        {
            if (!documentIcon)
                documentIcon = new QIcon(Gui::BitmapFactory().pixmap("Document"));
        }
        ~DocumentIndex()
        {
            qDeleteAll(childItems); childItems.clear();
        }
        ViewProviderIndex* cloneViewProvider(const ViewProviderDocumentObject&) const;
        int rowOfViewProvider(const ViewProviderDocumentObject&) const;
        void findViewProviders(const ViewProviderDocumentObject&, QList<ViewProviderIndex*>&) const;
        QVariant data(int role) const;
    };

    // ------------------------------------------------------------------------

    // Object nodes
    class ViewProviderIndex : public DocumentModelIndex
    {
        TYPESYSTEM_HEADER();

    public:
        const Gui::ViewProviderDocumentObject& v;
        ViewProviderIndex(const Gui::ViewProviderDocumentObject& v, DocumentIndex* d);
        ~ViewProviderIndex();
        ViewProviderIndex* clone() const;
        void findViewProviders(const ViewProviderDocumentObject&, QList<ViewProviderIndex*>&) const;
        QVariant data(int role) const;

    private:
        DocumentIndex* d;
    };

    // ------------------------------------------------------------------------

    int ApplicationIndex::findChild(const Gui::Document& d) const
    {
        int child=0;
        QList<DocumentModelIndex*>::const_iterator it;
        for (it = childItems.begin(); it != childItems.end(); ++it, ++child) {
            DocumentIndex* doc = static_cast<DocumentIndex*>(*it);
            if (&doc->d == &d)
                return child;
        }

        return -1;
    }

    Qt::ItemFlags ApplicationIndex::flags() const
    {
        return Qt::ItemIsEnabled;
    }

    QVariant ApplicationIndex::data(int role) const
    {
        if (role == Qt::DecorationRole) {
            return qApp->windowIcon();
        }
        else if (role == Qt::DisplayRole) {
            return DocumentModel::tr("Application");
        }
        return QVariant();
    }

    // ------------------------------------------------------------------------

    QIcon* DocumentIndex::documentIcon = 0;

    void DocumentIndex::addToDocument(ViewProviderIndex* vp)
    {
        vp_nodes[&vp->v].insert(vp);
    }

    void DocumentIndex::removeFromDocument(ViewProviderIndex* vp)
    {
        vp_nodes[&vp->v].erase(vp);
    }

    ViewProviderIndex*
    DocumentIndex::cloneViewProvider(const ViewProviderDocumentObject& vp) const
    {
        std::map<const ViewProviderDocumentObject*, boost::unordered_set<ViewProviderIndex*> >::const_iterator it;
        it = vp_nodes.find(&vp);
        if (it != vp_nodes.end()) {
            boost::unordered_set<ViewProviderIndex*>::const_iterator v;
            v = it->second.begin();
            return (*v)->clone();
        }
        return new ViewProviderIndex(vp, const_cast<DocumentIndex*>(this));
    }

    void DocumentIndex::findViewProviders(const ViewProviderDocumentObject& vp,
        QList<ViewProviderIndex*>& index) const
    {
        QList<DocumentModelIndex*>::const_iterator it;
        for (it = childItems.begin(); it != childItems.end(); ++it) {
            ViewProviderIndex* v = static_cast<ViewProviderIndex*>(*it);
            v->findViewProviders(vp, index);
        }
    }

    int DocumentIndex::rowOfViewProvider(const ViewProviderDocumentObject& vp) const
    {
        QList<DocumentModelIndex*>::const_iterator it;
        int index=0;
        for (it = childItems.begin(); it != childItems.end(); ++it, ++index) {
            ViewProviderIndex* v = static_cast<ViewProviderIndex*>(*it);
            if (&v->v == &vp)
                return index;
        }

        return -1;
    }

    QVariant DocumentIndex::data(int role) const
    {
        if (role == Qt::DecorationRole) {
            return *documentIcon;
        }
        else if (role == Qt::DisplayRole) {
            App::Document* doc = d.getDocument();
            return QString::fromUtf8(doc->Label.getValue());
        }
        else if (role == Qt::FontRole) {
            Document* doc = Application::Instance->activeDocument();
            QFont font;
            font.setBold(doc==&d);
            QVariant variant;
            variant.setValue<QFont>(font);
            return variant;
        }

        return QVariant();
    }

    // ------------------------------------------------------------------------

    ViewProviderIndex::ViewProviderIndex(const Gui::ViewProviderDocumentObject& v, DocumentIndex* d)
        : v(v),d(d)
    {
        if (d) d->addToDocument(this);
    }

    ViewProviderIndex::~ViewProviderIndex()
    {
        if (d) d->removeFromDocument(this);
    }

    ViewProviderIndex* ViewProviderIndex::clone() const
    {
        ViewProviderIndex* copy = new ViewProviderIndex(this->v, this->d);
        for (QList<DocumentModelIndex*>::const_iterator it = childItems.begin(); it != childItems.end(); ++it) {
            ViewProviderIndex* c = static_cast<ViewProviderIndex*>(*it)->clone();
            copy->appendChild(c);
        }
        return copy; 
    }

    void ViewProviderIndex::findViewProviders(const ViewProviderDocumentObject& vp,
                                              QList<ViewProviderIndex*>& index) const
    {
        if (&this->v == &vp)
            index.push_back(const_cast<ViewProviderIndex*>(this));
        QList<DocumentModelIndex*>::const_iterator it;
        for (it = childItems.begin(); it != childItems.end(); ++it) {
            ViewProviderIndex* v = static_cast<ViewProviderIndex*>(*it);
            v->findViewProviders(vp, index);
        }
    }

    QVariant ViewProviderIndex::data(int role) const
    {
        if (role == Qt::DecorationRole) {
            return v.getIcon();
        }
        else if (role == Qt::DisplayRole) {
            App::DocumentObject* obj = v.getObject();
            return QString::fromUtf8(obj->Label.getValue());
        }
        else if (role == Qt::FontRole) {
            App::DocumentObject* obj = v.getObject();
            App::DocumentObject* act = obj->getDocument()->getActiveObject();
            QFont font;
            font.setBold(obj==act);
            QVariant variant;
            variant.setValue<QFont>(font);
            return variant;
        }

        return QVariant();
    }

    // ------------------------------------------------------------------------

    TYPESYSTEM_SOURCE_ABSTRACT(Gui::DocumentModelIndex, Base::BaseClass);
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::ApplicationIndex,Gui::DocumentModelIndex);
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::DocumentIndex, Gui::DocumentModelIndex);
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::ViewProviderIndex, Gui::DocumentModelIndex);

    struct DocumentModelP
    {
        DocumentModelP()
        { rootItem = new ApplicationIndex(); }
        ~DocumentModelP()
        { delete rootItem; }
        ApplicationIndex *rootItem;
    };
}

// -----------------------------------------------------------------

DocumentModel::DocumentModel(QObject* parent)
    : QAbstractItemModel(parent), d(new DocumentModelP)
{
    static bool inittype = false;
    if (!inittype) {
        inittype = true;
        DocumentModelIndex  ::init();
        ApplicationIndex    ::init();
        DocumentIndex       ::init();
        ViewProviderIndex   ::init();
    }

    // Setup connections
    Application::Instance->signalNewDocument.connect(boost::bind(&DocumentModel::slotNewDocument, this, _1));
    Application::Instance->signalDeleteDocument.connect(boost::bind(&DocumentModel::slotDeleteDocument, this, _1));
    Application::Instance->signalRenameDocument.connect(boost::bind(&DocumentModel::slotRenameDocument, this, _1));
    Application::Instance->signalActiveDocument.connect(boost::bind(&DocumentModel::slotActiveDocument, this, _1));
    Application::Instance->signalRelabelDocument.connect(boost::bind(&DocumentModel::slotRelabelDocument, this, _1));
}

DocumentModel::~DocumentModel()
{
    delete d; d = 0;
}

void DocumentModel::slotNewDocument(const Gui::Document& Doc)
{
    Doc.signalNewObject.connect(boost::bind(&DocumentModel::slotNewObject, this, _1));
    Doc.signalDeletedObject.connect(boost::bind(&DocumentModel::slotDeleteObject, this, _1));
    Doc.signalChangedObject.connect(boost::bind(&DocumentModel::slotChangeObject, this, _1, _2));
    Doc.signalRenamedObject.connect(boost::bind(&DocumentModel::slotRenameObject, this, _1));
    Doc.signalActivatedObject.connect(boost::bind(&DocumentModel::slotActiveObject, this, _1));
    Doc.signalInEdit.connect(boost::bind(&DocumentModel::slotInEdit, this, _1));
    Doc.signalResetEdit.connect(boost::bind(&DocumentModel::slotResetEdit, this, _1));

    QModelIndex parent = createIndex(0,0,d->rootItem);
    int count_docs = d->rootItem->childCount();
    beginInsertRows(parent, count_docs, count_docs);
    d->rootItem->appendChild(new DocumentIndex(Doc));
    endInsertRows();
}

void DocumentModel::slotDeleteDocument(const Gui::Document& Doc)
{
    int row = d->rootItem->findChild(Doc);
    if (row > -1) {
        QModelIndex parent = createIndex(0,0,d->rootItem);
        beginRemoveRows(parent, row, row);
        DocumentModelIndex* item = d->rootItem->child(row);
        d->rootItem->removeChild(row);
        delete item;
        endRemoveRows();
    }
}

void DocumentModel::slotRenameDocument(const Gui::Document& Doc)
{
    // do nothing here
}

void DocumentModel::slotRelabelDocument(const Gui::Document& Doc)
{
    int row = d->rootItem->findChild(Doc);
    if (row > -1) {
        QModelIndex parent = createIndex(0,0,d->rootItem);
        QModelIndex item = index (row, 0, parent);
        dataChanged(item, item);
    }
}

void DocumentModel::slotActiveDocument(const Gui::Document& /*Doc*/)
{
    // don't know which was the previous active document, so check simply all
    QModelIndex parent = createIndex(0,0,d->rootItem);
    QModelIndex top = index (0, 0, parent);
    QModelIndex bottom = index (d->rootItem->childCount()-1, 0, parent);
    dataChanged(top, bottom);
}

void DocumentModel::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
}

void DocumentModel::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
}

void DocumentModel::slotNewObject(const Gui::ViewProviderDocumentObject& obj)
{
    App::Document* doc = obj.getObject()->getDocument();
    Gui::Document* gdc = Application::Instance->getDocument(doc);
    int row = d->rootItem->findChild(*gdc);
    if (row > -1) {
        DocumentIndex* index = static_cast<DocumentIndex*>(d->rootItem->child(row));
        QModelIndex parent = createIndex(index->row(),0,index);
        int count_obj = index->childCount();
        beginInsertRows(parent, count_obj, count_obj);
        index->appendChild(new ViewProviderIndex(obj, index));
        endInsertRows();
    }
}

void DocumentModel::slotDeleteObject(const Gui::ViewProviderDocumentObject& obj)
{
    App::Document* doc = obj.getObject()->getDocument();
    Gui::Document* gdc = Application::Instance->getDocument(doc);
    int row = d->rootItem->findChild(*gdc);
    if (row > -1) {
        DocumentIndex* doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
        QList<ViewProviderIndex*> views;
        doc_index->findViewProviders(obj, views);
        for (QList<ViewProviderIndex*>::iterator it = views.begin(); it != views.end(); ++it) {
            DocumentModelIndex* parentitem = (*it)->parent();
            QModelIndex parent = createIndex(doc_index->row(), 0, parentitem);
            int row = (*it)->row();
            beginRemoveRows(parent, row, row);
            parentitem->removeChild(row);
            delete *it;
            endRemoveRows();
        }
    }
}

void DocumentModel::slotChangeObject(const Gui::ViewProviderDocumentObject& obj, const App::Property& Prop)
{
    App::DocumentObject* fea = obj.getObject();
    if (&fea->Label == &Prop) {
        App::Document* doc = fea->getDocument();
        Gui::Document* gdc = Application::Instance->getDocument(doc);
        int row = d->rootItem->findChild(*gdc);
        if (row > -1) {
            DocumentIndex* doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
            QList<ViewProviderIndex*> views;
            doc_index->findViewProviders(obj, views);
            for (QList<ViewProviderIndex*>::iterator it = views.begin(); it != views.end(); ++it) {
                DocumentModelIndex* parentitem = (*it)->parent();
                QModelIndex parent = createIndex(0,0,parentitem);
                int row = (*it)->row();
                QModelIndex item = index (row, 0, parent);
                dataChanged(item, item);
            }
        }
    }
    else if (isPropertyLink(Prop)) {
        App::Document* doc = fea->getDocument();
        Gui::Document* gdc = Application::Instance->getDocument(doc);
        std::vector<ViewProviderDocumentObject*> views = claimChildren(*gdc, obj);

        int row = d->rootItem->findChild(*gdc);
        if (row > -1) {
            QList<DocumentModelIndex*> del_items;
            DocumentIndex* doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
            for (std::vector<ViewProviderDocumentObject*>::iterator vp = views.begin(); vp != views.end(); ++vp) {
                int row = doc_index->rowOfViewProvider(**vp);
                // is it a top-level child in the document
                if (row >= 0) {
                    DocumentModelIndex* child = doc_index->child(row);
                    del_items.push_back(child);
                    QModelIndex parent = createIndex(doc_index->row(), 0, doc_index);
                    beginRemoveRows(parent, row, row);
                    doc_index->removeChild(row);
                    endRemoveRows();
                }
            }

            // get all occurrences of the view provider in the tree structure
            QList<ViewProviderIndex*> obj_index;
            doc_index->findViewProviders(obj, obj_index);
            for (QList<ViewProviderIndex*>::iterator it = obj_index.begin(); it != obj_index.end(); ++it) {
                QModelIndex parent = createIndex((*it)->row(),0,*it);
                int count_obj = (*it)->childCount();
                beginRemoveRows(parent, 0, count_obj);
                // remove all children but do not yet delete them
                QList<DocumentModelIndex*> items = (*it)->removeAll();
                endRemoveRows();

                beginInsertRows(parent, 0, (int)views.size());
                for (std::vector<ViewProviderDocumentObject*>::iterator vp = views.begin(); vp != views.end(); ++vp) {
                    ViewProviderIndex* clone = doc_index->cloneViewProvider(**vp);
                    (*it)->appendChild(clone);
                }
                endInsertRows();

                del_items.append(items);
            }

            qDeleteAll(del_items);
        }
    }
}

void DocumentModel::slotRenameObject(const Gui::ViewProviderDocumentObject& obj)
{
    // renaming of objects not supported at the moment
}

void DocumentModel::slotActiveObject(const Gui::ViewProviderDocumentObject& obj)
{
    // do nothing here because this is automatically done by calling
    // ViewProviderIndex::data()
}

const Document* DocumentModel::getDocument(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;
    Base::BaseClass* item = 0;
    item = static_cast<Base::BaseClass*>(index.internalPointer());
    if (item->getTypeId() == DocumentIndex::getClassTypeId()) {
        const Gui::Document& d = static_cast<DocumentIndex*>(item)->d;
        return (&d);
    }

    return 0;
}

bool DocumentModel::isPropertyLink(const App::Property& prop) const
{
    if (prop.isDerivedFrom(App::PropertyLink::getClassTypeId()))
        return true;
    if (prop.isDerivedFrom(App::PropertyLinkSub::getClassTypeId()))
        return true;
    if (prop.isDerivedFrom(App::PropertyLinkList::getClassTypeId()))
        return true;
    if (prop.isDerivedFrom(App::PropertyLinkSubList::getClassTypeId()))
        return true;
    return false;
}

std::vector<ViewProviderDocumentObject*>
DocumentModel::claimChildren(const Gui::Document& doc, const ViewProviderDocumentObject& obj) const
{
    std::vector<ViewProviderDocumentObject*> views;
    std::vector<App::DocumentObject*> childs = obj.claimChildren();
    for (std::vector<App::DocumentObject*>::iterator it = childs.begin(); it != childs.end(); ++it) {
        ViewProvider* view = doc.getViewProvider(*it);
        if (view && view->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
            views.push_back(static_cast<ViewProviderDocumentObject*>(view));
    }

    return views;
}

int DocumentModel::columnCount (const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant DocumentModel::data (const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();
    return static_cast<DocumentModelIndex*>(index.internalPointer())->data(role);
}

bool DocumentModel::setData(const QModelIndex& index, const QVariant & value, int role)
{
    if (!index.isValid())
        return false;
    return static_cast<DocumentModelIndex*>(index.internalPointer())->setData(value, role);
}

Qt::ItemFlags DocumentModel::flags(const QModelIndex &index) const
{
    //if (index.internalPointer() == d->rootItem)
    //    return Qt::ItemIsEnabled;
    //return QAbstractItemModel::flags(index);
    if (!index.isValid())
        return 0;
    return static_cast<DocumentModelIndex*>(index.internalPointer())->flags();
}

QModelIndex DocumentModel::index (int row, int column, const QModelIndex & parent) const
{
    DocumentModelIndex* item = 0;
    if (!parent.isValid())
        item = d->rootItem;
    else
        item = static_cast<DocumentModelIndex*>(parent.internalPointer())->child(row);
    if (!item)
        return QModelIndex();
    return createIndex(row, column, item);
}

QModelIndex DocumentModel::parent (const QModelIndex & index) const
{
    if (!index.isValid() || index.internalPointer() == d->rootItem)
        return QModelIndex();
    DocumentModelIndex* item = 0;
    item = static_cast<DocumentModelIndex*>(index.internalPointer());
    DocumentModelIndex* parent = item->parent();
    return createIndex(parent->row(), 0, parent);
}

int DocumentModel::rowCount (const QModelIndex & parent) const
{
    if (!parent.isValid())
        return 1; // the root item
    DocumentModelIndex* item = 0;
    item = static_cast<DocumentModelIndex*>(parent.internalPointer());
    return item->childCount();
}

QVariant DocumentModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return QVariant();
        return tr("Labels & Attributes");
    }

    return QVariant();
}

bool DocumentModel::setHeaderData (int, Qt::Orientation, const QVariant &, int)
{
    return false;
}
