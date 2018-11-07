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

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Writer.h>
using Base::Writer;
#include <Base/Reader.h>
using Base::XMLReader;
#include "Transactions.h"
#include "Property.h"
#include "Document.h"
#include "DocumentObject.h"

using namespace App;
using namespace std;

TYPESYSTEM_SOURCE(App::Transaction, Base::Persistence)

//**************************************************************************
// Construction/Destruction

Transaction::Transaction()
  : iPos(0)
{
}

Transaction::Transaction(int pos)
  : iPos(pos)
{
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Transaction::~Transaction()
{
    TransactionList::iterator It;
    for (It= _Objects.begin();It!=_Objects.end();++It) {
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

bool Transaction::isEmpty() const
{
    return _Objects.empty();
}

int Transaction::getPos(void) const
{
    return iPos;
}

bool Transaction::hasObject(const TransactionalObject *Obj) const
{
    TransactionList::const_iterator it;
    for (it = _Objects.begin(); it != _Objects.end(); ++it) {
        if (it->first == Obj)
            return true;
    }

    return false;
}

void Transaction::removeProperty(TransactionalObject *Obj,
                                 const Property* pcProp)
{
    for (auto it : _Objects) {
        if (it.first == Obj)
            it.second->removeProperty(pcProp);
    }
}

//**************************************************************************
// separator for other implementation aspects


void Transaction::apply(Document &Doc, bool forward)
{
    TransactionList::iterator It;
    //for (It= _Objects.begin();It!=_Objects.end();++It)
    //    It->second->apply(Doc,const_cast<DocumentObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyDel(Doc, const_cast<TransactionalObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyNew(Doc, const_cast<TransactionalObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyChn(Doc, const_cast<TransactionalObject*>(It->first), forward);
}

void Transaction::addObjectNew(TransactionalObject *Obj)
{
    TransactionList::iterator pos = _Objects.end();
    for (TransactionList::iterator it = _Objects.begin(); it != _Objects.end(); ++it) {
        if (it->first == Obj) {
            pos = it;
            break;
        }
    }

    if (pos != _Objects.end()) {
        if (pos->second->status == TransactionObject::Del) {
            delete pos->second;
            delete pos->first;
            _Objects.erase(pos);
        }
        else {
            pos->second->status = TransactionObject::New;
            pos->second->_NameInDocument = Obj->detachFromDocument();
            // move item at the end to make sure the order of removal is kept
            _Objects.splice(_Objects.end(), _Objects, pos);
        }
    }
    else {
        TransactionObject *To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        To->status = TransactionObject::New;
        To->_NameInDocument = Obj->detachFromDocument();
        _Objects.push_back(std::make_pair(Obj, To));
    }
}

void Transaction::addObjectDel(const TransactionalObject *Obj)
{
    TransactionList::iterator pos = _Objects.end();
    for (TransactionList::iterator it = _Objects.begin(); it != _Objects.end(); ++it) {
        if (it->first == Obj) {
            pos = it;
            break;
        }
    }

    // is it created in this transaction ?
    if (pos != _Objects.end() && pos->second->status == TransactionObject::New) {
        // remove completely from transaction
        delete pos->second;
        _Objects.erase(pos);
    }
    else if (pos != _Objects.end() && pos->second->status == TransactionObject::Chn) {
        pos->second->status = TransactionObject::Del;
    }
    else {
        TransactionObject *To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        _Objects.push_back(std::make_pair(Obj, To));
        To->status = TransactionObject::Del;
    }
}

void Transaction::addObjectChange(const TransactionalObject *Obj, const Property *Prop)
{
    TransactionList::iterator pos = _Objects.end();
    for (TransactionList::iterator it = _Objects.begin(); it != _Objects.end(); ++it) {
        if (it->first == Obj) {
            pos = it;
            break;
        }
    }

    TransactionObject *To;

    if (pos != _Objects.end()) {
        To = pos->second;
    }
    else {
        To = TransactionFactory::instance().createTransaction(Obj->getTypeId());
        _Objects.push_back(std::make_pair(Obj, To));
        To->status = TransactionObject::Chn;
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
    std::map<const Property*,Property*>::const_iterator It;
    for (It=_PropChangeMap.begin();It!=_PropChangeMap.end();++It)
        delete It->second;
}

void TransactionObject::applyDel(Document & /*Doc*/, TransactionalObject * /*pcObj*/)
{
}

void TransactionObject::applyNew(Document & /*Doc*/, TransactionalObject * /*pcObj*/)
{
}

void TransactionObject::applyChn(Document & /*Doc*/, TransactionalObject * /*pcObj*/, bool Forward)
{
    if (status == New || status == Chn) {
        // apply changes if any
        if (!Forward) {
            std::map<const Property*,Property*>::const_reverse_iterator It;
            std::map<const Property*,Property*>::const_reverse_iterator rendIt = _PropChangeMap.rend();
            for (It = _PropChangeMap.rbegin(); It != rendIt; ++It)
                const_cast<Property*>(It->first)->Paste(*(It->second));
        }
        else {
            std::map<const Property*,Property*>::const_iterator It;
            std::map<const Property*,Property*>::const_iterator endIt = _PropChangeMap.end();
            for (It = _PropChangeMap.begin(); It != endIt; ++It)
                const_cast<Property*>(It->first)->Paste(*(It->second));
        }
    }
}

void TransactionObject::setProperty(const Property* pcProp)
{
    std::map<const Property*, Property*>::iterator pos = _PropChangeMap.find(pcProp);
    if (pos == _PropChangeMap.end())
        _PropChangeMap[pcProp] = pcProp->Copy();
}

void TransactionObject::removeProperty(const Property* pcProp)
{
    std::map<const Property*, Property*>::iterator pos = _PropChangeMap.find(pcProp);
    if (pos != _PropChangeMap.end()) {
        delete pos->second;
        _PropChangeMap.erase(pos);
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
