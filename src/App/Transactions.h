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


#ifndef APP_TRANSACTION_H
#define APP_TRANSACTION_H

#include <Base/Factory.h>
#include <Base/Persistence.h>

namespace App
{

class Document;
class Property;
class Transaction;
class TransactionObject;
class TransactionalObject;


/** Represents a atomic transaction of the document
 */
class AppExport Transaction : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    /// Construction
    Transaction();
    /// Construction
    Transaction(int pos);
    /// Destruction
    virtual ~Transaction();

    /// apply the content to the document
    void apply(Document &Doc,bool forward);

    // the utf-8 name of the transaction
    std::string Name;

    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);

    /// Returns true if the transaction list is empty; otherwise returns false.
    bool isEmpty() const;
    /// get the position in the transaction history
    int getPos(void) const;
    /// check if this object is used in a transaction
    bool hasObject(const TransactionalObject *Obj) const;
    void removeProperty(TransactionalObject *Obj, const Property* pcProp);

    void addObjectNew(TransactionalObject *Obj);
    void addObjectDel(const TransactionalObject *Obj);
    void addObjectChange(const TransactionalObject *Obj, const Property *Prop);

private:
    int iPos;
    typedef std::list <std::pair<const TransactionalObject*, TransactionObject*> > TransactionList;
    TransactionList _Objects;
};

/** Represents an entry for an object in a Transaction
 */
class AppExport TransactionObject : public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    /// Construction
    TransactionObject();
    /// Destruction
    virtual ~TransactionObject();

    virtual void applyNew(Document &Doc, TransactionalObject *pcObj);
    virtual void applyDel(Document &Doc, TransactionalObject *pcObj);
    virtual void applyChn(Document &Doc, TransactionalObject *pcObj, bool Forward);

    void setProperty(const Property* pcProp);
    void removeProperty(const Property* pcProp);

    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);

    friend class Transaction;

protected:
    enum Status {New,Del,Chn} status;
    std::map<const Property*,Property*> _PropChangeMap;
    std::string _NameInDocument;
};

/** Represents an entry for a document object in a transaction
 */
class AppExport TransactionDocumentObject : public TransactionObject
{
    TYPESYSTEM_HEADER();

public:
    /// Construction
    TransactionDocumentObject();
    /// Destruction
    virtual ~TransactionDocumentObject();

    void applyNew(Document &Doc, TransactionalObject *pcObj);
    void applyDel(Document &Doc, TransactionalObject *pcObj);
};

class AppExport TransactionFactory
{
public:
    static TransactionFactory& instance();
    static void destruct ();

    TransactionObject* createTransaction (const Base::Type& type) const;
    void addProducer (const Base::Type& type, Base::AbstractProducer *producer);

private:
    static TransactionFactory* self;
    std::map<Base::Type, Base::AbstractProducer*> producers;

    TransactionFactory(){}
    ~TransactionFactory(){}
};

template <class CLASS>
class TransactionProducer : public Base::AbstractProducer
{
public:
    TransactionProducer (const Base::Type& type)
    {
        TransactionFactory::instance().addProducer(type, this);
    }

    virtual ~TransactionProducer (){}

    /**
     * Creates an instance of the specified transaction object.
     */
    virtual void* Produce () const
    {
        return (void*)(new CLASS);
    }
};

} //namespace App

#endif // APP_TRANSACTION_H

