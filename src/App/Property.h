// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Exception.h>
#include <Base/Persistence.h>
#include <boost/any.hpp>
#include <fastsignals/signal.h>
#include <bitset>
#include <string>
#include <FCGlobal.h>

#include "ElementNamingUtils.h"
namespace Py
{
class Object;
}

namespace App
{

class PropertyContainer;
class ObjectIdentifier;

/**
 * @brief %Base class of all properties.
 * @ingroup PropertyFramework
 *
 * This is the base class of all properties.  Properties are objects that are
 * used in documents or document objects that maintain all kinds of information
 * about the document or document objects.  Examples are properties of
 * features, such as the length, but properties can also keep track of shapes.
 * They are also used to gain access to the document object in the console.
 *
 * This abstract base class defines all methods shared by all possible
 * properties.  It is also possible to create user-defined properties.
 *
 * For a more high-level overview see topic @ref PropertyFramework "Property Framework".
 */
class AppExport Property: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:

    /**
     * @brief Defines the position in the status bitmask.
     */
    enum Status
    {
        /// Whether a property is touched.
        Touched = 0,
        /// Whether a property can be modified.
        Immutable = 1,
        /// Whether a property is read-only for the property editor.
        ReadOnly = 2,
        /// Whether the property is hidden in the property editor.
        Hidden = 3,
        /// Whether a property is saved in the document.
        Transient = 4,
        /// To turn ON PropertyMaterial edit.
        MaterialEdit = 5,
        /// To turn OFF PropertyMaterialList edit.
        NoMaterialListEdit = 6,
        /// Whether a property is an output property.
        Output = 7,
        /// Whether a dynamic property can be removed.
        LockDynamic = 8,
        /// Prevents causing `Gui::Document::setModified()`.
        NoModify = 9,
        /// Whether to allow change in a partial document.
        PartialTrigger = 10,
        /// Whether to prevent to touch the owner for a recompute on property change.
        NoRecompute = 11,
        /// Whether a floating point number should be saved as single precision.
        Single = 12,
        /// For PropertyLists, whether the order of the elements is
        /// relevant for the container using it.
        Ordered = 13,
        /// In case of expression binding, whether the expression on
        /// restore and touch the object on value change.
        EvalOnRestore = 14,
        /// For internal use to avoid recursive signaling.
        Busy = 15,
        /// Whether the linked object should be copied on change of the property.
        CopyOnChange = 16,
        /// Whether the property editor should create a button for user defined editing.
        UserEdit = 17,
        /// Do not propagate changes of the property to its container
        DisableNotify = 18,

        // The following bits are corresponding to PropertyType set when the
        // property added. These types are meant to be static, and cannot be
        // changed in runtime. It is mirrored here to save the linear search
        // required in PropertyContainer::getPropertyType()

        /// Mark the beginning of enum PropertyType bits.
        PropStaticBegin = 21,
        /// Whether the property is dynamically added.
        PropDynamic = 21,
        /// Corresponds to Prop_NoPersist.
        PropNoPersist = 22,
        /// Corresponds to Prop_NoRecompute.
        PropNoRecompute = 23,
        /// Corresponds to Prop_ReadOnly.
        PropReadOnly = 24,
        /// Corresponds to Prop_Transient.
        PropTransient = 25,
        /// Corresponds to Prop_Hidden.
        PropHidden = 26,
        /// Corresponds to Prop_Output.
        PropOutput = 27,
        /// Mark the end of enum PropertyType bits.
        PropStaticEnd = 28,

        /// User defined status bit.
        User1 = 28,
        /// User defined status bit.
        User2 = 29,
        /// User defined status bit.
        User3 = 30,
        /// User defined status bit.
        User4 = 31
    };

    /// Construct a property.
    Property();
    /// Destruct a property.
    ~Property() override;

    /**
     * @brief Safely delete a dynamic property.
     *
     * @param[inout] p The property to delete.
     */
    static void destroy(Property* p);

    unsigned int getMemSize() const override
    {
        // you have to implement this method in all property classes!
        return sizeof(father) + sizeof(StatusBits);
    }

    /**
     * @brief Get the name of this property in the belonging container.
     *
     * With \ref hasName() it can be checked beforehand if a valid name is set.
     *
     * @return The name of the property or if no name is set, an empty string.
     */
    const char* getName() const;

    /**
     * @brief Check whether the property has a name set.
     *
     * If no name is set then \ref getName() will return an empty string
     *
     * @return True if a name is set, false otherwise.
     */
    bool hasName() const;

    /**
     * @brief Check whether the passed name is valid.
     *
     * If a name is null or an empty string it is considered invalid, and valid
     * otherwise.
     *
     * @param[in] name The name to check.
     * @return True if the name is valid, false otherwise.
     */
    static bool isValidName(const char* name);

    /**
     * @brief Get the name of the property and its container.
     *
     * The container name is separated from the property name by a dot.  If the
     * property has no name, then a question mark is returned.  If the
     * container has no name, then a question mark is returned for the
     * container named.
     */
    std::string getFullName() const;

    /**
     * @brief Get the class name of the associated property editor item.
     *
     * @return The class name of the property editor item or an empty string if
     * not defined.
     */
    virtual const char* getEditorName() const
    {
        return "";
    }

    /**
     * @brief Get the type of the property in the container.
     *
     * The type is expressed as a bitmask of enum PropertyType.
     *
     * @return The type of the property as a bitmask of enum PropertyType.
     */
    short getType() const;

    /**
     * @brief Get the group of this property.
     *
     * @return The group of the property or a nullptr if not defined.
     */
    const char* getGroup() const;

    /**
     * @brief Get the documentation of this property.
     *
     * @return The documentation of the property or a nullptr if not defined.
     */
    const char* getDocumentation() const;

    /**
     * @brief Set the container of this property.
     *
     * This is called by the framework to set the father (container).
     *
     * @param[in] father The container of this property.
     */
    void setContainer(PropertyContainer* father);

    /**
     * @brief Get the container of this property.
     *
     * Get a pointer to the PropertyContainer derived class to which the
     * property belongs.
     *
     * @return A pointer to the PropertyContainer derived class.
     */
    PropertyContainer* getContainer() const
    {
        return father;
    }

    /**
     * @brief Set the value of the property identified by the path.
     *
     * This function sets the value of the property identified by the path.  It
     * is meant to be overridden for subclasses in which the `path` is
     * typically ignored.  The default implementation redirects setting a value
     * to the `path` ObjectIdentifier.
     *
     * @param[in] path The path to the property.
     * @param[in] value The value to set.
     */
    virtual void setPathValue(const App::ObjectIdentifier& path, const boost::any& value);

    /**
     * @brief Get the value of the property identified by the path.
     *
     * This function gets the value of the property identified by the path.  It
     * is meant to be overridden for subclasses in which the `path` is
     * typically ignored.  The default implementation makes use of the `path`
     * ObjectIdentifier to get the value of the property.
     *
     * @param[in] path The path to the property.
     * @return The value of the property.
     */
    virtual const boost::any getPathValue(const App::ObjectIdentifier& path) const;

    /**
     * @brief Get the Python value of the property identified by the path.
     *
     * This function gets the Python value of the property identified by the
     * path.  It is meant to be overridden for subclasses.  This default
     * implementation return `false`.
     *
     * @param[in] path The path to the property.
     * @param[out] value The Python value of the property.
     * @return True if the value was successfully retrieved, false otherwise.
     */
    virtual bool getPyPathValue([[maybe_unused]] const App::ObjectIdentifier& path,
                                [[maybe_unused]] Py::Object& value) const
    {
        return false;
    }

    /**
     * @brief Convert an object identifier to a canonical representation.
     *
     * Convert an object identifier to a canonical representation of the object
     * identifier.
     *
     * @param[in] path The object identifier to convert.
     * @return An object identifier that represents the canonical path.
     */
    virtual App::ObjectIdentifier canonicalPath(const App::ObjectIdentifier& path) const;

    /**
     * @brief Get valid paths for this property.
     *
     * This function is used by auto completer.
     *
     * @param[out] paths A vector of object identifiers with the valid paths
     * for this property.
     */
    virtual void getPaths(std::vector<App::ObjectIdentifier>& paths) const;

    /** @brief Callback for after document restore.
     *
     * This function is called at the beginning of Document::afterRestore().
     * It is called without dependency sorting, because some link property
     * types can only reconstruct the linking information inside this function.
     * Typical use cases include:
     *
     * - **PropertyLinkSub**
     *   Parses and restores sub-object references (including
     *   those that point into external documents), applying any necessary
     *   name-mapping during import.
     *
     * - **PropertyExpressionEngine**
     *   Re-parses expressions after restore so that it can handle document
     *   name adjustment.  It internally relies on PropertyXLink to store the
     *   external document path for external linking.  When the external
     *   document is restored, its internal name may change due to name
     *   conflict with existing documents.  With this callback it can now auto
     *   adjust external references without any problem.
     */
    virtual void afterRestore() {}

    /** @brief Callback for after document restore.
     *
     * This function is called by Document::restore() after finished calling
     * Property::afterRestore() on all properties of objects. By then, the
     * object dependency information is assumed ready. So, unlike
     * Property::afterRestore(), this function is called on objects with
     * dependency order.
     *
     * It is called before calling DocumentObject::onDocumentRestored().
     */
    virtual void onContainerRestored() {}

    /** Property status handling
     */
    //@{
    /// This method sets whether notification will be propagated on changing
    /// the value of the property. The old value of the setting is returned.
    bool enableNotify(bool on);
    /// This method returns whether notification of changes to the property value
    /// are propagated to the container.
    bool isNotifyEnabled() const;
    /// Set the property touched
    void touch();

    /**
     * @brief Test if this property is touched.
     *
     * @return True if the property is touched, false otherwise.
     */
    inline bool isTouched() const
    {
        return StatusBits.test(Touched);
    }

    /// Reset this property as being touched.
    inline void purgeTouched()
    {
        StatusBits.reset(Touched);
    }

    /**
     * @brief Get the status of this property.
     *
     * @return The status of the property as a bitmask of enum Status.
     */
    inline unsigned long getStatus() const
    {
        return StatusBits.to_ulong();
    }

    /**
     * @brief Test if the property has a specific status.
     *
     * @param[in] pos The status to test.
     * @return True if the property has the specified status, false otherwise.
     */
    inline bool testStatus(Status pos) const
    {
        return StatusBits.test(static_cast<size_t>(pos));
    }

    /**
     * @brief Set the status of the property.
     *
     * Sets the status of the property given a specific position.
     *
     * @param[in] pos The status to set.
     * @param[in] on The value to set the status to.
     */
    void setStatus(Status pos, bool on);

    /**
     * @brief Set the status of the property.
     *
     * Sets the status of the property given a specific bitmask.
     *
     * @param[in] status The status to set as a bitmask.
     */
    void setStatusValue(unsigned long status);

    /**
     * @brief Set the property read only.
     *
     * This sets property editable/grayed out in property editor.
     * @param[in] readOnly True to set the property as read-only, false otherwise.
     */
    void setReadOnly(bool readOnly);

    /**
     * @brief Check if the property is read-only.
     *
     * @return True if the property is read-only, false otherwise.
     */
    inline bool isReadOnly() const
    {
        return testStatus(App::Property::ReadOnly);
    }

    /**
     * @brief Set the precision of floating point properties.
     *
     * This sets the precision of properties using floating point
     * numbers to single precision. The default is double precision.
     *
     * @param[in] single True to set single precision, false for double precision.
     */
    void setSinglePrecision(bool single)
    {
        setStatus(App::Property::Single, single);
    }

    /**
     * @brief Gets precision of floating point properties.
     *
     * @return True if single precision is set, false for double precision.
     */
    inline bool isSinglePrecision() const
    {
        return testStatus(App::Property::Single);
    }
    /// @}

    /**
     * @brief Returns a new copy of the property.
     *
     * The copy is mainly used for Undo/Redo and transactions.
     *
     * @return A new copy of the property.
     */
    virtual Property* Copy() const = 0;

    /**
     * @brief Pastes the value from a property.
     *
     * This function pastes the value from a property into this property.  It
     * is mainly used for Undo/Redo and transactions.
     *
     * @param[in] from The property to paste from.
     */
    virtual void Paste(const Property& from) = 0;

    /**
     * @brief Callback for when a child property has changed value.
     *
     * @param[in] prop The child property that has changed value.
     */
    virtual void hasSetChildValue([[maybe_unused]] Property& prop) {}

    /**
     * @brief Callback for when a child property is about to change value.
     *
     * It is called before a child property changes value.
     *
     *@param[in] prop The child property that is about to change value.
     */
    virtual void aboutToSetChildValue([[maybe_unused]] Property& prop) {}

    /**
     * @brief Compare if this property has the same content as the given one.
     *
     * @param[in] other The property to compare with.
     * @return True if the properties are the same, false otherwise.
     */
    virtual bool isSame(const Property& other) const;

    /**
     * @brief Return a unique ID for the property.
     *
     * The ID of a property is generated from a monotonically increasing
     * internal counter.  The intention of the ID is to be used as a key for
     * mapping, instead of using the raw pointer.  This prevent the runtime
     * allocator to reuse just deleted memory as a result of using raw
     * pointers, something that is challenging to debug.
     */
    int64_t getID() const
    {
        return _id;
    }

    /**
     * @brief Callback for when the property is about to be saved.
     *
     * This method is called before saving the property.  It can be overridden
     * by subclasses to implement custom behavior before saving.
     *
     * @see PropertyContainer::beforeSave()
     */
    virtual void beforeSave() const {}

    friend class PropertyContainer;
    friend struct PropertyData;
    friend class DynamicProperty;

protected:
    /** @brief %Status bits of the property.
     *
     * The first 8 bits are used for the base system the rest can be used in
     * descendent classes to mark special statuses on the objects.
     * The bits and their meaning are listed below:
     * - **0**: object is marked as 'touched'
     * - **1**: object is marked as 'immutable'
     * - **2**: object is marked as 'read-only' (for property editor)
     * - **3**: object is marked as 'hidden' (for property editor)
     *
     * @see Property::Status that defines the position of the status bits.
     */
    std::bitset<32> StatusBits;

protected:

    /**
     * @brief Callback for when the value of the property has changed.
     *
     * This is called by all setValue() methods after the value has changed.
     */
    virtual void hasSetValue();

    /**
     * @brief Callback for when the value of the property is about to change.
     *
     * This is called by all setValue() methods before the value has changed.
     */
    virtual void aboutToSetValue();

    /**
     * @brief Verify a path for the current property.
     *
     * @param[in] path The path to verify.
     */
    virtual void verifyPath(const App::ObjectIdentifier& path) const;

    /**
     * @brief Return a file name suitable for saving this property.
     *
     * @param[in] postfix The postfix to append to the file name.
     * @param[in] prefix The prefix to prepend to the file name.
     * @return The file name for saving the property.
     */
    std::string getFileName(const char* postfix = nullptr, const char* prefix = nullptr) const;

public:
    /**
     * @brief The copy constructor is deleted to prevent copying.
     */
    Property(const Property&) = delete;

    /**
     * @brief The assignment operator is deleted to prevent assignment.
     */
    Property& operator=(const Property&) = delete;

private:
    // Sync status with Property_Type
    void syncType(unsigned type);

private:
    PropertyContainer* father {nullptr};
    const char* myName {nullptr};
    int64_t _id;

public:
    /// Signal emitted when the property value has changed.
    fastsignals::signal<void(const App::Property&)> signalChanged;
};


/**
 * @brief A template class to inhibit nested calls for setting values.
 *
 * A template class that is used to inhibit multiple nested calls to
 * aboutToSetValue/hasSetValue for properties, and only invoke it on change and
 * on the last time it is needed. This is useful in cases where you want to
 * change multiple values in a property "atomically", using possibly multiple
 * primitive functions that normally would trigger aboutToSetValue/hasSetValue
 * calls on their own.
 *
 * To use, inherit privately from the AtomicPropertyChangeInterface class,
 * using your class name as the template argument. In all cases in which you
 * would normally call aboutToSetValue/hasSetValue before and after a change,
 * create an AtomicPropertyChange object.
 *
 * The default constructor assumes you are about to change the property and
 * will call property's Property::aboutToSetValue() if the property has not
 * been marked as changed before by any other AtomicPropertyChange instances in
 * current call stack.  You can pass `false` as the a second argument to the
 * constructor, and manually call AtomicPropertyChange::aboutToChange() before
 * actual change, which enables you to prevent unnecessary property copy for
 * undo/redo when there are actual changes.  AtomicPropertyChange will
 * guarantee calling Property::hasSetValue() when the last instance in the
 * current call stack is destroyed.
 *
 * One thing to take note is that, because C++ does not allow throwing
 * exception in destructor, any exception thrown when calling property's
 * Property::hasSetValue() will be caught and swallowed.  To allow exception
 * propagation, you can manually call AtomicPropertyChange::tryInvoke(). If the
 * condition is satisfied, it will call Property::hasSetValue() that allows
 * exception propagation.
 */
template<class P>
class AtomicPropertyChangeInterface
{
protected:
    AtomicPropertyChangeInterface() = default;

public:
    /**
     * @brief A class that captures property changes atomically.
     *
     * This class is used to capture multiple property changes atomically.  It
     * automatically marks the property as changed and calls the
     * Property::aboutToSetValue() method when the object is constructed.  It
     * also automatically calls the Property::hasSetValue() method when the
     * object is destructed, if the property has been marked as changed.  This
     * ensures that Property::aboutToSetValue() and Property::hasSetValue() are
     * only called once for multiple property changes.
     *
     * @see AtomicPropertyChangeInterface
     */
    class AtomicPropertyChange
    {
    public:
        /**
         * @brief Construct an AtomicPropertyChange object.
         *
         * @param prop: the property
         * @param markChange: If true, marks the property as changed if it
         *                    hasn't been marked before, and calls its
         *                    aboutToSetValue().
         */
        explicit AtomicPropertyChange(P& prop, bool markChange = true)
            : mProp(prop)
        {
            mProp.signalCounter++;
            if (markChange) {
                aboutToChange();
            }
        }

        /**
         * @brief Mark the property as changed
         *
         * It will mark the property as changed only if it has been marked
         * before, and only then will it call the property's aboutToSetValue().
         */
        void aboutToChange()
        {
            if (!mProp.hasChanged) {
                mProp.hasChanged = true;
                mProp.aboutToSetValue();
            }
        }

        /**
         * @brief Destruct an AtomicPropertyChange object.
         *
         * If the property is marked as changed, and this is the last instance
         * of the class in current call stack, it will call property's
         * hasSetValue()
         */
        ~AtomicPropertyChange()
        {
            // Signal counter == 1? meaning we are the last one. Invoke
            // hasSetValue() before decrease counter to prevent recursive call
            // triggered by another AtomicPropertyChange created inside
            // hasSetValue(), as it has now been changed to a virtual function.
            if (mProp.signalCounter == 1 && mProp.hasChanged) {
                // Must make sure to not throw in a destructor
                try {
                    mProp.hasSetValue();
                }
                catch (Base::Exception& e) {
                    e.reportException();
                }
                catch (...) {
                }
                mProp.hasChanged = false;
            }
            if (mProp.signalCounter > 0) {
                mProp.signalCounter--;
            }
        }

        /**
         * @brief Check and invoke property's hasSetValue().
         *
         * Check if this is the last instance and the property has been marked
         * as changed. If so, invoke property's hasSetValue().
         */
        // Destructor cannot throw. So we provide this function to allow error
        // propagation.
        void tryInvoke()
        {
            if (mProp.signalCounter == 1 && mProp.hasChanged) {
                mProp.hasSetValue();
                if (mProp.signalCounter > 0) {
                    --mProp.signalCounter;
                }
                mProp.hasChanged = false;
            }
        }

    private:
        P& mProp; /**< Referenced to property we work on */
    };

protected:
    /// Counter for invoking transaction start/stop.
    int signalCounter {0};
    /// Flag to indicate if the property has been changed.
    bool hasChanged {false};
};


/**
 * @brief Base class for list-like properties.
 *
 * This class is not derived from Property so that we can have more that one
 * base class for list-like properties.
 *
 * @see PropertyList
 * @see PropertyLinkListBase
 */
class AppExport PropertyListsBase
{
public:

    /**
     * @brief Set the size of the property list.
     *
     *@param newSize The new size of the property list.
     */
    virtual void setSize(int newSize) = 0;

    /**
     * @brief Get the size of the property list.
     *
     * @return The size of the property list.
     */
    virtual int getSize() const = 0;

    /**
     * @brief Get the list of touched elements.
     *
     * @return A set of touched elements.
     */
    const std::set<int>& getTouchList() const
    {
        return _touchList;
    }

    /// Clear the list of touched elements.
    void clearTouchList()
    {
        _touchList.clear();
    }

protected:
    /**
     * @brief Set the values of the property list with Python values.
     *
     * This method is used to set the values of the property list with values
     * represented as Python objects.  If `indices` is empty, all values are
     * set.
     *
     * @param[in] vals The new values for the property list as Python values.
     * @param[in] indices The indices of the values to set.
     */
    virtual void setPyValues(const std::vector<PyObject*>& vals, const std::vector<int>& indices)
    {
        (void)vals;
        (void)indices;
        throw Base::NotImplementedError("not implemented");
    }

    /**
     * @brief Set the values of the property list with a Python object.
     *
     * The Python object is expected to be a dictionary or something that looks
     * like a sequence of values, for example a list or tuple, or an iterable.
     *
     * @param[in] pyObj The Python object to set the values from.
     */
    void _setPyObject(PyObject* pyObj);

protected:
    /// The list of touched elements.
    std::set<int> _touchList;
};

/**
 * @brief The base class of all property lists.
 *
 * The PropertyLists class is the base class for properties that can contain
 * multiple values, not only a single value.  All property types that may
 * contain more than one value inherit from this class.
 */
class AppExport PropertyLists: public Property, public PropertyListsBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * @brief Set the values of the property list with a Python object.
     *
     * The Python object is expected to be a dictionary or something that looks
     * like a sequence of values, for example a list or tuple, or an iterable.
     *
     * @param[in] obj The Python object to set the values from.
     */
    void setPyObject(PyObject* obj) override
    {
        _setPyObject(obj);
    }

    /**
     * @brief Set the order of the elements to be relevant.
     *
     * If the order of the elements in the list is relevant, certain
     * operations, such as restoring, must ensure that the order is kept
     * despite potential errors.
     *
     * @param[in] on True to set the order as relevant, false otherwise.
     */
    inline void setOrderRelevant(bool on)
    {
        this->setStatus(Status::Ordered, on);
    }

    /**
     * @brief Check if the order of the elements is relevant.
     *
     * @return True if the order is relevant, false otherwise.
     */
    inline bool isOrderRelevant() const
    {
        return this->testStatus(Status::Ordered);
    }
};

/**
 * @brief Helper class to implement PropertyLists.
 *
 * This class combines property storage (via a standard container ListT) with
 * the change-notification interface in AtomicPropertyChangeInterface.
 *
 * @tparam T       The type of individual property values.
 * @tparam ListT   The container type for holding values (defaults to @c std::vector<T>).
 * @tparam ParentT The base class providing core property-list behavior
 *                 (defaults to PropertyLists).
 */
template<class T, class ListT = std::vector<T>, class ParentT = PropertyLists>
class PropertyListsT: public ParentT,
                      public AtomicPropertyChangeInterface<PropertyListsT<T, ListT, ParentT>>
{
public:
    /// Alias for a reference to a const element in the list.
    using const_reference = typename ListT::const_reference;

    /// The underlying container type.
    using list_type = ListT;

    /// The base class type.
    using parent_type = ParentT;

    /**
     * @brief Helper type for performing atomic property changes.
     *
     * This is defined by the AtomicPropertyChangeInterface class.
     */
    using atomic_change = typename AtomicPropertyChangeInterface<
        PropertyListsT<T, ListT, ParentT>>::AtomicPropertyChange;

    /**
     * @brief Grant atomic_change access to private internals.
     *
     * Allows atomic_change operations to modify the internal state directly.
     */
    friend atomic_change;

    /**
     * @brief Resize the property list, filling new slots with a given value.
     *
     * @param[in] newSize  The desired total number of elements.
     * @param[in] def      The single element value to use for all newly-added slots.
     */
    virtual void setSize(int newSize, const_reference def)
    {
        _lValueList.resize(newSize, def);
    }

    /**
     * @brief Resize the property list, value-initializing any new elements.
     *
     * Adjusts the container to hold exactly @p newSize elements.
     * - If @p newSize is less than the current size, the container is shrunk.
     * - If @p newSize is greater, new elements are default-constructed
     *
     * @param[in] newSize  The desired total number of elements.
     */
    void setSize(int newSize) override
    {
        _lValueList.resize(newSize);
    }

    /**
     * @brief Get the size of the list.
     *
     * @return The size of the list.
     */
    int getSize() const override
    {
        return static_cast<int>(_lValueList.size());
    }

    /**
     * @brief Set a property list to a given value.
     *
     * Clears any existing values and makes the list contain exactly one
     * element initialized to @p value.
     *
     * @param[in] value  The value to assign.
     */
    void setValue(const_reference value)
    {
        ListT vals;
        vals.resize(1, value);
        setValues(vals);
    }

    /**
     * @brief Replace the entire list of values.
     *
     * Clears and assigns @p newValues to the internal list, notifying
     * observers of the change atomically.
     *
     * @param[in] newValues  The new container of values (defaults to empty list).
     */
    virtual void setValues(const ListT& newValues = ListT())
    {
        atomic_change guard(*this);
        this->_touchList.clear();
        this->_lValueList = newValues;
        guard.tryInvoke();
    }

    /**
     * @brief Alias for setValues().
     *
     * @param[in] newValues  The new container of values (defaults to empty list).
     */
    void setValue(const ListT& newValues = ListT())
    {
        setValues(newValues);
    }

    /**
     * @brief Retrieve the underlying list of values.
     *
     * @return A const reference to the internal container.
     */
    const ListT& getValues() const
    {
        return _lValueList;
    }

    /**
     * @brief Alias for getValues().
     *
     * @return A const reference to the internal container.
     */
    const ListT& getValue() const
    {
        return getValues();
    }

    /**
     * @brief Retrieve an element by index.
     *
     * @param[in] idx The index of the element to retrieve.
     * @return The element at position @p idx.
     *
     * @note No bounds-check is performed.
     */
    const_reference operator[](int idx) const
    {
        return _lValueList[idx];
    }

    /**
     * @brief Compare two Property instances for equivalence.
     *
     * @param[in] other  The other Property to compare against.
     * @return True if both are of the same type and contain equal values.
     */
    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return this->getTypeId() == other.getTypeId()
            && this->getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

    /**
     * @brief Set the property value from a Python object.
     *
     * Attempts to extract a value via getPyValue() and assign it;
     * on failure, falls back to the ParentT implementation.
     *
     * @param[in] value  A PyObject pointer representing the new value.
     */
    void setPyObject(PyObject* value) override
    {
        try {
            setValue(getPyValue(value));
            return;
        }
        catch (...) {
        }
        parent_type::setPyObject(value);
    }

    /**
     * @brief Set a value at a specific index, growing the list if needed.
     *
     * If @p index is -1 or equal to the current size, the list grows by one.
     * Otherwise, the existing element at @p index is replaced.
     * Observers are notified of the change atomically.
     *
     * @param[in] index  The position at which to set @p value (-1 for append).
     * @param[in] value  The new element value.
     * @throw Base::RuntimeError if @p index is out of bounds (< -1 or > size).
     */
    virtual void set1Value(int index, const_reference value)
    {
        int size = getSize();
        if (index < -1 || index > size) {
            throw Base::RuntimeError("index out of bound");
        }

        atomic_change guard(*this);
        if (index == -1 || index == size) {
            index = size;
            setSize(index + 1, value);
        }
        else {
            _lValueList[index] = value;
        }
        this->_touchList.insert(index);
        guard.tryInvoke();
    }

protected:
    void setPyValues(const std::vector<PyObject*>& vals, const std::vector<int>& indices) override
    {
        if (indices.empty()) {
            ListT values {};
            values.reserve(vals.size());
            for (auto* valsContent : vals) {
                values.push_back(getPyValue(valsContent));
            }
            setValues(std::move(values));
            return;
        }
        assert(vals.size() == indices.size());
        atomic_change guard(*this);
        int i {0};
        for (auto index : indices) {
            set1Value(index, getPyValue(vals[i]));
            i++;
        }
        guard.tryInvoke();
    }

    /**
     * @brief Convert a Python object to a value of type T.
     *
     * @param[in] item  The Python object to convert.
     *
     * @return The converted value of type T.
     */
    virtual T getPyValue(PyObject* item) const = 0;

protected:
    ListT _lValueList;
};

}  // namespace App
