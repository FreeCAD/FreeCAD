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

#ifndef APP_PROPERTYLINKSUBLIST_H
#define APP_PROPERTYLINKSUBLIST_H

#include <FCGlobal.h>

#include "PropertyLinkBase.h"

namespace App {
using SubSet = std::pair<DocumentObject*, std::vector<std::string>>;

class AppExport PropertyLinkSubList: public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyLinkSubList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyLinkSubList() override;

    void afterRestore() override;
    void onContainerRestored() override;

    int getSize() const;
    void setSize(int newSize);

    /** Sets the property.
     * setValue(0, whatever) clears the property
     */
    void setValue(DocumentObject*, const char*);
    void setValues(const std::vector<DocumentObject*>&, const std::vector<const char*>&);
    void setValues(const std::vector<DocumentObject*>&,
                   const std::vector<std::string>&,
                   std::vector<ShadowSub>&& ShadowSubList = {});
    void setValues(std::vector<DocumentObject*>&&,
                   std::vector<std::string>&& subs,
                   std::vector<ShadowSub>&& ShadowSubList = {});

    /**
     * @brief setValue: PropertyLinkSub-compatible overload
     * @param SubList
     */
    void setValue(App::DocumentObject* lValue,
                  const std::vector<std::string>& SubList = std::vector<std::string>());

    void addValue(App::DocumentObject* obj,
                  const std::vector<std::string>& SubList = {},
                  bool reset = false);

    const std::vector<DocumentObject*>& getValues() const
    {
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

    const std::vector<std::string>& getSubValues() const
    {
        return _lSubList;
    }

    std::vector<std::string> getSubValues(bool newStyle) const;

    const std::vector<ShadowSub>& getShadowSubs() const
    {
        return _ShadowSubList;
    }

    /**
     * @brief Removes all occurrences of \a lValue in the property
     * together with its sub-elements and returns the number of entries removed.
     */
    int removeValue(App::DocumentObject* lValue);

    void setSubListValues(const std::vector<SubSet>&);
    std::vector<SubSet> getSubListValues(bool newStyle = false) const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    bool upgrade(Base::XMLReader& reader, const char* typeName);

    Property* Copy() const override;
    void Paste(const Property& from) override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkListItem";
    }

    /// Return a copy of the property if any changes caused by importing external object
    Property*
    CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const override;

    Property* CopyOnLabelChange(App::DocumentObject* obj,
                                const std::string& ref,
                                const char* newLabel) const override;

    Property* CopyOnLinkReplace(const App::DocumentObject* parent,
                                App::DocumentObject* oldObj,
                                App::DocumentObject* newObj) const override;

    unsigned int getMemSize() const override;

    void updateElementReference(DocumentObject* feature,
                                bool reverse = false,
                                bool notify = false) override;

    bool referenceChanged() const override;

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

    void setSyncSubObject(bool enable);

private:
    void verifyObject(App::DocumentObject*, App::DocumentObject*);

private:
    // FIXME: Do not make two independent lists because this will lead to some inconsistencies!
    std::vector<DocumentObject*> _lValueList;
    std::vector<std::string> _lSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubListChild: public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubListChild()
    {
        _pcScope = LinkScope::Child;
    }
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubListGlobal: public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubListGlobal()
    {
        _pcScope = LinkScope::Global;
    }
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubListHidden: public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubListHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

}

#endif
