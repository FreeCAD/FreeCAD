/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <unordered_map>
#include <Base/Factory.h>
#include <Base/Persistence.h>
#include <App/PropertyContainer.h>

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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** Construction
     *
     * @param id: transaction id. If zero, then it will be generated
     * automatically as a monotonically increasing index across the entire
     * application. User can pass in a transaction id to group multiple
     * transactions from different document, so that they can be undo/redo
     * together.
     */
    explicit Transaction(int id = 0);
    /// Construction
    ~Transaction() override;

    /// apply the content to the document
    void apply(Document &Doc,bool forward);

    // the utf-8 name of the transaction
    std::string Name;

    unsigned int getMemSize () const override;
    void Save (Base::Writer &writer) const override;
    /// This method is used to restore properties from an XML document.
    void Restore(Base::XMLReader &reader) override;

    /// Return the transaction ID
    int getID() const;

    /// Generate a new unique transaction ID
    static int getNewID();
    static int getLastID();

    /// Returns true if the transaction list is empty; otherwise returns false.
    bool isEmpty() const;
    /// check if this object is used in a transaction
    bool hasObject(const TransactionalObject *Obj) const;
    void addOrRemoveProperty(TransactionalObject *Obj, const Property* pcProp, bool add);

    void addObjectNew(TransactionalObject *Obj);
    void addObjectDel(const TransactionalObject *Obj);
    void addObjectChange(const TransactionalObject *Obj, const Property *Prop);

private:
    int transID;
    using Info = std::pair<const TransactionalObject*, TransactionObject*>;
    bmi::multi_index_container<
        Info,
        bmi::indexed_by<
            bmi::sequenced<>,
            bmi::hashed_unique<
                bmi::member<Info, const TransactionalObject*, &Info::first>
            >
        >
    > _Objects;
};

/** Represents an entry for an object in a Transaction
 */
class AppExport TransactionObject : public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Construction
    TransactionObject();
    /// Destruction
    ~TransactionObject() override;

    virtual void applyNew(Document &Doc, TransactionalObject *pcObj);
    virtual void applyDel(Document &Doc, TransactionalObject *pcObj);
    virtual void applyChn(Document &Doc, TransactionalObject *pcObj, bool Forward);

    void setProperty(const Property* pcProp);
    void addOrRemoveProperty(const Property* pcProp, bool add);

    unsigned int getMemSize () const override;
    void Save (Base::Writer &writer) const override;
    /// This method is used to restore properties from an XML document.
    void Restore(Base::XMLReader &reader) override;

    friend class Transaction;

protected:
    enum Status {New,Del,Chn} status{New};

    struct PropData : DynamicProperty::PropData {
        Base::Type propertyType;
        const Property *propertyOrig = nullptr;
    };
    std::unordered_map<int64_t, PropData> _PropChangeMap;

    std::string _NameInDocument;
};

/** Represents an entry for a document object in a transaction
 */
class AppExport TransactionDocumentObject : public TransactionObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Construction
    TransactionDocumentObject();
    /// Destruction
    ~TransactionDocumentObject() override;

    void applyNew(Document &Doc, TransactionalObject *pcObj) override;
    void applyDel(Document &Doc, TransactionalObject *pcObj) override;
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

    TransactionFactory() = default;
    ~TransactionFactory() = default;
};

template <class CLASS>
class TransactionProducer : public Base::AbstractProducer
{
public:
    explicit TransactionProducer (const Base::Type& type)
    {
        TransactionFactory::instance().addProducer(type, this);
    }

    ~TransactionProducer () override = default;

    /**
     * Creates an instance of the specified transaction object.
     */
    void* Produce () const override
    {
        return (new CLASS);
    }
};

} //namespace App

#endif // APP_TRANSACTION_H

