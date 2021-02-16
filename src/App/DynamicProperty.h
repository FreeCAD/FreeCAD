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
#include <App/Property.h>

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
        if(!s) return 0;
        return boost::hash_range(s,s+std::strlen(s));
    }
    inline bool operator()(const char *a, const char *b) const {
        if(!a) return !b;
        if(!b) return false;
        return std::strcmp(a,b)==0;
    }
};

/** This class implements an interface to add properties at run-time to an object
 * derived from PropertyContainer. The additional properties are made persistent.
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
    Property *getDynamicPropertyByName(const std::string& name) const;
    /*!
      Add a dynamic property of the type @a type and with the name @a name.
      @a Group gives the grouping name which appears in the property editor and
      @a doc shows the tooltip there.
      The new property has status Prop_Dynamic set to true
     */
    Property* addDynamicProperty(
        PropertyContainer &pc,
        const std::string& type,
        const std::string& name,
        const std::string& group,
        const std::string& doc
        );
    /*!
      Removes a dynamic property by name. Returns true if the property is part of the container, otherwise
      false is returned.
     */
    bool removeDynamicProperty(const std::string& name);
    /// Remove pre-existing property, which will not be deleted.
    bool removeProperty(const Property *prop);
    /// Get a list of all dynamic properties.
    std::vector<std::string> getDynamicPropertyNames() const;
    //@}

    /// Remove all properties
    void clear();

    /// Get property count
    size_t size() const { return props.size(); }

    void save(const Property *prop, Base::Writer &writer) const;

    Property *restore(PropertyContainer &pc,
        const char *PropName, const char *TypeName, Base::XMLReader &reader);

    struct PropData : public  App::PropertySpec {
        Property* property;

        PropData(Property *prop, const std::string& pn,
                const std::string& g, const std::string& d)
            :PropertySpec(pn,g,d,-1),property(prop)
        {}
    };

private:
    std::string getUniquePropertyName(PropertyContainer &pc, const std::string& Name) const;

private:
    bmi::multi_index_container<
        std::shared_ptr<PropData>,
        bmi::indexed_by<
            bmi::hashed_unique<
                bmi::member<PropertySpec, const std::string , &PropData::Name>
            >,
            bmi::hashed_unique<
                bmi::member<PropData, Property*, &PropData::property>
            >
        >
    > props;
};

} // namespace App

#endif // APP_DYNAMICPROPERTY_H
