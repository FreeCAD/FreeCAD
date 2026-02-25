// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#include <bitset>
#include <climits>
#include <map>
#include <limits>
#include <set>
#include <string>
#include <vector>
#include <boost/any.hpp>
#include <FCConfig.h>

#include "ElementNamingUtils.h"

namespace Py
{
class Object;
}
namespace App
{

using any = boost::any;

/**
 * @brief Extract a const reference from a boost::any object.
 *
 * This function is a wrapper around boost::any_cast that allows
 * to extract a const reference from a boost::any object.
 *
 * @tparam T The type to extract.
 *
 * @param[in] value The boost::any object to extract from.
 *
 * @return A const reference to the extracted value.
 * @throws boost::bad_any_cast if the type of the value does not match T.
 */
template<class T>
inline const T& any_cast(const boost::any& value)
{
    return boost::any_cast<const T&>(value);
}

/**
 * @brief Extract a mutable reference from a boost::any object.
 *
 * This function is a wrapper around boost::any_cast that allows
 * to extract a mutable reference from a boost::any object.
 *
 * @tparam T The type to extract.
 *
 * @param[in] value The boost::any object to extract from.
 *
 * @return A const reference to the extracted value.
 * @throws boost::bad_any_cast if the type of the value does not match T.
 */
template<class T>
inline T& any_cast(boost::any& value)
{
    return boost::any_cast<T&>(value);
}

class Property;
class Document;
class PropertyContainer;
class DocumentObject;
class ExpressionVisitor;

/**
 * @brief Quote a string.
 *
 * Quote an input string according to quoting rules for an expression: because
 * " and ' are used to designate inch and foot units, strings are quoted as
 * `<<string>>`.
 *
 * @param[in] input     The string to quote.
 * @param[in] toPython  If true, use Python quoting rules. Otherwise, use
 *                      FreeCAD quoting rules.
 *
 * @return The string quoted.
 */
AppExport std::string quote(const std::string& input, bool toPython = false);

// Unfortunately VS2013 does not support default move constructor, so we have
// to implement them manually
#define FC_DEFAULT_CTORS(_t)                                                                       \
    _t(const _t&) = default;                                                                       \
    _t& operator=(const _t&) = default;                                                            \
    _t(_t&& other)                                                                                 \
    {                                                                                              \
        *this = std::move(other);                                                                  \
    }                                                                                              \
    _t& operator=(_t&& other)


/**
 * @brief A class that identifies properties in document objects.
 *
 * An object identifier is a data structure that identifies a (sub)properties
 * in a document or document objects, or documents or document objects
 * themselves.
 */
class AppExport ObjectIdentifier
{

public:
    /**
     * @brief A helper class to maintain a mapping between document names.
     *
     * This class maps old document names to new document names and is used to
     * provide a scope in which calls to setDocumentName() make use of the
     * mapping stored here.
     */
    class AppExport DocumentMapper
    {
    public:
        /**
         * @brief Construct a DocumentMapper object.
         *
         * The constructor takes a map of document names and stores a pointer
         * to it in a variable local to the compilation unit.  The idea is to
         * create a %DocumentMapper on the stack and when it goes out of scope,
         * all is reset.  During that time calls to setDocumentName() can be
         * done, making use of the mapping provided with this constructor.
         */
        explicit DocumentMapper(const std::map<std::string, std::string>&);

        /**
         * @brief Destroy the DocumentMapper object.
         *
         * When the DocumentMapper goes out of scope, the pointer to the
         * variable local to the compilation unit is set to `nullptr`.
         */
        ~DocumentMapper();
    };

    /**
     * @brief A class that represents a string in an ObjectIdentifier.
     */
    class String
    {
        friend class ObjectIdentifier;

    public:
        /**
         * @brief Construct a String object for object identifiers.
         *
         * @param[in] s The string to be used.
         * @param[in] _isRealString If true, the string is a real string and should be
         *                          quoted. Otherwise, it is a simple identifier.
         * @param[in] _forceIdentifier If true, the string is a forced identifier.
         */
        String(const std::string& s = "", bool _isRealString = false, bool _forceIdentifier = false)
            : str(s)
            , isString(_isRealString)
            , forceIdentifier(_forceIdentifier)
        {}  // explicit bombs

        /**
         * @brief Explicit move‐construct a String object for object identifiers.
         *
         * This constructor takes ownership of the provided string via move semantics,
         * preventing an implicit conversion from temporary std::string.
         *
         * @param[in,out] s The string to be used (will be moved-from).
         * @param[in] _isRealString If true, the string is a real string and should be
         *                          quoted. Otherwise, it is a simple identifier.
         * @param[in] _forceIdentifier If true, the string is a forced identifier.
         */
        explicit String(std::string&& s, bool _isRealString = false, bool _forceIdentifier = false)
            : str(std::move(s))
            , isString(_isRealString)
            , forceIdentifier(_forceIdentifier)
        {}

        FC_DEFAULT_CTORS(String)
        {
            str = std::move(other.str);
            isString = other.isString;
            forceIdentifier = other.forceIdentifier;
            return *this;
        }

        // Accessors

        /**
         * @brief Get the string as a `std::string`.
         *
         * @return The string as a `std::string`.
         */
        const std::string& getString() const
        {
            return str;
        }

        /**
         * @brief Test whether the string is a real string.
         *
         * A real string is a string that should be quoted.  A simple identifier
         * is a string that should not be quoted.
         *
         * @return True if the string is a real string, false otherwise.
         */
        bool isRealString() const
        {
            return isString;
        }

        /**
         * @brief Test whether the string is a forced identifier.
         *
         * A forced identifier is a string that should be treated as an
         * identifier.
         *
         * @return True if the string is a forced identifier, false otherwise.
         */
        bool isForceIdentifier() const
        {
            return forceIdentifier;
        }

        /**
         * @brief Get a string representation of this object identifier.
         *
         * @param[in] toPython If true, use Python quoting rules. Otherwise, use
         *                     FreeCAD quoting rules.
         * @return The string representation.
         */
        std::string toString(bool toPython = false) const;

        // Operators

        /**
         * @brief Explicitly convert to `std::string`.
         *
         * Returns a copy of the internal std::string. Because this operator is
         * marked explicit, it is required to use a cast:
         * `static_cast<std::string>(myString)`.
         *
         * @return A copy of the string as `std::string`.
         */
        explicit operator std::string() const
        {
            return str;
        }

        /**
         * @brief Explicitly convert to C-style string.
         *
         * Returns a pointer to the internal null-terminated character array.
         * The pointer remains valid as long as the String object is alive and
         * unmodified.  This operator is explicit, so it is required to use:
         * `static_cast<const char*>(myString)`.
         *
         * @return A const char* pointing at the internal string buffer.
         */
        explicit operator const char*() const
        {
            return str.c_str();
        }

        /**
         * @brief Test for equality.
         *
         * Compares the underlying string values for exact equality.
         *
         * @param[in] other The %String to compare against.
         * @return true if both strings have identical content; false otherwise.
         */
        bool operator==(const String& other) const
        {
            return str == other.str;
        }

        /**
         * @brief Test for inequality.
         *
         * Compares the underlying string values for inequality.
         *
         * @param[in] other The %String we want to compare to.
         * @return true if this string does not have identical content to the
         * other string; false otherwise.
         */
        bool operator!=(const String& other) const
        {
            return str != other.str;
        }

        /**
         * @brief Lexicographical greater-than-or-equal-to comparison.
         *
         * Determines if this string is lexicographically greater than or equal
         * to the other string.
         *
         * @param[in] other The %String to compare against.
         * @return true if this string is lexicographically greater than or
         * equal to `other`; false otherwise.
         */
        bool operator>=(const String& other) const
        {
            return str >= other.str;
        }

        /**
         * @brief Lexicographical less-than comparison.
         *
         * Determines if this string precedes `other` in lexicographical order.
         *
         * @param[in] other The %String to compare against.
         * @return true if this string is lexicographically less than `other`; false otherwise.
         */
        bool operator<(const String& other) const
        {
            return str < other.str;
        }

        /**
         * @brief Lexicographical greater-than comparison.
         *
         * Determines if this string follows `other` in lexicographical order.
         *
         * @param[in] other The %String to compare against.
         * @return true if this string is lexicographically greater than `other`; false otherwise.
         */
        bool operator>(const String& other) const
        {
            return str > other.str;
        }

         /**
          * @brief Remaps and resolves identifier strings during import.
          *
          * Applies name mapping and, if “@” markers are present, defers label
          * substitution to import-time via PropertyLinkBase::importSubName() and
          * PropertyLinkBase::restoreLabelReference().
          *
          * @param[in] owner   The importing document’s owner object.
          * @param[in] obj     (Optional) Direct pointer to the target object.
          * @param[in] objName (Optional) Name to look up the object if @p obj is null.
          *
          * @sa importSubName(), restoreLabelReference()
          */
        void checkImport(const App::DocumentObject* owner,
                         const App::DocumentObject* obj = nullptr,
                         const String* objName = nullptr);

    private:
        std::string str;
        bool isString;
        bool forceIdentifier;
    };

    /**
     * @brief A component is a part of an ObjectIdentifier.
     *
     * It is used to either name a property or a field within a property. A
     * component can be either a single entry, and array, or a map to other
     * sub-fields.
     */
    class AppExport Component
    {

    private:
        enum typeEnum
        {
            SIMPLE,
            MAP,
            ARRAY,
            RANGE,
        };

    public:
        // Constructors
        FC_DEFAULT_CTORS(Component)
        {
            name = std::move(other.name);
            type = other.type;
            begin = other.begin;
            end = other.end;
            step = other.step;
            return *this;
        }

        /**
         * @brief Construct a Component part.
         *
         * @param[in] _name The name of the component.
         * @param[in] _type The type: `SIMPLE`, `ARRAY`, `RANGE` or `MAP`.
         * @param[in] begin The array index or beginning of a range, or `INT_MAX` for other types.
         * @param[in] end The ending of a range, or `INT_MAX` for other types.
         * @param[in] step The step of a range, or `1` for other types.
         */
        Component(const String& _name = String(),
                  typeEnum _type = SIMPLE,
                  int begin = std::numeric_limits<int>::max(),
                  int end = std::numeric_limits<int>::max(),
                  int step = 1);  // explicit bombs

        /**
         * @brief Construct a Component with move semantics.
         *
         * @param[in,out] _name The name of the component.
         * @param[in] _type The type: `SIMPLE`, `ARRAY`, `RANGE` or `MAP`.
         * @param[in] begin The array index or beginning of a range, or `INT_MAX` for other types.
         * @param[in] end The ending of a range, or `INT_MAX` for other types.
         * @param[in] step The step of a range, or `1` for other types.
         */
        Component(String&& _name,
                  typeEnum _type = SIMPLE,
                  int begin = std::numeric_limits<int>::max(),
                  int end = std::numeric_limits<int>::max(),
                  int step = 1);  // explicit bombs

        /**
         * @brief Create a simple component with the given name.
         *
         * @param[in] _component The name of the component.
         * @return A new Component object.
         */
        static Component SimpleComponent(const char* _component);

        /**
         * @brief Create a simple component with the given name.
         *
         * @param[in] _component The name of the component.
         * @return A new Component object.
         */
        static Component SimpleComponent(const String& _component);

        /**
         * @brief Create a simple component with move semantics.
         *
         * @param[in,out] _component The name of the component.
         * @return A new Component object.
         */
        static Component SimpleComponent(String&& _component);

        /**
         * @brief Create an array component an index.
         *
         * @param[in] _index The index of the component.
         * @return A new Component object.
         */
        static Component ArrayComponent(int _index);

        /**
         * @brief Create a range component with given begin and end.
         *
         * @param[in] _begin The begin index of the range.
         * @param[in] _end The end index of the range.
         * @param[in] _step The step of the range.
         * @return A new Component object.
         */
        static Component RangeComponent(int _begin,
                                        int _end = std::numeric_limits<int>::max(),
                                        int _step = 1);

        /**
         * @brief Create a map component with a given key.
         *
         * @param[in] _key The key of the component.
         * @return A new Component object.
         */
        static Component MapComponent(const String& _key);

        /**
         * @brief Create a map component with move semantics.
         *
         * Create a map component with a given key.
         *
         * @param[in] _key The key of the component.
         * @return A new Component object.
         */
        static Component MapComponent(String&& _key);

        // Type queries

        /**
         * @brief Check if the component is a simple component.
         *
         * @return true if the component is a simple component, false otherwise.
         */
        bool isSimple() const
        {
            return type == SIMPLE;
        }

        /**
         * @brief Check if the component is a map component.
         *
         *@return true if the component is a map component, false otherwise.
         */
        bool isMap() const
        {
            return type == MAP;
        }

        /**
         * @brief Check if the component is an array component.
         *
         * @return true if the component is an array component, false otherwise.
         */
        bool isArray() const
        {
            return type == ARRAY;
        }

        /**
         * @brief Check if the component is a range component.
         *
         * @return true if the component is a range component, false otherwise.
         */
        bool isRange() const
        {
            return type == RANGE;
        }

        // Accessors

        /**
         * @brief Create a string representation of a component.
         *
         * The string is appended to the output stream.
         *
         * @param[in,out] ss The output stream to write to.
         * @param[in] toPython If true, use Python quoting rules. Otherwise, use
         *                     FreeCAD quoting rules.
         */
        void toString(std::ostream& ss, bool toPython = false) const;

        /**
         * @brief Get the name of the component.
         *
         * @return The name of the component.
         */
        const std::string& getName() const
        {
            return name.getString();
        }

        /**
         * @brief Get the index of the component.
         *
         * @return The index of the component.
         */
        int getIndex() const
        {
            return begin;
        }

        /**
         * @brief Get the index given a maximum count.
         *
         * This method interprets the member `begin` as a Python-style index
         * that may be negative.  Given the maximum count it returns a valid
         * index.
         *
         * @param[in] count  The number of elements in the target collection.
         * @return       The index in the range [0, count).
         * @throws Base::IndexError If `begin` is out of bounds.
         */
        size_t getIndex(size_t count) const;

        /**
         * @brief Get the begin index of the component.
         *
         * @return The begin index of the component.
         */
        int getBegin() const
        {
            return begin;
        }

        /**
         * @brief Get the end index of the component.
         *
         * @return The end index of the component.
         */
        int getEnd() const
        {
            return end;
        }

        /**
         * @brief Get the step of the component.
         *
         * @return The step of the component.
         */
        int getStep() const
        {
            return step;
        }

        // Operators

        /**
         * @brief Test the component for equality.
         *
         * @param[in] other The object we want to compare to.
         * @return true if the components are equal, false if not.
         */
        bool operator==(const Component& other) const;

        /**
         * @brief Lexicographical less-than comparison.
         *
         * @param[in] other The object we want to compare to.
         * @return true if this component is lexicographically less than the other.
         */
        bool operator<(const Component& other) const;

        /**
         * @brief Get the value of the component given a Python object.
         *
         * @param[in] pyobj The Python object to get the value from.
         * @return The value of the component.
         */
        Py::Object get(const Py::Object& pyobj) const;

        /**
         * @brief Set the value of the component given a Python object.
         *
         * @param[in,out] pyobj The Python object to set the value to.
         * @param[in] value The value to set.
         */
        void set(Py::Object& pyobj, const Py::Object& value) const;

        /**
         * @brief Delete the value of the component given a Python object.
         *
         * @param[in,out] pyobj The Python object to delete the value from.
         */
        void del(Py::Object& pyobj) const;

    private:
        String name;
        typeEnum type;
        int begin;
        int end;
        int step;
        friend class ObjectIdentifier;
    };

    /**
     * @brief Create a simple component with the given name.
     *
     * @param[in] _component The name of the component.
     * @return A new Component object.
     */
    static Component SimpleComponent(const char* _component)
    {
        return Component::SimpleComponent(_component);
    }

    /**
     * @brief Create a simple component with the given name.
     *
     * @param[in] _component The name of the component.
     * @return A new Component object.
     */
    static Component SimpleComponent(const String& _component)
    {
        return Component::SimpleComponent(_component);
    }

    /**
     * @brief Create a simple component with move semantics.
     *
     * @param[in,out] _component The name of the component.
     * @return A new Component object.
     */
    static Component SimpleComponent(String&& _component)
    {
        return Component::SimpleComponent(std::move(_component));
    }

    /**
     * @brief Create a simple component with the given name.
     *
     * @param[in] _component The name of the component.
     * @return A new Component object.
     */
    static Component SimpleComponent(const std::string _component)
    {
        return Component::SimpleComponent(_component.c_str());
    }

    /**
     * @brief Create an array component with the given index.
     *
     * @param[in] _index The index of the component.
     * @return A new Component object.
     */
    static Component ArrayComponent(int _index)
    {
        return Component::ArrayComponent(_index);
    }

    /**
     * @brief Create a range component with given begin and end.
     *
     * @param[in] _begin The begin index of the range.
     * @param[in] _end The end index of the range.
     * @param[in] _step The step of the range.
     * @return A new Component object.
     */
    static Component RangeComponent(int _begin,
                                    int _end = std::numeric_limits<int>::max(),
                                    int _step = 1)
    {
        return Component::RangeComponent(_begin, _end, _step);
    }

    /**
     * @brief Create a map component with a given key.
     *
     * @param[in] _key The key of the component.
     * @return A new Component object.
     */
    static Component MapComponent(const String& _key)
    {
        return Component::MapComponent(_key);
    }

    /**
     * @brief Create a map component with move semantics.
     *
     * @param[in,out] _key The key of the component.
     * @return A new Component object.
     */
    static Component MapComponent(String&& _key)
    {
        return Component::MapComponent(_key);
    }

    /**
     * @brief Construct an ObjectIdentifier object.
     *
     * Construct an ObjectIdentifier object given an owner and a single-value
     * property, possibly with an array index.
     *
     * @param[in] _owner The owner of the property.
     * @param[in] property The name of the property.
     * @param[in] index The index into the array.
     * @throw Base::RuntimeError if the owner is not a document object.
     */
    explicit ObjectIdentifier(const App::PropertyContainer* _owner = nullptr,
                              const std::string& property = std::string(),
                              int index = std::numeric_limits<int>::max());

    /**
     *@brief Construct an ObjectIdentifier object.
     *
     * @param[in] _owner The owner of the property.
     * @param[in] localProperty If true, the property is a local property.
     * @throw Base::RuntimeError if the owner is not a document object.
     */
    ObjectIdentifier(const App::PropertyContainer* _owner, bool localProperty);

    /**
     * @brief Construct an ObjectIdentifier object given a property.
     *
     * The property is assumed to be single-valued but may have an array index.
     *
     * @param[in] prop The property to construct object identifier for.
     * @param[in] index The index into the array.
     *
     * @throw Base::RuntimeError if the owner is not a document object or if
     * the property does not have a name.
     */
    ObjectIdentifier(const App::Property& prop,
                     int index = std::numeric_limits<int>::max());  // explicit bombs

    FC_DEFAULT_CTORS(ObjectIdentifier)
    {
        owner = other.owner;
        documentName = std::move(other.documentName);
        documentObjectName = std::move(other.documentObjectName);
        subObjectName = std::move(other.subObjectName);
        shadowSub = std::move(other.shadowSub);
        components = std::move(other.components);
        documentNameSet = other.documentNameSet;
        documentObjectNameSet = other.documentObjectNameSet;
        localProperty = other.localProperty;
        _cache = std::move(other._cache);
        _hash = other._hash;
        return *this;
    }

    /// Destruct an ObjectIdentifier object.
    virtual ~ObjectIdentifier() = default;

    /**
     * @brief Get the owner of this object identifier.
     *
     * @return The owner of this object identifier.
     */
    App::DocumentObject* getOwner() const
    {
        return owner;
    }

    /**
     * @brief Add a component to this object identifier.
     *
     * @param[in] c The component to add.
     */
    void addComponent(const Component& c)
    {
        components.push_back(c);
        _cache.clear();
    }

    /**
     * @brief Add a component to this object identifier with move semantics.
     *
     * @param[in] c The component to add.
     */
    void addComponent(Component&& c)
    {
        components.push_back(std::move(c));
        _cache.clear();
    }

    /**
     * @brief Get the name of the property.
     * @return The name of the property.
     */
    std::string getPropertyName() const;

    /**
     * @brief Add components to this object identifier.
     *
     * @tparam C A container type that supports iterators.
     * @param[in] cs The components to add.
     */
    template<typename C>
    void addComponents(const C& cs)
    {
        components.insert(components.end(), cs.begin(), cs.end());
    }

    /**
     * @brief Get a component given an index.
     *
     * @param[in] i: The index of the component.
     * @param[out] idx: Optional return of an adjusted component index.
     * @return A component.
     */
    const Component& getPropertyComponent(int i, int* idx = nullptr) const;

    /**
     * @brief Set a component at an index with move semantics.
     *
     * @param[in] idx: The index to store the component.
     * @param[in,out] comp: The component to set.
     */
    void setComponent(int idx, Component&& comp);

    /**
     * @brief Set a component at an index.
     *
     * @param[in] idx: The index to store the component.
     * @param[in] comp: The component to set.
     */
    void setComponent(int idx, const Component& comp);

    /**
     * @brief Get the property components of this object identifier.
     *
     * @return A vector of components of properties.
     */
    std::vector<Component> getPropertyComponents() const;

    /**
     * @brief Get the components of this object identifier.
     *
     * @return A vector of components.
     */
    const std::vector<Component>& getComponents() const
    {
        return components;
    }

    /**
     * @brief Get a string representation of the subpath.
     *
     * @param[in] toPython If true, use Python quoting rules. Otherwise, use
     *                     FreeCAD quoting rules.
     *
     * @return The string representation of the subpath.
     */
    std::string getSubPathStr(bool toPython = false) const;

    /**
     * @brief Return the number of components.
     *
     * @return The number of components in this identifier.
     */
    int numComponents() const;

    /**
     * @brief Compute the number of sub components.
     *
     * Compute the number of sub components, meaning that this excludes the property.
     *
     * @return Number of components.
     */
    int numSubComponents() const;

    /**
     * @brief Create a string representation of this object identifier.
     *
     * An identifier is written as
     * `document#documentobject.property.subproperty1...subpropertyN`.  The
     * string `document#` may be dropped; in that case it is assumed to be
     * within owner's document.  If `documentobject` is dropped, the property
     * is assumed to be owned by the owner specified in the object identifiers
     * constructor.
     *
     * @return A string representation of the object identifier.
     */
    const std::string& toString() const;

    /**
     * @brief Create a persistent string representation of this object identifier.
     *
     * The persistent string representation is used where the object identifier
     * is required to survive import and export.
     *
     * @see toString()
     *
     * @return A persistent string representation of the object identifier.
     */
    std::string toPersistentString() const;

    /**
     * @brief Create an escapedstring representation of this object identifier.
     *
     * The escaped string representation is suitable for being embedded in a
     * Python command.
     *
     * @return The escaped string representation.
     */
    std::string toEscapedString() const;

    /**
     * @brief Whether the property of the object identifier is touched.
     *
     * This method is used to determine if the property that this object
     * identifier represents is touched.
     *
     * @return true if the property of the object identifier is touched, false
     * otherwise.
     */
    bool isTouched() const;

    /**
     * @brief Get the property this object identifier represents.
     *
     * @param[out] ptype Optional return of the property type.
     *
     * @return A pointer to property if it is uniquely defined, or `nullptr` otherwise.
     */
    App::Property* getProperty(int* ptype = nullptr) const;

    /**
     * @brief Create a canonical representation of the object identifier.
     *
     * The main work is actually done by the property's virtual
     * Property::canonicalPath() method that is invoked by this call.
     *
     * @return A new object identifier.
     */
    App::ObjectIdentifier canonicalPath() const;

    // Document-centric functions

    /**
     * @brief Set the document name for this object identifier.
     *
     * If @p force is true, the document name will always be included in the
     * string representation.
     *
     * @param[in,out] name The name of the document.
     * @param[in] force Force the name to be set.
     */
    void setDocumentName(String&& name, bool force = false);

    /**
     * @brief Get the document name from this object identifier
     *
     * @return The document name as a String object.
     */
    String getDocumentName() const;

    /**
     * @brief Set the document object name of this object identifier.
     *
     * If force is true, the document object will not be resolved dynamically
     * from the object identifier's components, but used as given by this
     * method.
     *
     * This function uses move semantics on @p name and @p subname.
     *
     * @param[in,out] name The name of document object.
     * @param[in] force Force the name to be set.
     * @param[in,out] subname The name of the subobject.
     * @param[in] checkImport If true, check for import.
     */
    void setDocumentObjectName(String&& name,
                               bool force = false,
                               String&& subname = String(),
                               bool checkImport = false);

    /**
     * @brief Set the document object name of this object identifier.
     *
     * If force is true, the document object will not be resolved dynamically
     * from the object identifier's components, but used as given by this method.
     *
     * @param[in] obj The document object which name is set in the object identifier.
     * @param[in] force Force the name to be set.
     * @param[in,out] subname The name of the subobject using move semantics.
     * @param[in] checkImport If true, check for import.
     */
    void setDocumentObjectName(const App::DocumentObject* obj,
                               bool force = false,
                               String&& subname = String(),
                               bool checkImport = false);

    /**
     * @brief Whether the object identifier has a document object name.
     *
     * This method checks if the object identifier has a document object name
     * and optionally checks if the name was forced.
     *
     * @param[in] forced If true, check if the name was forced.
     * @return true if the object identifier has a document object name, false
     * otherwise.
     */
    bool hasDocumentObjectName(bool forced = false) const;

    /**
     * @brief Test whether the property is local.
     *
     * This method checks whether the property this object identifier
     * represents is a local property.
     *
     * @return true if the property is local, false otherwise.
     */
    bool isLocalProperty() const
    {
        return localProperty;
    }

    /**
     * @brief Get the document object name.
     *
     * @return String with name of document object as resolved by the object
     * identifier.
     */
    String getDocumentObjectName() const;

    /**
     * @brief Get the subobject name.
     *
     * @param[in] newStyle If true, use the new style of subobject name.
     * @return The subobject name.
     */
    const std::string& getSubObjectName(bool newStyle) const;

    /**
     * @brief Get the subobject name.
     *
     * @return The subobject name.
     */
    const std::string& getSubObjectName() const;

    /**
     * @brief A type for a map of subobject names.
     *
     * The map is a map with keys of a pair of document object and a
     * subobject name to the subobject name for the imported object.
     */
    using SubNameMap = std::map<std::pair<App::DocumentObject*, std::string>, std::string>;

    /**
     * @brief Import subnames from a map of subnames.
     *
     * Given a map of subnames from the linked subobject to the imported
     * subobject, this method will update the subnames in the object
     * identifier.
     *
     * @param[in] subNameMap The map of subnames to import.
     */
    void importSubNames(const SubNameMap& subNameMap);

    /**
     * @brief Update the label reference.
     *
     * This method updates the label reference of the object identifier.
     *
     * @param[in] obj The document object that owns the label.
     * @param[in] ref The old label reference.
     * @param[in] newLabel The new label to set.
     */
    bool updateLabelReference(const App::DocumentObject* obj,
                              const std::string& ref,
                              const char* newLabel);

    /**
     * @brief Relabel the document name.
     *
     * This method relabels the document name of the object identifier as part
     * of a visit of the ExpressionVisitor.
     *
     * @param[in,out] v The expression visitor.
     * @param[in] oldLabel The old label to relabel.
     * @param[in] newLabel The new label to set.
     */
    bool relabeledDocument(ExpressionVisitor& v,
                           const std::string& oldLabel,
                           const std::string& newLabel);

    /**
     * @brief A type for storing dependencies of an ObjectIdentifier.
     *
     * It is a map from document object to a set of property names.  An object
     * identifier may references multiple objects using syntax like
     * `%Part.%Group[0].Width`.
     *
     * Additionally, we use a set of strings instead of set of Properties,
     * because the property may not exist at the time this ObjectIdentifier is
     * constructed.
     */
    using Dependencies = std::map<App::DocumentObject*, std::set<std::string>>;

    /**
     * @brief Get the dependencies of this object identifier.
     *
     * @param[in] needProps: Whether we need property dependencies.
     * @param[out] labels: Optional return of any label references.
     *
     * In case of multi-object references, like `%Part.%Group[0].Width`, if no
     * property dependency is required, then this function will only return the
     * first referred object dependency. Or else, all object and property
     * dependencies will be returned.
     */
    Dependencies getDep(bool needProps, std::vector<std::string>* labels = nullptr) const;

    /**
     * @brief Get the dependencies of this object identifier.
     *
     * @param[in,out] deps: Returns the dependencies.
     * @param[in] needProps: Whether need property dependencies.
     * @param[out] labels: Optional return of any label references.
     *
     * @see ObjectIdentifier::getDep(bool,std::vector<std::string>*) const
     */
    void
    getDep(Dependencies& deps, bool needProps, std::vector<std::string>* labels = nullptr) const;

    /**
     * @brief Returns all label references in the object identifier.
     *
     * @param[in,out] labels The container in which the labels are returned.
     */
    void getDepLabels(std::vector<std::string>& labels) const;

    /**
     * @brief Find a document with the given name.
     *
     * This method will search for a document with the given name. If @p name
     * is not provided, the document of the object identifier is returned.  If
     * @p ambiguous is not `nullptr`, the method will return whether the found
     * document is unique.
     *
     * @param[in] name The name of the document.
     * @param[out] ambiguous If true, the document is not uniquely defined by name.
     *
     * @return The found document, or `nullptr` if it is not found or not
     * uniquely defined by name.
     */
    App::Document* getDocument(String name = String(), bool* ambiguous = nullptr) const;

    /**
     * @brief Get the document object for the object identifier.
     * @return The document object, or `nullptr` if not found or uniquely defined.
     */
    App::DocumentObject* getDocumentObject() const;

    /**
     * @brief Get the parts of the object identifier as a string list.
     *
     * The parts include the document name, the document object name, the
     * subobject name, and the various components.
     *
     * @return The list of strings representing the various components.
     */
    std::vector<std::string> getStringList() const;

    /**
     * @brief Construct the simplest possible object identifier relative to another.
     *
     * @param[in] other The other object identifier.
     * @return A new simplified object identifier.
     */
    App::ObjectIdentifier relativeTo(const App::ObjectIdentifier& other) const;

    /**
     * @brief Replace an object in the object identifier.
     *
     * This method replaces an object in the object identifier with another
     * object. It is used to create a new object identifier when the document
     * object is replaced.
     *
     * @param[out] res The resulting object identifier.
     * @param[in] parent The parent document object.
     * @param[in] oldObj The old document object to replace.
     * @param[in] newObj The new document object to replace with.
     *
     * @return true if the object was replaced, false otherwise.
     */
    bool replaceObject(ObjectIdentifier& res,
                       const App::DocumentObject* parent,
                       App::DocumentObject* oldObj,
                       App::DocumentObject* newObj) const;

    // Operators

    /**
     * @brief Operator to add a component to the object identifier.
     *
     * @param[in] value Component object
     *
     * @return A reference to itself.
     */
    App::ObjectIdentifier& operator<<(const Component& value);

    /**
     * @brief Operator to add a component to the object identifier.
     *
     * This method uses move semantics.
     *
     * @param[in,out] value Component object
     *
     * @return A reference to itself.
     */
    App::ObjectIdentifier& operator<<(Component&& value);

    /**
     * @brief Compare object identifiers for equality.
     *
     * @param[in] other The other object identifier.
     *
     * @return true if they are equal, false otherwise.
     */
    bool operator==(const ObjectIdentifier& other) const;

    /**
     * @brief Compare object identifiers for inequality.
     *
     * @param[in] other The other object identifier.
     *
     * @return true if they differ from each other, false otherwise.
     */
    bool operator!=(const ObjectIdentifier& other) const;

    /**
     * @brief Lexicographical less-than comparison.
     *
     * @param[in] other The other object identifier.
     *
     * @return true if this object is less than the other.
     */
    bool operator<(const ObjectIdentifier& other) const;

    // Getter

    /**
     * @brief Get the value of the property or field pointed to by this object
     * identifier.
     *
     * All type of objects are supported. Some types are casted to FC native
     * type, including: Int, Float, %String, Unicode %String, and Quantities. Others
     * are just kept as Python object wrapped by App::any.
     *
     * @param[in] pathValue: if true, calls the property's getPathValue(), which is
     * necessary for quantities to work.
     * @param[in] isPseudoProperty: if not `nullptr`, set to true if the property is a
     * pseudo property.
     *
     * @return The value of the property or field.
     */
    App::any getValue(bool pathValue = false, bool* isPseudoProperty = nullptr) const;

    /**
     * @brief Get the value of the property or field pointed to by this object
     * identifier.
     *
     * @see ObjectIdentifier::getValue().  In contrast, this method
     * returns a Python %object.
     *
     * @param[in] pathValue: if true, calls the property's getPyPathValue(), which is
     * necessary for quantities to work.
     * @param[in] isPseudoProperty: if not `nullptr`, set to true if the property is a
     * pseudo property.
     *
     * @return The value of the property or field.
     */
    Py::Object getPyValue(bool pathValue = false, bool* isPseudoProperty = nullptr) const;

    // Setter: is const because it does not alter the object state,
    // but does have an aiding effect.

    /**
     * @brief Set the value of a property or field pointed to by this object identifier.
     *
     * This method uses Python to do the actual work and a limited set of types
     * that can be in the `App::any` variable are supported: `Base::Quantity`,
     * `double`, `char*`, `const char*`, `int`, `unsigned int`, `short`, `unsigned short`,
     * `char`, and `unsigned char`.
     *
     * @param[in] value The value to set.
     */
    void setValue(const App::any& value) const;

    // Static functions

    /**
     * @brief Parse a string to create an object identifier.
     *
     * @param[in] docObj Document object that will own this object identifier.
     * @param[in] str String to parse
     *
     * @return A new object identifier.
     * @throw Base::RuntimeError if the string is invalid.
     */
    static ObjectIdentifier parse(const App::DocumentObject* docObj, const std::string& str);

    /**
     * @brief Acquire the error string after name resolution of the object identifier.
     *
     * @return The error string.
     */
    std::string resolveErrorString() const;

    /**
     * @brief Adjust the links of the object identifier.
     *
     * This method will adjust the links of the object identifier and is part
     * of a visit from an expression visitor.  This is typically necessary when
     * a link object is moved.  The @p inList is a list of dependencies of the
     * object for which the visitor was started.
     *
     * @param[in,out] v The expression visitor.
     * @param[in] inList The list of dependencies.
     *
     * @return true if the links were adjusted, false otherwise.
     */
    bool adjustLinks(ExpressionVisitor& v, const std::set<App::DocumentObject*>& inList);

    /**
     * @brief Update the element reference of the object identifier.
     *
     * This method is part of the expression visitor and is used to update the
     * element reference of object identifiers in case of a geometry element
     * reference change due to geometry model changes.
     *
     * @see PropertyLinkBase::_updateElementReference()
     *
     * @param[in,out] v The expression visitor.
     * @param[in,out] feature If given, then only update element references for this
     * feature, otherwise update geometry element references.
     * @param[in] reverse If true, use the old style before the new style. If
     * false the other way around.
     *
     * @return true if the element reference was updated, false otherwise.
     */
    bool updateElementReference(ExpressionVisitor& v,
                                App::DocumentObject* feature = nullptr,
                                bool reverse = false);


    /// Resolve ambiguity in the object identifier.
    void resolveAmbiguity();

    /**
     * @brief Verify the object identifier.
     *
     * This method will verify the object identifier against property @p prop.
     *
     * @param[in] prop The property to verify against.
     * @param[in] silent If true, do not throw an exception.
     *
     * @return true if verification succeeded, false otherwise.
     * @throw Base::ValueError if there is an invalid property path.
     */
    bool verify(const App::Property& prop, bool silent = false) const;

    /**
     * @brief Compute the hash of the object identifier.
     *
     * This method computes the hash of the object identifier.  The hash is
     * computed from the string representation of the object identifier.
     *
     * @return The hash of the object identifier.
     */
    std::size_t hash() const;

protected:
    /**
     * @brief A structure to hold the results of resolving an ObjectIdentifier.
     *
     * This structure is used to hold the results of resolving an object
     * identifier.
     */
    struct ResolveResults
    {

        /**
         * @brief Construct and initialize a ResolveResults object, given an ObjectIdentifier instance.
         *
         * The constructor will invoke the ObjectIdentifier's resolve() method
         * to initialize the object's data.
         */
        explicit ResolveResults(const ObjectIdentifier& oi);

        int propertyIndex {0};
        App::Document* resolvedDocument {nullptr};
        String resolvedDocumentName;
        App::DocumentObject* resolvedDocumentObject {nullptr};
        String resolvedDocumentObjectName;
        String subObjectName;
        App::DocumentObject* resolvedSubObject {nullptr};
        App::Property* resolvedProperty {nullptr};
        std::string propertyName;
        int propertyType {0};
        std::bitset<32> flags;

        /**
         * @brief Acquire the error string after name resolution.
         *
         * @return The error string.
         */
        std::string resolveErrorString() const;

        /**
         * @brief Obtain the property of the object identifier for this ResolveResults.
         *
         * @param[in] oi The object identifier.
         */
        void getProperty(const ObjectIdentifier& oi);
    };


    /// Provide access to the internals of ObjectIdentifier.
    friend struct ResolveResults;

    /**
     * @brief Resolve the property of the object identifier.
     *
     * This method will resolve the property of the object identifier and
     * return the property type.
     *
     * @param[in] obj The document object that owns the property.
     * @param[in] propertyName The name of the property to resolve.
     * @param[out] sobj The sub object.
     * @param[out] ptype The type of the property.
     *
     * @return The property or `nullptr` if not resolved.
     */
    App::Property* resolveProperty(const App::DocumentObject* obj,
                                   const char* propertyName,
                                   App::DocumentObject*& sobj,
                                   int& ptype) const;

    /**
     * @brief Get sub field part of a property as a string.
     *
     * @param[in,out] ss The output stream to write to.
     * @param[in] result The resolve results.
     * @param[in] toPython If true, use Python quoting rules. Otherwise, use
     *                     FreeCAD quoting rules.
     */
    void getSubPathStr(std::ostream& ss, const ResolveResults& result, bool toPython = false) const;

    /**
     * @brief Access the value of the property or field pointed to by this
     * object.
     *
     * This method can either set or get a value of the property or field of
     * the object identifier given a ResolveResults.  If @p deps is not
     * `nullptr`, the dependencies are updated.
     *
     * @param[in] rs The resolve results.
     * @param[in] value The value to set.
     * @param[in,out] deps The dependencies to set.
     */
    Py::Object access(const ResolveResults& rs,
                      const Py::Object* value = nullptr,
                      Dependencies* deps = nullptr) const;

    /**
     * @brief Resolve the object identifier to a concrete document, document
     * object, and property.
     *
     * This method is a helper method that fills out data in the given
     * ResolveResults object.
     *
     * @param[in,out] results The ResolveResults object to fill out.
     */
    void resolve(ResolveResults& results) const;

    /**
     * @brief Resolve ambiguity in the object identifier.
     *
     * Given a ResolveResults, the ambiguities in the object identifier are resolved.
     *
     * @param[in] results The result of the resolve method.
     */
    void resolveAmbiguity(const ResolveResults& results);

    /**
     * @brief Returns all label references in the object identifier.
     *
     * @param[in] result The result of the resolve method.
     * @param[in,out] labels The container in which the labels are returned.
     */
    void getDepLabels(const ResolveResults& result, std::vector<std::string>& labels) const;

    /// The owner of the object identifier.
    App::DocumentObject* owner;
    /// The document name that this object identifier refers to.
    String documentName;
    /// The document object name that this object identifier refers to.
    String documentObjectName;
    /// The sub object name that this object identifier refers to.
    String subObjectName;
    /**
     * @brief The shadow sub-element names that this object identifier refers to.
     *
     * This contains both the new and old style sub-element names.
     */
    ElementNamePair shadowSub;
    /// The components of the object identifier.
    std::vector<Component> components;
    /// Whether a document name is forced set.
    bool documentNameSet;
    /// Whether a document object name is forced set.
    bool documentObjectNameSet;
    /// Whether the property is local.
    bool localProperty;

private:
    static App::DocumentObject*
    getDocumentObject(const App::Document* doc, const String& name, std::bitset<32>& flags);

private:
    std::string _cache;  // Cached string represstation of this identifier
    std::size_t _hash;   // Cached hash of this string
};

/**
 * @brief Hash function for ObjectIdentifier.
 *
 * @param[in] path The object identifier to hash.
 *
 * @return The hash value of the object identifier.
 */
inline std::size_t hash_value(const App::ObjectIdentifier& path)
{
    return path.hash();
}

//@{
/**
 * @brief Helper function to convert Python object to App::any
 *
 * @warning Must hold Python global interpreter lock before calling these
 * functions
 *
 * @param[in] pyobj The Python object to convert.
 * @param[in] check If true, check if the object is convertible to App::any,
 * otherwise just return as is.
 *
 * @return The converted App::any object.
 * @throw Base::ValueError if the object is not convertible to a unicode string.
 */
App::any AppExport pyObjectToAny(Py::Object pyobj, bool check = true);

/**
 * @brief Helper function to convert Python object from App::any
 *
 * @warning Must hold Python global interpreter lock before calling these
 * functions
 *
 * @param[in] value The App::any object to convert.
 * @return The converted Python object.
 * @throw Base::ExpressionError if the value has an unknown type.
 */
Py::Object AppExport pyObjectFromAny(const App::any& value);
//@}
}  // namespace App

namespace std
{

template<>
struct hash<App::ObjectIdentifier>
{
    using argument_type = App::ObjectIdentifier;
    using result_type = std::size_t;
    inline result_type operator()(argument_type const& s) const
    {
        return s.hash();
    }
};
}  // namespace std
