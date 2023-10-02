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
# include <QFont>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include "DocumentModel.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;
namespace sp = std::placeholders;

namespace Gui {
    // forward declaration
    class ViewProviderIndex;

    // Base class
    class DocumentModelIndex : public Base::BaseClass
    {
        TYPESYSTEM_HEADER_WITH_OVERRIDE();

    public:
        ~DocumentModelIndex() override
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
        {
            Q_UNUSED(role);
            return {};
        }
        virtual bool setData (const QVariant & value, int role)
        {
            Q_UNUSED(value);
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
        DocumentModelIndex() = default;
        DocumentModelIndex *parentItem{nullptr};
        QList<DocumentModelIndex*> childItems;
    };

    // ------------------------------------------------------------------------

    // Root node
    class ApplicationIndex : public DocumentModelIndex
    {
        TYPESYSTEM_HEADER_WITH_OVERRIDE();

    public:
        ApplicationIndex() = default;
        int findChild(const Gui::Document& d) const;
        Qt::ItemFlags flags() const override;
        QVariant data(int role) const override;
    };

    // ------------------------------------------------------------------------

    // Document nodes
    class DocumentIndex : public DocumentModelIndex
    {
        friend class ViewProviderIndex;
        TYPESYSTEM_HEADER_WITH_OVERRIDE();
        static QIcon* documentIcon;
        using IndexSet = boost::unordered_set<ViewProviderIndex*>;
        std::map<const ViewProviderDocumentObject*, IndexSet> vp_nodes;
        void addToDocument(ViewProviderIndex*);
        void removeFromDocument(ViewProviderIndex*);

    public:
        const Gui::Document& d;
        explicit DocumentIndex(const Gui::Document& d) : d(d)
        {
            if (!documentIcon)
                documentIcon = new QIcon(Gui::BitmapFactory().pixmap("Document"));
        }
        ~DocumentIndex() override
        {
            qDeleteAll(childItems); childItems.clear();
        }
        ViewProviderIndex* cloneViewProvider(const ViewProviderDocumentObject&) const;
        int rowOfViewProvider(const ViewProviderDocumentObject&) const;
        void findViewProviders(const ViewProviderDocumentObject&, QList<ViewProviderIndex*>&) const;
        QVariant data(int role) const override;
    };

    // ------------------------------------------------------------------------

    // Object nodes
    class ViewProviderIndex : public DocumentModelIndex
    {
        TYPESYSTEM_HEADER_WITH_OVERRIDE();

    public:
        const Gui::ViewProviderDocumentObject& v;
        ViewProviderIndex(const Gui::ViewProviderDocumentObject& v, DocumentIndex* d);
        ~ViewProviderIndex() override;
        ViewProviderIndex* clone() const;
        void findViewProviders(const ViewProviderDocumentObject&, QList<ViewProviderIndex*>&) const;
        QVariant data(int role) const override;

    private:
        DocumentIndex* d;
    };

    // ------------------------------------------------------------------------

    int ApplicationIndex::findChild(const Gui::Document& d) const
    {
        int child=0;
        QList<DocumentModelIndex*>::const_iterator it;
        for (it = childItems.begin(); it != childItems.end(); ++it, ++child) {
            auto doc = static_cast<DocumentIndex*>(*it);
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
        return {};
    }

    // ------------------------------------------------------------------------

    QIcon* DocumentIndex::documentIcon = nullptr;

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
            if (!it->second.empty()) {
                v = it->second.begin();
                if (*v)
                    return (*v)->clone();
            }
        }
        return new ViewProviderIndex(vp, const_cast<DocumentIndex*>(this));
    }

    void DocumentIndex::findViewProviders(const ViewProviderDocumentObject& vp,
        QList<ViewProviderIndex*>& index) const
    {
        QList<DocumentModelIndex*>::const_iterator it;
        for (it = childItems.begin(); it != childItems.end(); ++it) {
            auto v = static_cast<ViewProviderIndex*>(*it);
            v->findViewProviders(vp, index);
        }
    }

    int DocumentIndex::rowOfViewProvider(const ViewProviderDocumentObject& vp) const
    {
        QList<DocumentModelIndex*>::const_iterator it;
        int index=0;
        for (it = childItems.begin(); it != childItems.end(); ++it, ++index) {
            auto v = static_cast<ViewProviderIndex*>(*it);
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
            return static_cast<QVariant>(font);
        }

        return {};
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
        auto copy = new ViewProviderIndex(this->v, this->d);
        for (const auto & childItem : childItems) {
            ViewProviderIndex* c = static_cast<ViewProviderIndex*>(childItem)->clone();
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
            auto v = static_cast<ViewProviderIndex*>(*it);
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
            return static_cast<QVariant>(font);
        }

        return {};
    }

    // ------------------------------------------------------------------------

    TYPESYSTEM_SOURCE_ABSTRACT(Gui::DocumentModelIndex, Base::BaseClass)
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::ApplicationIndex,Gui::DocumentModelIndex)
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::DocumentIndex, Gui::DocumentModelIndex)
    TYPESYSTEM_SOURCE_ABSTRACT(Gui::ViewProviderIndex, Gui::DocumentModelIndex)

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

    //NOLINTBEGIN
    // Setup connections
    Application::Instance->signalNewDocument.connect(std::bind(&DocumentModel::slotNewDocument, this, sp::_1));
    Application::Instance->signalDeleteDocument.connect(std::bind(&DocumentModel::slotDeleteDocument, this, sp::_1));
    Application::Instance->signalRenameDocument.connect(std::bind(&DocumentModel::slotRenameDocument, this, sp::_1));
    Application::Instance->signalActiveDocument.connect(std::bind(&DocumentModel::slotActiveDocument, this, sp::_1));
    Application::Instance->signalRelabelDocument.connect(std::bind(&DocumentModel::slotRelabelDocument, this, sp::_1));
    //NOLINTEND
}

DocumentModel::~DocumentModel()
{
    delete d; d = nullptr;
}

void DocumentModel::slotNewDocument(const Gui::Document& Doc)
{
    //NOLINTBEGIN
    Doc.signalNewObject.connect(std::bind(&DocumentModel::slotNewObject, this, sp::_1));
    Doc.signalDeletedObject.connect(std::bind(&DocumentModel::slotDeleteObject, this, sp::_1));
    Doc.signalChangedObject.connect(std::bind(&DocumentModel::slotChangeObject, this, sp::_1, sp::_2));
    Doc.signalRelabelObject.connect(std::bind(&DocumentModel::slotRenameObject, this, sp::_1));
    Doc.signalActivatedObject.connect(std::bind(&DocumentModel::slotActiveObject, this, sp::_1));
    Doc.signalInEdit.connect(std::bind(&DocumentModel::slotInEdit, this, sp::_1));
    Doc.signalResetEdit.connect(std::bind(&DocumentModel::slotResetEdit, this, sp::_1));
    //NOLINTEND

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
    Q_UNUSED(Doc);
    // do nothing here
}

void DocumentModel::slotRelabelDocument(const Gui::Document& Doc)
{
    int row = d->rootItem->findChild(Doc);
    if (row > -1) {
        QModelIndex parent = createIndex(0,0,d->rootItem);
        QModelIndex item = index (row, 0, parent);
        Q_EMIT dataChanged(item, item);
    }
}

void DocumentModel::slotActiveDocument(const Gui::Document& /*Doc*/)
{
    // don't know which was the previous active document, so check simply all
    QModelIndex parent = createIndex(0,0,d->rootItem);
    QModelIndex top = index (0, 0, parent);
    QModelIndex bottom = index (d->rootItem->childCount()-1, 0, parent);
    Q_EMIT dataChanged(top, bottom);
}

void DocumentModel::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    Q_UNUSED(v);
}

void DocumentModel::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    Q_UNUSED(v);
}

void DocumentModel::slotNewObject(const Gui::ViewProviderDocumentObject& obj)
{
    App::Document* doc = obj.getObject()->getDocument();
    Gui::Document* gdc = Application::Instance->getDocument(doc);
    int row = d->rootItem->findChild(*gdc);
    if (row > -1) {
        auto index = static_cast<DocumentIndex*>(d->rootItem->child(row));
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
        auto doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
        QList<ViewProviderIndex*> views;
        doc_index->findViewProviders(obj, views);
        for (auto & view : views) {
            DocumentModelIndex* parentitem = view->parent();
            QModelIndex parent = createIndex(doc_index->row(), 0, parentitem);
            int row = view->row();
            beginRemoveRows(parent, row, row);
            parentitem->removeChild(row);
            delete view;
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
            auto doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
            QList<ViewProviderIndex*> views;
            doc_index->findViewProviders(obj, views);
            for (const auto & view : qAsConst(views)) {
                DocumentModelIndex* parentitem = view->parent();
                QModelIndex parent = createIndex(0,0,parentitem);
                int row = view->row();
                QModelIndex item = index (row, 0, parent);
                Q_EMIT dataChanged(item, item);
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
            auto doc_index = static_cast<DocumentIndex*>(d->rootItem->child(row));
            for (const auto & view : views) {
                int row = doc_index->rowOfViewProvider(*view);
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
            for (const auto & it : qAsConst(obj_index)) {
                QModelIndex parent = createIndex(it->row(),0,it);
                int count_obj = it->childCount();
                beginRemoveRows(parent, 0, count_obj);
                // remove all children but do not yet delete them
                QList<DocumentModelIndex*> items = it->removeAll();
                endRemoveRows();

                beginInsertRows(parent, 0, (int)views.size());
                for (const auto & view : views) {
                    ViewProviderIndex* clone = doc_index->cloneViewProvider(*view);
                    it->appendChild(clone);
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
    Q_UNUSED(obj);
    // renaming of objects not supported at the moment
}

void DocumentModel::slotActiveObject(const Gui::ViewProviderDocumentObject& obj)
{
    Q_UNUSED(obj);
    // do nothing here because this is automatically done by calling
    // ViewProviderIndex::data()
}

const Document* DocumentModel::getDocument(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;
    Base::BaseClass* item = nullptr;
    item = static_cast<Base::BaseClass*>(index.internalPointer());
    if (item->getTypeId() == DocumentIndex::getClassTypeId()) {
        const Gui::Document& d = static_cast<DocumentIndex*>(item)->d;
        return (&d);
    }

    return nullptr;
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
    for (const auto & child : childs) {
        ViewProvider* view = doc.getViewProvider(child);
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
        return {};
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
        return {};
    return static_cast<DocumentModelIndex*>(index.internalPointer())->flags();
}

QModelIndex DocumentModel::index (int row, int column, const QModelIndex & parent) const
{
    DocumentModelIndex* item = nullptr;
    if (!parent.isValid())
        item = d->rootItem;
    else
        item = static_cast<DocumentModelIndex*>(parent.internalPointer())->child(row);
    if (!item)
        return {};
    return createIndex(row, column, item);
}

QModelIndex DocumentModel::parent (const QModelIndex & index) const
{
    if (!index.isValid() || index.internalPointer() == d->rootItem)
        return {};
    DocumentModelIndex* item = nullptr;
    item = static_cast<DocumentModelIndex*>(index.internalPointer());
    DocumentModelIndex* parent = item->parent();
    return createIndex(parent->row(), 0, parent);
}

int DocumentModel::rowCount (const QModelIndex & parent) const
{
    if (!parent.isValid())
        return 1; // the root item
    DocumentModelIndex* item = nullptr;
    item = static_cast<DocumentModelIndex*>(parent.internalPointer());
    return item->childCount();
}

QVariant DocumentModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return {};
        return tr("Labels & Attributes");
    }

    return {};
}

bool DocumentModel::setHeaderData (int, Qt::Orientation, const QVariant &, int)
{
    return false;
}
