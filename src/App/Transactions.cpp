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
    std::map<const DocumentObject*,TransactionObject*>::iterator It;
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
            if (!It->first->pcNameInDocument) {
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

int Transaction::getPos(void) const
{
    return iPos;
}

bool Transaction::hasObject(DocumentObject *Obj) const
{
    std::map<const DocumentObject*,TransactionObject*>::const_iterator it;
    it = _Objects.find(Obj);
    return (it != _Objects.end());
}

//**************************************************************************
// separator for other implemetation aspects


void Transaction::apply(Document &Doc, bool forward)
{
    std::map<const DocumentObject*,TransactionObject*>::iterator It;
    //for (It= _Objects.begin();It!=_Objects.end();++It)
    //    It->second->apply(Doc,const_cast<DocumentObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyDel(Doc,const_cast<DocumentObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyNew(Doc,const_cast<DocumentObject*>(It->first));
    for (It= _Objects.begin();It!=_Objects.end();++It)
        It->second->applyChn(Doc,const_cast<DocumentObject*>(It->first),forward);
}

void Transaction::addObjectNew(DocumentObject *Obj)
{
    std::map<const DocumentObject*,TransactionObject*>::iterator pos = _Objects.find(Obj);

    if (pos != _Objects.end()) {
        if (pos->second->status == TransactionObject::Del) {
            delete pos->second;
            delete pos->first;
            _Objects.erase(pos);
        }
        else {
            pos->second->status = TransactionObject::New;
            pos->second->_NameInDocument = Obj->getNameInDocument();
            Obj->pcNameInDocument = 0;
        }
    }
    else {
        TransactionObject *To = new TransactionObject(Obj,Obj->getNameInDocument());
        _Objects[Obj] = To;
        // set name cache false
        Obj->pcNameInDocument = 0;
        To->status = TransactionObject::New;
    }
}

void Transaction::addObjectDel(const DocumentObject *Obj)
{
    std::map<const DocumentObject*,TransactionObject*>::iterator pos = _Objects.find(Obj);

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
        TransactionObject *To = new TransactionObject(Obj);
        _Objects[Obj] = To;
        To->status = TransactionObject::Del;
    }
}

void Transaction::addObjectChange(const DocumentObject *Obj,const Property *Prop)
{
    std::map<const DocumentObject*,TransactionObject*>::iterator pos = _Objects.find(Obj);
    TransactionObject *To;

    if (pos != _Objects.end()) {
        To = pos->second;
    }
    else {
        To = new TransactionObject(Obj);
        _Objects[Obj] = To;
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
TransactionObject::TransactionObject(const DocumentObject * /*pcObj*/,const char *NameInDocument)
  : status(New)
{
    if (NameInDocument)
        _NameInDocument=NameInDocument;
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

void TransactionObject::applyDel(Document &Doc, DocumentObject *pcObj)
{
    if (status == Del) {
        // simply filling in the saved object
        Doc._remObject(pcObj);
    }
}

void TransactionObject::applyNew(Document &Doc, DocumentObject *pcObj)
{
    if (status == New) {
        Doc._addObject(pcObj,_NameInDocument.c_str());
    }
}

void TransactionObject::applyChn(Document & /*Doc*/, DocumentObject * /*pcObj*/,bool Forward)
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
    std::map<const Property*,Property*>::iterator pos = _PropChangeMap.find(pcProp);
    if (pos == _PropChangeMap.end())
        _PropChangeMap[pcProp] = pcProp->Copy();
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
