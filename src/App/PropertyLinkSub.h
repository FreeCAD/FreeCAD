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

#ifndef APP_PROPERTYLINKSUB_H
#define APP_PROPERTYLINKSUB_H

#include <FCGlobal.h>

#include <utility>
#include <vector>
#include <string>

#include "PropertyLinkBase.h"


#define ATTR_SHADOWED "shadowed"
#define ATTR_SHADOW "shadow"
#define ATTR_MAPPED "mapped"
#define IGNORE_SHADOW false

namespace App {
class DocumentObject;
using SubSet = std::pair<DocumentObject*, std::vector<std::string>>;

/** the Link Property with sub elements
 *  This property links an object and a defined sequence of
 *  sub elements. These subelements (like Edges of a Shape)
 *  are stored as names, which can be resolved by the
 *  ComplexGeoDataType interface to concrete sub objects.
 */
class AppExport PropertyLinkSub: public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

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
    ~PropertyLinkSub() override;

    void afterRestore() override;
    void onContainerRestored() override;

    /** Sets the property
     */
    void setValue(App::DocumentObject*,
                  const std::vector<std::string>& SubList,
                  std::vector<ShadowSub>&& ShadowSubList = {});
    void setValue(App::DocumentObject*,
                  std::vector<std::string>&& SubList = {},
                  std::vector<ShadowSub>&& ShadowSubList = {});

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject* getValue() const;

    /// return the list of sub elements
    const std::vector<std::string>& getSubValues() const;

    /// return the list of sub elements with mapped names
    const std::vector<ShadowSub>& getShadowSubs() const
    {
        return _ShadowSubList;
    }

    std::vector<std::string> getSubValues(bool newStyle) const;

    /// return the list of sub elements starts with a special string
    std::vector<std::string> getSubValuesStartsWith(const char*, bool newStyle = false) const;

    /** Returns the link type checked
     */
    App::DocumentObject* getValue(Base::Type t) const;

    /** Returns the link type checked
     */
    template<typename _type>
    inline _type getValue() const
    {
        return _pcLinkSub ? dynamic_cast<_type>(_pcLinkSub) : 0;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkItem";
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

    unsigned int getMemSize() const override
    {
        return sizeof(App::DocumentObject*);
    }

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

protected:
    App::DocumentObject* _pcLinkSub {nullptr};
    std::vector<std::string> _cSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
    bool _restoreLabel {false};
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubChild: public PropertyLinkSub
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubChild()
    {
        _pcScope = LinkScope::Child;
    }
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubGlobal: public PropertyLinkSub
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubGlobal()
    {
        _pcScope = LinkScope::Global;
    }
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubHidden: public PropertyLinkSub
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

bool updateLinkReference(App::PropertyLinkBase* prop,
                         App::DocumentObject* feature,
                         bool reverse,
                         bool notify,
                         App::DocumentObject* link,
                         std::vector<std::string>& subs,
                         std::vector<int>& mapped,
                         std::vector<PropertyLinkBase::ShadowSub>& shadows);

// Extern instantiation in .cpp
template<class Func, class... Args>
std::vector<std::string> updateLinkSubs(const App::DocumentObject* obj,
                                        const std::vector<std::string>& subs,
                                        Func* f,
                                        Args&&... args);

const std::string& getSubNameWithStyle(const std::string& subName,
                                       const PropertyLinkBase::ShadowSub& shadow,
                                       bool newStyle,
                                       std::string& tmp);

App::DocumentObject*
adjustLinkSubs(App::PropertyLinkBase* prop,
               const std::set<App::DocumentObject*>& inList,
               App::DocumentObject* link,
               std::vector<std::string>& subs,
               std::map<App::DocumentObject*, std::vector<std::string>>* links = nullptr);

}  // namespace App

#endif
