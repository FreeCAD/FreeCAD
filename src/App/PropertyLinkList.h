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

#include "FCGlobal.h"

#include "PropertyLinkBase.h"

#ifndef APP_PROPERTYLINKLIST_H
#define APP_PROPERTYLINKLIST_H

namespace App {

class AppExport PropertyLinkListBase: public PropertyLinkBase, public PropertyListsBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    void setPyObject(PyObject* obj) override
    {
        _setPyObject(obj);
    }
};

class AppExport PropertyLinkList
    : public PropertyListsT<DocumentObject*, std::vector<DocumentObject*>, PropertyLinkListBase>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited =
        PropertyListsT<DocumentObject*, std::vector<DocumentObject*>, PropertyLinkListBase>;

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
    ~PropertyLinkList() override;

    void setSize(int newSize) override;
    void setSize(int newSize, const_reference def) override;

    /** Sets the property
     */
    void setValues(const std::vector<DocumentObject*>& value) override;

    void set1Value(int idx, DocumentObject* const& value) override;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkListItem";
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

    DocumentObject* findUsingMap(const std::string&, int* pindex = nullptr) const;
    DocumentObject* find(const char* sub, int* pindex = nullptr) const;

protected:
    DocumentObject* getPyValue(PyObject* item) const override;

protected:
    mutable std::map<std::string, int> _nameMap;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkListChild: public PropertyLinkList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkListChild()
    {
        _pcScope = LinkScope::Child;
    }
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkListGlobal: public PropertyLinkList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkListGlobal()
    {
        _pcScope = LinkScope::Global;
    }
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkListHidden: public PropertyLinkList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkListHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

}

#endif
