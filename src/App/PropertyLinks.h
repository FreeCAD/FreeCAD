/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef APP_PROPERTYLINKS_H
#define APP_PROPERTYLINKS_H

// Std. configurations


#include <vector>
#include <string>
#include "Property.h"

namespace Base {
class Writer;
}

namespace App
{
class DocumentObject;

/**
 * @brief Defines different scopes for which a link can be valid
 * The scopes defined in this enum describe the different possibilities of where a link can point to.
 * Local:    links are valid only within the same GeoFeatureGroup as the linkowner is in or in none. 
 * Child:    links are valid within the same or any sub GeoFeatureGroup
 * Global:   all possible links are valid
 */
enum class LinkScope {
    Local,
    Child,
    Global
};

/**
 * @brief Enables scope handling for links
 * This class is a base for all link properties and enables them to handle scopes of the linked objects.
 * The possible scopes are defined by LinkScope enum class. The default value is Local. 
 * The scope of a property is not saved in the document. It is a value that needs to be fixed when
 * the object holding the property is loaded. That is possible with two methods: 
 * 1. Set the scope value in the constructor of the link property
 * 2. Use setScope to change the scope in the constructor of the link property
 * 
 * The second option is only available in c++, not in python, as setscope is not exposed. It would 
 * not make sense to expose it there, as restoring python objects does not call the constructor again.
 * Hence in python the only way to create a LinkProperty with different scope than local is to use a 
 * specialized property for that. In c++ existing properties can simply be changed via setScope in the 
 * objects constructor. 
 */
class AppExport ScopedLink {
  
public:
    /**
     * @brief Set the links scope
     * Allows to define what kind of links are allowed. Only in the Local GeoFeatureGroup, in this and 
     * all Childs or to all objects within the Glocal scope.
     */
    void setScope(LinkScope scope) {_pcScope = scope;};    
    /**
     * @brief Get the links scope
     * Retrieve what kind of links are allowed. Only in the Local GeoFeatureGroup, in this and 
     * all Childs or to all objects within the Glocal scope.
     */
    LinkScope getScope() {return _pcScope;};
    
protected:
    LinkScope _pcScope = LinkScope::Local;
};

/** The general Link Property
 *  Main Purpose of this property is to Link Objects and Features in a document. Like all links this 
 *  property is scope aware, meaning it does define which objects are allowed to be linked depending 
 *  of the GeoFeatureGroup where it is in. Default is Local.
 * 
 *  @note Links that are invalid in respect to the scope of this property is set to are not rejected. 
 *        They are only detected to be invalid and prevent the feature from recomputing.
 */
class AppExport PropertyLink : public Property, public ScopedLink
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLink();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyLink();

    /** Sets the property
     */
    void setValue(App::DocumentObject *);

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject * getValue(void) const;

    /** Returns the link type checked
     */
    App::DocumentObject * getValue(Base::Type t) const;

   /** Returns the link type checked
     */
    template <typename _type>
    inline _type getValue(void) const {
        return _pcLink ? dynamic_cast<_type>(_pcLink) : 0;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{
        return sizeof(App::DocumentObject *);
    }
    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyLinkItem"; }

protected:
    App::DocumentObject *_pcLink;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkChild : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkChild() {_pcScope = LinkScope::Child;};
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkGlobal : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkGlobal() {_pcScope = LinkScope::Global;};
};

class AppExport PropertyLinkList : public PropertyLists, public ScopedLink
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLinkList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyLinkList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property
     */
    void setValue(DocumentObject*);
    void setValues(const std::vector<DocumentObject*>&);

    /// index operator
    DocumentObject* operator[] (const int idx) const {
        return _lValueList.operator[] (idx);
    }


    void  set1Value(const int idx, DocumentObject* value) {
        _lValueList.operator[] (idx) = value;
    }

    const std::vector<DocumentObject*> &getValues(void) const {
        return _lValueList;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save(Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize(void) const;
    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyLinkListItem"; }

private:
    std::vector<DocumentObject*> _lValueList;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkListChild : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListChild() {_pcScope = LinkScope::Child;};
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkListGlobal : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListGlobal() {_pcScope = LinkScope::Global;};
};

/** the Link Property with sub elements
 *  This property links an object and a defined sequence of
 *  sub elements. These subelements (like Edges of a Shape)
 *  are stored as names, which can be resolved by the 
 *  ComplexGeoDataType interface to concrete sub objects.
 */
class AppExport PropertyLinkSub : public Property, public ScopedLink
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLinkSub();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyLinkSub();

    /** Sets the property
     */
    void setValue(App::DocumentObject *,const std::vector<std::string> &SubList=std::vector<std::string>());

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject * getValue(void) const;

    /// return the list of sub elements 
    const std::vector<std::string>& getSubValues(void) const;

    /// return the list of sub elements starts with a special string 
    std::vector<std::string> getSubValuesStartsWith(const char*) const;

    /** Returns the link type checked
     */
    App::DocumentObject * getValue(Base::Type t) const;

   /** Returns the link type checked
     */
    template <typename _type>
    inline _type getValue(void) const {
        return _pcLinkSub ? dynamic_cast<_type>(_pcLinkSub) : 0;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{
        return sizeof(App::DocumentObject *);
    }

protected:
    App::DocumentObject*     _pcLinkSub;
    std::vector<std::string> _cSubList;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubChild : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubChild() {_pcScope = LinkScope::Child;};
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubGlobal : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubGlobal() {_pcScope = LinkScope::Global;};
};

class AppExport PropertyLinkSubList : public PropertyLists, public ScopedLink
{
    TYPESYSTEM_HEADER();

public:
    typedef std::pair<DocumentObject*, std::vector<std::string> > SubSet;
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLinkSubList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyLinkSubList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property.
     * setValue(0, whatever) clears the property
     */
    void setValue(DocumentObject*,const char*);
    void setValues(const std::vector<DocumentObject*>&,const std::vector<const char*>&);
    void setValues(const std::vector<DocumentObject*>&,const std::vector<std::string>&);

    /**
     * @brief setValue: PropertyLinkSub-compatible overload
     * @param SubList
     */
    void setValue(App::DocumentObject *lValue, const std::vector<std::string> &SubList=std::vector<std::string>());

    const std::vector<DocumentObject*> &getValues(void) const {
        return _lValueList;
    }

    const std::string getPyReprString() const;

    /**
     * @brief getValue emulates the action of a single-object link.
     * @return reference to object, if the link is to only one object. NULL if
     * the link is empty, or links to subelements of more than one document
     * object.
     */
    DocumentObject* getValue() const;

    const std::vector<std::string> &getSubValues(void) const {
        return _lSubList;
    }

    /**
     * @brief Removes all occurrences of \a lValue in the property
     * together with its sub-elements and returns the number of entries removed.
     */
    int removeValue(App::DocumentObject *lValue);

    void setSubListValues(const std::vector<SubSet>&);
    std::vector<SubSet> getSubListValues() const;

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const;

private:
    //FIXME: Do not make two independent lists because this will lead to some inconsistencies!
    std::vector<DocumentObject*> _lValueList;
    std::vector<std::string>     _lSubList;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubListChild : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListChild() {_pcScope = LinkScope::Child;};
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubListGlobal : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListGlobal() {_pcScope = LinkScope::Global;};
};

} // namespace App


#endif // APP_PROPERTYLINKS_H
