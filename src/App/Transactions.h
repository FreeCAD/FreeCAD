// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <unordered_map>
#include <Base/Factory.h>
#include <Base/Persistence.h>
#include <App/PropertyContainer.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace bmi = boost::multi_index;

namespace App
{

class Document;
class Property;
class Transaction;
class TransactionObject;
class TransactionalObject;


/**
 * @brief A class that represents an atomic transaction of the document.
 *
 * A transaction can contain multiple actions.  These actions are represented
 * by TransactionObject objects.  Actions can be on objects: adding one,
 * removing one, or changing one.  It can also be on properties: adding one or
 * removing one.
 */
class AppExport Transaction: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * @brief Construct a transaction.
     *
     * @param[in] id: The transaction id. If zero, then it will be generated
     * automatically as a monotonically increasing index across the entire
     * application. Users can pass in a transaction id to group multiple
     * transactions from different document, so that they can be undone/redone
     * together.
     */
    explicit Transaction(int id = 0);

    ~Transaction() override;

    /**
     * @brief Apply the content of this transaction to the document.
     *
     * @param[in] Doc The document to apply the transaction to.
     * @param[in] forward If true, apply the transaction; otherwise, undo it.
     */
    void apply(Document& Doc, bool forward);

    /// The UTF-8 name of the transaction
    std::string Name;

    unsigned int getMemSize() const override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    /// Get the transaction ID of this transaction.
    int getID() const;

    /// Generate a new unique transaction ID.
    static int getNewID();

    /// Get the last transaction ID.
    static int getLastID();

    /// Check if the transaction list is empty.
    bool isEmpty() const;

    /**
     * @brief Check if this object is used in a transaction.
     *
     * @param[in] Obj The object to check.
     * @return true if the object is used in a transaction; otherwise false.
     */
    bool hasObject(const TransactionalObject* Obj) const;

    /**
     * @brief Record renaming a property.
     *
     * @param[in] Obj The object of which the property to rename.
     * @param[in] pcProp The property with a new name.
     * @param[in] oldName The old name of the property.
     */
    void renameProperty(TransactionalObject* Obj, const Property* pcProp, const char* oldName);

    /**
     * @brief Record adding or removing a property from an object.
     *
     * @param[in] Obj The object to add or remove the property from.
     * @param[in] pcProp The property to add or remove.
     * @param[in] add If true, add the property; otherwise, remove it.
     */
    void addOrRemoveProperty(TransactionalObject* Obj, const Property* pcProp, bool add);

    /**
     * @brief Record adding a new object to the transaction.
     *
     * @param[in] Obj The object to add.
     */
    void addObjectNew(TransactionalObject* Obj);

    /**
     * @brief Record removing an object from the transaction.
     *
     * @param[in] Obj The object to remove.
     */
    void addObjectDel(const TransactionalObject* Obj);

    /**
     *@brief Record changing an object in the transaction.
     *
     * @param[in] Obj The object to change.
     * @param[in] Prop The property that is changed.
     */
    void addObjectChange(const TransactionalObject* Obj, const Property* Prop);

private:
    void changeProperty(TransactionalObject* Obj,
                        std::function<void(TransactionObject* to)> changeFunc);

private:
    int transID;
    using Info = std::pair<const TransactionalObject*, TransactionObject*>;
    bmi::multi_index_container<
        Info,
        bmi::indexed_by<
            bmi::sequenced<>,
            bmi::hashed_unique<bmi::member<Info, const TransactionalObject*, &Info::first>>>>
        _Objects;
};

/**
 * @brief Class that represents an entry for an object in a Transaction.
 *
 * This class is used to store the information about the object and its
 * properties.  It should not be confused with the TransactionalObject class
 * that is a base class that contains functionality for a DocumentObject
 * regarding transactions.
 */
class AppExport TransactionObject: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TransactionObject();

    ~TransactionObject() override;

    /**
     * @brief Apply the transaction that adds a new object to the document.
     *
     * @param[in,out] doc The document to apply the transaction to.
     * @param[in,out] obj The object that is added in the transaction.
     */
    virtual void applyNew(Document& doc, TransactionalObject* obj);

    /**
     * @brief Apply the transaction that removes an object from the document.
     *
     * @param[in,out] doc The document to apply the transaction to.
     * @param[in,out] obj The object that is removed in the transaction.
     */
    virtual void applyDel(Document& doc, TransactionalObject* obj);

    /**
     * @brief Apply the transaction that changes an object in the document.
     *
     * @param[in,out] doc The document to apply the transaction to.
     * @param[in,out] obj The object that is changed in the transaction.
     * @param[in] forward If true, apply the transaction; otherwise, undo it.
     */
    virtual void applyChn(Document& doc, TransactionalObject* obj, bool forward);

    /**
     * @brief Set the property of the object that is affected by the transaction.
     *
     * @param[in] prop The property that is affected by the transaction.
     */
    void setProperty(const Property* prop);

    /**
     * @brief Rename a property.
     *
     * @param[in] pcProp The property with the new name.
     * @param[in] oldName The old name of the property.
     */
    void renameProperty(const Property* pcProp, const char* oldName);

    /**
     * @brief Add or remove a property from the object.
     *
     * @param[in] prop The property to add or remove.
     * @param[in] add If true, add the property; otherwise, remove it.
     */
    void addOrRemoveProperty(const Property* prop, bool add);

    unsigned int getMemSize() const override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    friend class Transaction;

protected:
    /// The status of the transaction object.
    enum Status
    {
        New, ///< A new object is added to the document.
        Del, ///< An object is deleted from the document.
        Chn ///< An object is changed in the document.
    } status {New};

    /// Struct to maintain property information.
    struct PropData: DynamicProperty::PropData
    {
        Base::Type propertyType;
        const Property* propertyOrig = nullptr;
        // for property renaming
        std::string nameOrig;
    };

    /// A map to maintain the properties of the object.
    std::unordered_map<int64_t, PropData> _PropChangeMap;

    /// The name of the object in the document.
    std::string _NameInDocument;
};

/**
 * @brief Class that represents an entry for a document object in a transaction.
 */
class AppExport TransactionDocumentObject: public TransactionObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TransactionDocumentObject();

    ~TransactionDocumentObject() override;

    void applyNew(Document& Doc, TransactionalObject* pcObj) override;
    void applyDel(Document& Doc, TransactionalObject* pcObj) override;
};

/**
 * @brief Class that represents a factory for creating transaction objects.
 *
 * This class is used to create transaction objects for different types of
 * transactions.
 */
class AppExport TransactionFactory
{
public:
    /// Get the singleton instance of the TransactionFactory.
    static TransactionFactory& instance();

    /// Destruct the singleton instance of the TransactionFactory.
    static void destruct();

    /**
     * @brief Create a transaction object for the given type.
     *
     * @param[in] type The type of the transaction object to create.
     * @return A pointer to the created transaction object.
     */
    TransactionObject* createTransaction(const Base::Type& type) const;

    /**
     * @brief Add a producer for a transaction object.
     *
     * @param[in] type The type id of the transaction object.
     * @param[in,out] producer The producer to add.
     */
    void addProducer(const Base::Type& type, Base::AbstractProducer* producer);

private:
    static TransactionFactory* self;
    std::map<Base::Type, Base::AbstractProducer*> producers;

    TransactionFactory() = default;
    ~TransactionFactory() = default;
};

/**
 * @brief Class that represents a producer for creating transaction objects.
 *
 * @tparam CLASS The class of the transaction object to produce.
 */
template<class CLASS>
class TransactionProducer: public Base::AbstractProducer
{
public:
    /**
     * @brief Construct a transaction producer.
     *
     * @param[in] type The type of the transaction object.
     */
    explicit TransactionProducer(const Base::Type& type)
    {
        TransactionFactory::instance().addProducer(type, this);
    }

    ~TransactionProducer() override = default;

    /// Creates an instance of the specified transaction object.
    void* Produce() const override
    {
        return (new CLASS);
    }
};

}  // namespace App
