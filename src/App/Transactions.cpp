/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
# include <cassert>
#endif

#include <atomic>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Writer.h>
using Base::Writer;
#include <Base/Reader.h>
using Base::XMLReader;
#include <Base/Console.h>
#include "Transactions.h"
#include "Property.h"
#include "Document.h"
#include "DocumentObject.h"

FC_LOG_LEVEL_INIT("App",true,true);

using namespace App;
using namespace std;

TYPESYSTEM_SOURCE(App::Transaction, Base::Persistence)

//**************************************************************************
// Construction/Destruction

Transaction::Transaction(int id)
{
    if(!id) id = getNewID();
    transID = id;
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Transaction::~Transaction()
{
    auto &index = _Objects.get<0>();
    for (auto It= index.begin();It!=index.end();++It) {
        if (It->second->status == TransactionObject::New) {
            // If an object has been removed from the document the transaction
            // status is 'New'. The 'pcNameInDocument' member serves as criterion
            // to check whether the object is part of the document or not.
            // Note, it's possible that the transaction status is 'New' while the
            // object is (again) part of the document. This usually happens when
            // a previous removal is undone.
            // Thus, if the object has been removed, i.e. the status is 'New' and
            // is still not part of the document the object must be destroyed not
            // to cause a memory leak. This usually is the case when the removal
            // of an object is not undone or when an addition is undone.

            if (!It->first->isAttachedToDocument()) {
                if (It->first->getTypeId().isDerivedFrom(DocumentObject::getClassTypeId())) {
                    // #0003323: Crash when clearing transaction list
                    // It can happen that when clearing the transaction list several objects
                    // are destroyed with dependencies which can lead to dangling pointers.
                    // When setting the 'Destroy' flag of an object the destructors of link
                    // properties don't ry to remove backlinks, i.e. they don't try to access
                    // possible dangling pointers.
                    // An alternative solution is to call breakDependency inside
                    // Document::_removeObject. Make this change in v0.18.
                    const DocumentObject* obj = static_cast<const DocumentObject*>(It->first);
                    const_cast<DocumentObject*>(obj)->setStatus(ObjectStatus::Destroy, true);
                }
                delete It->first;
            }
        }
        delete It->second;
    }
}

static std::atomic<int> _TransactionID;

int Transaction::getNewID() {
    int id = ++_TransactionID;
    if(id) return id;
    // wrap around? really?
    return ++_TransactionID;
}

int Transaction::getLastID() {
    return _TransactionID;
}

unsigned int Transaction::getMemSize (void) const
{
    return 0;
}

void Transaction::Save (Base::Writer &/*writer*/) const
{
    assert(0);
}

void Transaction::Restore(Base::XMLReader &/*reader*/)
{
    assert(0);
}

int Transaction::getID(void) const
{
    return transID;
}

bool Transaction::isEmpty() const
{
    return _Objects.empty();
}

bool Transaction::hasObject(const TransactionalObject *Obj) const
{
    return !!_Objects.get<1>().count(Obj);
}

void Transaction::addOrRemoveProperty(TransactionalObject *Obj,
                                    const Property* pcProp, bool add)
{
    auto &index = _Objects.get<1>();
    auto pos = index.find(Obj);

    TransactionObject *To;

    if (pos != index.end()) {
        To = pos->second;
    }
    else {
        To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        To->status = TransactionObject::Chn;
        index.emplace(Obj,To);
    }

    To->addOrRemoveProperty(pcProp,add);
}

//**************************************************************************
// separator for other implementation aspects


void Transaction::apply(Document &Doc, bool forward)
{
    std::string errMsg;
    try {
        auto &index = _Objects.get<0>();
        for(auto &info : index) 
            info.second->applyDel(Doc, const_cast<TransactionalObject*>(info.first));
        for(auto &info : index) 
            info.second->applyNew(Doc, const_cast<TransactionalObject*>(info.first));
        for(auto &info : index) 
            info.second->applyChn(Doc, const_cast<TransactionalObject*>(info.first), forward);
    }catch(Base::Exception &e) {
        e.ReportException();
        errMsg = e.what();
    }catch(std::exception &e) {
        errMsg = e.what();
    }catch(...) {
        errMsg = "Unknown exception";
    }
    if(errMsg.size()) {
        FC_ERR("Exception on " << (forward?"redo":"undo") << " '" 
                << Name << "':" << errMsg);
    }
}

void Transaction::addObjectNew(TransactionalObject *Obj)
{
    auto &index = _Objects.get<1>();
    auto pos = index.find(Obj);
    if (pos != index.end()) {
        if (pos->second->status == TransactionObject::Del) {
            delete pos->second;
            delete pos->first;
            index.erase(pos);
        }
        else {
            pos->second->status = TransactionObject::New;
            pos->second->_NameInDocument = Obj->detachFromDocument();
            // move item at the end to make sure the order of removal is kept
            auto &seq = _Objects.get<0>();
            seq.relocate(seq.end(),_Objects.project<0>(pos));
        }
    }
    else {
        TransactionObject *To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        To->status = TransactionObject::New;
        To->_NameInDocument = Obj->detachFromDocument();
        index.emplace(Obj,To);
    }
}

void Transaction::addObjectDel(const TransactionalObject *Obj)
{
    auto &index = _Objects.get<1>();
    auto pos = index.find(Obj);

    // is it created in this transaction ?
    if (pos != index.end() && pos->second->status == TransactionObject::New) {
        // remove completely from transaction
        delete pos->second;
        index.erase(pos);
    }
    else if (pos != index.end() && pos->second->status == TransactionObject::Chn) {
        pos->second->status = TransactionObject::Del;
    }
    else {
        TransactionObject *To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        To->status = TransactionObject::Del;
        index.emplace(Obj,To);
    }
}

void Transaction::addObjectChange(const TransactionalObject *Obj, const Property *Prop)
{
    auto &index = _Objects.get<1>();
    auto pos = index.find(Obj);

    TransactionObject *To;

    if (pos != index.end()) {
        To = pos->second;
    }
    else {
        To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        To->status = TransactionObject::Chn;
        index.emplace(Obj,To);
    }

    To->setProperty(Prop);
}


//**************************************************************************
//**************************************************************************
// TransactionObject
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::TransactionObject, Base::Persistence);

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
TransactionObject::TransactionObject()
  : status(New)
{
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
TransactionObject::~TransactionObject()
{
    for(auto &v : _PropChangeMap)
        delete v.second.property;
}

void TransactionObject::applyDel(Document & /*Doc*/, TransactionalObject * /*pcObj*/)
{
}

void TransactionObject::applyNew(Document & /*Doc*/, TransactionalObject * /*pcObj*/)
{
}

void TransactionObject::applyChn(Document & /*Doc*/, TransactionalObject *pcObj, bool Forward)
{
    if (status == New || status == Chn) {
        // Property change order is not preserved, as it is recursive in nature
        for(auto &v : _PropChangeMap) {
            auto &data = v.second;
            auto prop = const_cast<Property*>(v.first);

            if(!data.property) {
                // here means we are undoing/redoing and property add operation
                pcObj->removeDynamicProperty(v.second.name.c_str());
                continue;
            }

            // getPropertyName() is specially coded to be safe even if prop has
            // been destroies. We must prepare for the case where user removed
            // a dynamic property but does not recordered as transaction.
            auto name = pcObj->getPropertyName(prop);
            if(!name) {
                // Here means the original property is not found, probably removed
                if(v.second.name.empty()) {
                    // not a dynamic property, nothing to do
                    continue;
                }

                // It is possible for the dynamic property to be removed and
                // restored. But since restoring property is actually creating
                // a new property, the property key inside redo stack will not
                // match. So we search by name first.
                prop = pcObj->getDynamicPropertyByName(v.second.name.c_str());
                if(!prop) {
                    // Still not found, re-create the property
                    prop = pcObj->addDynamicProperty(
                            data.property->getTypeId().getName(),
                            v.second.name.c_str(), data.group.c_str(), data.doc.c_str(),
                            data.attr, data.readonly, data.hidden);
                    if(!prop)
                        continue;
                    prop->setStatusValue(data.property->getStatus());
                }
            }
            // Because we now allow undo/redo dynamic property adding/removing,
            // we have to enforce property type checking before calling Copy/Paste.
            if(data.property->getTypeId() != prop->getTypeId()) {
                FC_WARN("Cannot " << (Forward?"redo":"undo") 
                        << " change of property " << prop->getName()
                        << " because of type change: "
                        << data.property->getTypeId().getName()
                        << " -> " << prop->getTypeId().getName());
                continue;
            }
            prop->Paste(*data.property);
        }
    }
}

void TransactionObject::setProperty(const Property* pcProp)
{
    auto &data = _PropChangeMap[pcProp];
    if(!data.property && data.name.empty()) {
        data = pcProp->getContainer()->getDynamicPropertyData(pcProp);
        data.property = pcProp->Copy();
        data.property->setStatusValue(pcProp->getStatus());
    }
}

void TransactionObject::addOrRemoveProperty(const Property* pcProp, bool add)
{
    (void)add;
    if(!pcProp || !pcProp->getContainer())
        return;

    auto &data = _PropChangeMap[pcProp];
    if(data.name.size()) {
        if(!add && !data.property) {
            // this means add and remove the same property inside a single
            // transaction, so they cancel each other out.
            _PropChangeMap.erase(pcProp);
        }
        return;
    }
    if(data.property) {
        delete data.property;
        data.property = 0;
    }
    data = pcProp->getContainer()->getDynamicPropertyData(pcProp);
    if(add) 
        data.property = 0;
    else {
        data.property = pcProp->Copy();
        data.property->setStatusValue(pcProp->getStatus());
    }
}

unsigned int TransactionObject::getMemSize (void) const
{
    return 0;
}

void TransactionObject::Save (Base::Writer &/*writer*/) const
{
    assert(0);
}

void TransactionObject::Restore(Base::XMLReader &/*reader*/)
{
    assert(0);
}

//**************************************************************************
//**************************************************************************
// TransactionDocumentObject
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::TransactionDocumentObject, App::TransactionObject);

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
TransactionDocumentObject::TransactionDocumentObject()
{
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
TransactionDocumentObject::~TransactionDocumentObject()
{
}

void TransactionDocumentObject::applyDel(Document &Doc, TransactionalObject *pcObj)
{
    if (status == Del) {
        DocumentObject* obj = static_cast<DocumentObject*>(pcObj);

#ifndef USE_OLD_DAG
        //Make sure the backlinks of all linked objects are updated. As the links of the removed
        //object are never set to [] they also do not remove the backlink. But as they are 
        //not in the document anymore we need to remove them anyway to ensure a correct graph
        auto list = obj->getOutList();
        for (auto link : list)
            link->_removeBackLink(obj);
#endif

        // simply filling in the saved object
        Doc._removeObject(obj);
    }
}

void TransactionDocumentObject::applyNew(Document &Doc, TransactionalObject *pcObj)
{
    if (status == New) {
        DocumentObject* obj = static_cast<DocumentObject*>(pcObj);
        Doc._addObject(obj, _NameInDocument.c_str());

#ifndef USE_OLD_DAG
        //make sure the backlinks of all linked objects are updated
        auto list = obj->getOutList();
        for (auto link : list)
            link->_addBackLink(obj);
#endif
    }
}

//**************************************************************************
//**************************************************************************
// TransactionFactory
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

App::TransactionFactory* App::TransactionFactory::self = nullptr;

TransactionFactory& TransactionFactory::instance()
{
    if (self == nullptr)
        self = new TransactionFactory;
    return *self;
}

void TransactionFactory::destruct()
{
    delete self;
    self = nullptr;
}

void TransactionFactory::addProducer (const Base::Type& type, Base::AbstractProducer *producer)
{
    producers[type] = producer;
}

/**
 * Creates a transaction object for the given type id.
 */
TransactionObject* TransactionFactory::createTransaction (const Base::Type& type) const
{
    std::map<Base::Type, Base::AbstractProducer*>::const_iterator it;
    for (it = producers.begin(); it != producers.end(); ++it) {
        if (type.isDerivedFrom(it->first)) {
            return static_cast<TransactionObject*>(it->second->Produce());
        }
    }

    assert(0);
    return nullptr;
}
