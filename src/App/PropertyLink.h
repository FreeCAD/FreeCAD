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


#ifndef APP_PROPERTYLINK_H
#define APP_PROPERTYLINK_H

#include <set>
#include <string>
#include <vector>

#include "PropertyLinkBase.h"

namespace Base
{
class Writer;
}

namespace App
{
class DocumentObject;
class Document;
class GeoFeature;
class SubObjectT;
class PropertyXLink;

/** The general Link Property
 *  Main Purpose of this property is to Link Objects and Features in a document. Like all links this
 *  property is scope aware, meaning it does define which objects are allowed to be linked depending
 *  of the GeoFeatureGroup where it is in. Default is Local.
 *
 *  @note Links that are invalid in respect to the scope of this property is set to are not
 * rejected. They are only detected to be invalid and prevent the feature from recomputing.
 */
class AppExport PropertyLink: public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

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
    ~PropertyLink() override;

    void resetLink();

    /** Sets the property
     */
    virtual void setValue(App::DocumentObject*);

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject* getValue() const;

    /** Returns the link type checked
     */
    App::DocumentObject* getValue(Base::Type t) const;

    /** Returns the link type checked
     */
    template<typename _type>
    inline _type getValue() const
    {
        return _pcLink ? dynamic_cast<_type>(_pcLink) : 0;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(App::DocumentObject*);
    }
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkItem";
    }

    void getLinks(std::vector<App::DocumentObject*>& objs,
                  bool all = false,
                  std::vector<std::string>* subs = nullptr,
                  bool newStyle = true) const override;

    void getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                    App::DocumentObject* obj,
                    const char* subname = nullptr,
                    bool all = false) const override;

    void breakLink(App::DocumentObject* obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject*>& inList) override;

    Property* CopyOnLinkReplace(const App::DocumentObject* parent,
                                App::DocumentObject* oldObj,
                                App::DocumentObject* newObj) const override;

protected:
    App::DocumentObject* _pcLink {nullptr};
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkChild: public PropertyLink
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkChild()
    {
        _pcScope = LinkScope::Child;
    }
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkGlobal: public PropertyLink
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkGlobal()
    {
        _pcScope = LinkScope::Global;
    }
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkHidden: public PropertyLink
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};



}  // namespace App


#endif  // APP_PROPERTYLINKS_H
