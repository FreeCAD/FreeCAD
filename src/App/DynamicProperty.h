/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_DYNAMICPROPERTY_H
#define APP_DYNAMICPROPERTY_H

#include <Base/Persistence.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>

#include <boost/functional/hash.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Base {
class Writer;
class XMLWriter;
}

namespace App
{
class Property;
class PropertyContainer;

namespace bmi = boost::multi_index;

struct CStringHasher {
    inline std::size_t operator()(const char *s) const {
        return boost::hash_range(s,s+std::strlen(s));
    }
    inline bool operator()(const char *a, const char *b) const {
        return std::strcmp(a,b)==0;
    }
};

/** This class implements an interface to add properties at run-time to an object
 * @author Werner Mayer
 */
class AppExport DynamicProperty
{
public:
    DynamicProperty();
    virtual ~DynamicProperty();

    /** @name Access properties */
    //@{
    /// Get all properties of the class (including parent)
    void getPropertyList(std::vector<Property*> &List) const;
    /// Get all properties of the class (including parent)
    void getPropertyMap(std::map<std::string,Property*> &Map) const;
    /// Find a dynamic property by its name
    Property *getDynamicPropertyByName(const char* name) const;
    /*!
      Add a dynamic property of the type @a type and with the name @a name.
      @a Group gives the grouping name which appears in the property editor and
      @a doc shows the tooltip there.
      With @a attr, @a ro and @a hidden the behaviour of the property can be controlled.
      @a attr is an OR'ed value of the PropertyType enumeration.
      If no special attribute should be set Prop_None can be set (or leave the default of 0).
      For convenience the attributes for 'Read-Only' and 'Hidden' can also be controlled with
      the values @a ro or @a hidden. This means,
      @code
       addDynamicProperty(..., ..., "Base","blah", Prop_ReadOnly | Prop_Hidden);
      @endcode
      is equivalent to
      @code
       addDynamicProperty(..., ..., "Base","blah", Prop_None, true, true);
      @endcode
     */
    Property* addDynamicProperty(PropertyContainer &pc, const char* type, const char* name=0, const char* group=0,
                                 const char* doc=0, short attr=0, bool ro=false, bool hidden=false);
    /** Add a pre-existing property
     *
     * The property is not treated as dynamic, and will not trigger signal.
     *
     * @return Return false if there is a property exist with the same name.
     */
    bool addProperty(Property *prop);
    /*!
      Removes a dynamic property by name. Returns true if the property is part of the container, otherwise
      false is returned.
     */
    bool removeDynamicProperty(const char* name);
    /// Remove pre-existing property, which will not be deleted.
    bool removeProperty(const Property *prop);
    /// Get a list of all dynamic properties.
    std::vector<std::string> getDynamicPropertyNames() const;
    /// Get the name of a property
    const char* getPropertyName(const Property* prop) const;
    //@}

    /** @name Property attributes */
    //@{
    /// Get the attributes of a property
    short getPropertyType(const Property* prop) const;
    /// Get the attributes of a named property
    short getPropertyType(const char *name) const;
    /// Get the group name of a property
    const char* getPropertyGroup(const Property* prop) const;
    /// Get the group name of a named property
    const char* getPropertyGroup(const char *name) const;
    /// Get the documentation of a property
    const char* getPropertyDocumentation(const Property* prop) const;
    /// Get the documentation of a named property
    const char* getPropertyDocumentation(const char *name) const;
    //@}

    /// Remove all properties
    void clear();

    /// Get property count
    size_t size() const { return props.size(); }

    void save(const Property *prop, Base::Writer &writer) const;

    Property *restore(PropertyContainer &pc, 
        const char *PropName, const char *TypeName, Base::XMLReader &reader);

    struct PropData {
        Property* property;
        std::string name;
        const char *pName;
        std::string group;
        std::string doc;
        short attr;
        bool readonly;
        bool hidden;

        PropData(Property *prop=0, std::string &&n=std::string(), const char *pn=0,
                const char *g=0, const char *d=0, short a=0, bool ro=false, bool h=false)
            :property(prop),name(std::move(n)),pName(pn)
            ,group(g?g:""),doc(d?d:""),attr(a),readonly(ro),hidden(h)
        {}

        const char *getName() const {
            return pName?pName:name.c_str();
        }
    };

    PropData getDynamicPropertyData(const Property* prop) const;

private:
    std::string getUniquePropertyName(PropertyContainer &pc, const char *Name) const;

private:
    bmi::multi_index_container<
        PropData,
        bmi::indexed_by<
            bmi::hashed_unique<
                bmi::const_mem_fun<PropData, const char*, &PropData::getName>,
                CStringHasher,
                CStringHasher
            >,
            bmi::hashed_unique<
                bmi::member<PropData, Property*, &PropData::property>
            >
        >
    > props;
};

} // namespace App

#endif // APP_DYNAMICPROPERTY_H
