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
#include <map>
#include <vector>
#include <string>

namespace Base {
class Writer;
class XMLWriter;
}

namespace App
{
class Property;
class PropertyContainer;

/** This class implements an interface to add properties at run-time to an object
 * derived from PropertyContainer. The additional properties are made persistent.
 * @author Werner Mayer
 */
class AppExport DynamicProperty : public Base::Persistence
{
public:
    struct PropData {
        Property* property;
        std::string group;
        std::string doc;
        short attr;
        bool readonly;
        bool hidden;
    };

    DynamicProperty(PropertyContainer* pc);
    virtual ~DynamicProperty();

    /** @name Access properties */
    //@{
    /// get all properties of the class (including parent)
    void getPropertyList(std::vector<Property*> &List) const;
    /// get all properties of the class (including parent)
    void getPropertyMap(std::map<std::string,Property*> &Map) const;
    /// find a property by its name
    Property *getPropertyByName(const char* name) const;
    /// find a property by its name
    Property *getDynamicPropertyByName(const char* name) const;
    Property* addDynamicProperty(const char* type, const char* name=0, const char* group=0,
                                 const char* doc=0, short attr=0, bool ro=false, bool hidden=false);
    bool removeDynamicProperty(const char* name);
    std::vector<std::string> getDynamicPropertyNames() const;
    void addDynamicProperties(const PropertyContainer*);
    /// get the name of a property
    const char* getPropertyName(const Property* prop) const;
    //@}

    /** @name Property attributes */
    //@{
    /// get the Type of a Property
    short getPropertyType(const Property* prop) const;
    /// get the Type of a named Property
    short getPropertyType(const char *name) const;
    /// get the Group of a Property
    const char* getPropertyGroup(const Property* prop) const;
    /// get the Group of a named Property
    const char* getPropertyGroup(const char *name) const;
    /// get the Group of a Property
    const char* getPropertyDocumentation(const Property* prop) const;
    /// get the Group of a named Property
    const char* getPropertyDocumentation(const char *name) const;
    /// check if the property is read-only
    bool isReadOnly(const Property* prop) const;
    /// check if the named property is read-only
    bool isReadOnly(const char *name) const;
    /// check if the property is hidden
    bool isHidden(const Property* prop) const;
    /// check if the named property is hidden
    bool isHidden(const char *name) const;
    //@}

    /** @name Property serialization */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    unsigned int getMemSize (void) const;
    //@}

private:
    /// Encodes an attribute upon saving.
    std::string encodeAttribute(const std::string&) const;
    std::string getUniquePropertyName(const char *Name) const;

private:
    PropertyContainer* pc;
    std::map<std::string,PropData> props;
};

} // namespace App

#endif // APP_DYNAMICPROPERTY_H
