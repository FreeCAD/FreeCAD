/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef APP_EXTENSIONCONTAINER_H
#define APP_EXTENSIONCONTAINER_H

#include "PropertyContainer.h"


namespace App {

class Extension;
/**
 * @brief Container which can hold extensions
 *
 * In FreeCAD normally inheritance is a chain, it is not possible to use multiple inheritance.
 * The reason for this is that all objects need to be exposed to python, and it is basically
 * impossible to handle multiple inheritance in the C-API for python extensions. Also using multiple
 * parent classes in python is currently not possible with the default object approach.
 *
 * The concept of extensions allow to circumvent those problems. Extensions are FreeCAD objects
 * which work like normal objects in the sense that they use properties and class methods to define
 * their functionality. However, they are not exposed as individual usable entities but are used to
 * extend other objects. A extended object gets all the properties and methods of the extension.
 * Therefore it is like c++ multiple inheritance, which is indeed used to achieve this on c++ side,
 * but provides a few important additional functionalities:
 * - Property persistence is handled, save and restore work out of the box
 * - The objects python API gets extended too with the extension python API
 * - Extensions can be added from c++ and python, even from both together
 *
 * The interoperability with python is highly important, as in FreeCAD all functionality should be
 * as easily accessible from python as from c++. To ensure this, and as already noted, extensions can
 * be added to a object from python. However, this means that it is not clear from the c++ object type
 * if an extension was added or not. If added from c++ it becomes clear in the type due to the use of
 * multiple inheritance. If added from python it is a runtime extension and not visible from type.
 * Hence querying existing extensions of an object and accessing its methods works not by type
 * casting but by the interface provided in ExtensionContainer. The default workflow is to query if
 * an extension exists and then get the extension object. No matter if added from python or c++ this
 * interface works always the same.
 * @code
 * if (object->hasExtension(GroupExtension::getClassTypeId())) {
 *     App::GroupExtension* group = object->getExtensionByType<GroupExtension>();
 *     group->hasObject(...);
 * }
 * @endcode
 *
 * To add a extension to an object, it must comply to a single restriction: it must be derived
 * from ExtensionContainer. This is important to allow adding extensions from python and also to
 * access the universal extension API. As DocumentObject itself derives from ExtensionContainer this
 * should be the case automatically in most circumstances.
 *
 * Note that two small boilerplate changes are needed next to the multiple inheritance when adding
 * extensions from c++.
 * 1. It must be ensured that the property and type registration is aware of the extensions by using
 *    special macros.
 * 2. The extensions need to be initialised in the constructor
 *
 * Here is a working example:
 * @code
 * class AppExport Part : public App::DocumentObject, public App::FirstExtension, public App::SecondExtension {
 *   PROPERTY_HEADER_WITH_EXTENSIONS(App::Part);
 * };
 * PROPERTY_SOURCE_WITH_EXTENSIONS(App::Part, App::DocumentObject)
 * Part::Part(void) {
 *   FirstExtension::initExtension(this);
 *   SecondExtension::initExtension(this);
 * }
 * @endcode
 *
 * From python adding an extension is easier, it must be simply registered to a document object
 * at object initialisation like done with properties. Note that the special python extension objects
 * need to be added, not the c++ objects. Normally the only difference in name is the additional
 * "Python" at the end of the extension name.
 * @code{.py}
 * class Test():
 *   __init(self)__:
 *     registerExtension("App::FirstExtensionPython", self)
 *     registerExtension("App::SecondExtensionPython", self)
 * @endcode
 *
 * Extensions can provide methods that should be overridden by the extended object for customisation
 * of the extension behaviour. In c++ this is as simple as overriding the provided virtual functions.
 * In python a class method must be provided which has the same name as the method to override. This
 * method must not necessarily be in the object that is extended, it must be in the object which is
 * provided to the "registerExtension" call as second argument. This second argument is used as a
 * proxy and enqueired if the method to override exists in this proxy before calling it.
 *
 * For information on howto create extension see the documentation of Extension
 */
class AppExport ExtensionContainer : public App::PropertyContainer
{

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:

    using ExtensionIterator = std::map<Base::Type, App::Extension*>::iterator;

    ExtensionContainer();
    ~ExtensionContainer() override;

    void registerExtension(Base::Type extension, App::Extension* ext);
    bool hasExtension(Base::Type, bool derived=true) const; //returns first of type (or derived from if set to true) and throws otherwise
    bool hasExtension(const std::string& name) const; //this version does not check derived classes
    bool hasExtensions() const;
    App::Extension* getExtension(Base::Type, bool derived = true, bool no_except=false) const;
    App::Extension* getExtension(const std::string& name) const; //this version does not check derived classes
    // this version checks for derived types and doesn't throw
    template<typename ExtensionT>
    ExtensionT* getExtension() const {
        return static_cast<ExtensionT*>(getExtension(ExtensionT::getExtensionClassTypeId(), true, true));
    }

    //returns first of type (or derived from) and throws otherwise
    template<typename ExtensionT>
    ExtensionT* getExtensionByType(bool no_except=false, bool derived=true) const {
        return static_cast<ExtensionT*>(getExtension(ExtensionT::getExtensionClassTypeId(),derived,no_except));
    }

    //get all extensions which have the given base class
    std::vector<Extension*> getExtensionsDerivedFrom(Base::Type type) const;
    template<typename ExtensionT>
    std::vector<ExtensionT*> getExtensionsDerivedFromType() const {
        std::vector<ExtensionT*> typevec;
        for(const auto& entry : _extensions) {
            if(entry.first.isDerivedFrom(ExtensionT::getExtensionClassTypeId()))
                typevec.push_back(static_cast<ExtensionT*>(entry.second));
        }
        return typevec;
    }

    ExtensionIterator extensionBegin() {return _extensions.begin();}
    ExtensionIterator extensionEnd() {return _extensions.end();}


    /** @name Access properties */
    //@{
    /// find a property by its name
    Property *getPropertyByName(const char* name) const override;
    /// get the name of a property
    const char* getPropertyName(const Property* prop) const override;
    /// get all properties of the class (including properties of the parent)
    void getPropertyMap(std::map<std::string,Property*> &Map) const override;
    /// get all properties of the class (including properties of the parent)
    void getPropertyList(std::vector<Property*> &List) const override;

    /// get the Type of a Property
    short getPropertyType(const Property* prop) const override;
    /// get the Type of a named Property
    short getPropertyType(const char *name) const override;
    /// get the Group of a Property
    const char* getPropertyGroup(const Property* prop) const override;
    /// get the Group of a named Property
    const char* getPropertyGroup(const char *name) const override;
    /// get the Group of a Property
    const char* getPropertyDocumentation(const Property* prop) const override;
    /// get the Group of a named Property
    const char* getPropertyDocumentation(const char *name) const override;
    //@}

    void onChanged(const Property*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    //those methods save/restore the dynamic extensions without handling properties, which is something
    //done by the default Save/Restore methods.
    void saveExtensions(Base::Writer& writer) const;
    void restoreExtensions(Base::XMLReader& reader);

    /** Extends the rules for handling property name changed, so that extensions are given an opportunity to handle it.
     *  If an extension handles a change, neither the rest of the extensions, nor the container itself get to handle it.
     *
     *  Extensions get their extensionHandleChangedPropertyName() called.
     *
     *  If no extension handles the request, then the containers handleChangedPropertyName() is called.
     */
    void handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName) override;
    /** Extends the rules for handling property type changed, so that extensions are given an opportunity to handle it.
     *  If an extension handles a change, neither the rest of the extensions, nor the container itself get to handle it.
     *
     *  Extensions get their extensionHandleChangedPropertyType() called.
     *
     *  If no extension handles the request, then the containers handleChangedPropertyType() is called.
     */
    void handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, Property * prop) override;

private:
    //stored extensions
    std::map<Base::Type, App::Extension*> _extensions;
};

#define PROPERTY_HEADER_WITH_EXTENSIONS(_class_) \
  PROPERTY_HEADER_WITH_OVERRIDE(_class)

/// We make sure that the PropertyData of the container is not connected to the one of the extension
#define PROPERTY_SOURCE_WITH_EXTENSIONS(_class_, _parentclass_) \
    PROPERTY_SOURCE(_class_, _parentclass_)

#define PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(_class_, _parentclass_) \
    PROPERTY_SOURCE_ABSTRACT(_class_, _parentclass_)

} //App

#endif // APP_EXTENSIONCONTAINER_H
