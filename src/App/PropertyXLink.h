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

#ifndef APP_PROPERTYXLINK_H
#define APP_PROPERTYXLINK_H

#include "PropertyLinkBase.h"
#include "PropertyLink.h"

namespace App {
class DocInfo;
using DocInfoPtr = std::shared_ptr<DocInfo>;
class PropertyXLinkSubList;
class PropertyLinkSubList;
using SubSet = std::pair<DocumentObject*, std::vector<std::string>>;

/** Link to an (sub)object in the same or different document
 */
class AppExport PropertyXLink: public PropertyLinkGlobal
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit PropertyXLink(bool allowPartial = false, PropertyLinkBase* parent = nullptr);

    ~PropertyXLink() override;

    PropertyLinkBase* parent() const
    {
        return parentProp;
    }

    void afterRestore() override;
    void onContainerRestored() override;

    void setValue(App::DocumentObject*) override;
    void setValue(App::DocumentObject*, const char* subname);

    void setValue(std::string&& filePath,
                  std::string&& objectName,
                  std::vector<std::string>&& SubList,
                  std::vector<ShadowSub>&& ShadowSubList = {});

    void setValue(App::DocumentObject*,
                  std::vector<std::string>&& SubList,
                  std::vector<ShadowSub>&& ShadowSubList = {});

    void setValue(App::DocumentObject*,
                  const std::vector<std::string>& SubList,
                  std::vector<ShadowSub>&& ShadowSubList = {});

    void setSubValues(std::vector<std::string>&& SubList,
                      std::vector<ShadowSub>&& ShadowSubList = {});

    const char* getSubName(bool newStyle = true) const;
    void setSubName(const char* subname);

    bool hasSubName() const
    {
        return !_SubList.empty();
    }

    App::Document* getDocument() const;
    const char* getDocumentPath() const;
    const char* getObjectName() const;

    int checkRestore(std::string* msg = nullptr) const override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    /// Return a copy of the property if any changes caused by importing external object
    Property*
    CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const override;

    Property* CopyOnLabelChange(App::DocumentObject* obj,
                                const std::string& ref,
                                const char* newLabel) const override;

    Property* CopyOnLinkReplace(const App::DocumentObject* parent,
                                App::DocumentObject* oldObj,
                                App::DocumentObject* newObj) const override;

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    friend class DocInfo;

    static bool supportXLink(const App::Property* prop);
    static bool hasXLink(const App::Document* doc);
    static bool hasXLink(const std::vector<App::DocumentObject*>& objs,
                         std::vector<App::Document*>* unsaved = nullptr);
    static std::map<App::Document*, std::set<App::Document*>>
    getDocumentOutList(App::Document* doc = nullptr);
    static std::map<App::Document*, std::set<App::Document*>>
    getDocumentInList(App::Document* doc = nullptr);
    static void restoreDocument(const App::Document& doc);

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

    bool adjustLink(const std::set<App::DocumentObject*>& inList) override;

    const std::vector<std::string>& getSubValues() const
    {
        return _SubList;
    }
    const std::vector<ShadowSub>& getShadowSubs() const
    {
        return _ShadowSubList;
    }
    std::vector<std::string> getSubValues(bool newStyle) const;
    std::vector<std::string> getSubValuesStartsWith(const char*, bool newStyle = false) const;

    void setAllowPartial(bool enable) override;

    const char* getFilePath() const
    {
        return filePath.c_str();
    }

    virtual bool upgrade(Base::XMLReader& reader, const char* typeName);

    void setSyncSubObject(bool enable);

protected:
    void unlink();
    void detach();

    void restoreLink(App::DocumentObject*);

    void copyTo(PropertyXLink& other,
                App::DocumentObject* linked = nullptr,
                std::vector<std::string>* subs = nullptr) const;

    void aboutToSetValue() override;

    void hasSetValue() override;

    friend class PropertyXLinkSubList;

protected:
    DocInfoPtr docInfo;
    std::string filePath;
    std::string docName;
    std::string objectName;
    std::string stamp;
    std::vector<std::string> _SubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
    PropertyLinkBase* parentProp;
    mutable std::string tmpShadow;
};


/** Link to one or more (sub)object from the same or different document
 */
class AppExport PropertyXLinkSub: public PropertyXLink
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit PropertyXLinkSub(bool allowPartial = false, PropertyLinkBase* parent = nullptr);

    ~PropertyXLinkSub() override;

    bool upgrade(Base::XMLReader& reader, const char* typeName) override;

    PyObject* getPyObject() override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkItem";
    }
};

/** The PropertyXLinkSub that is hidden from dependency checking
 */
class AppExport PropertyXLinkSubHidden: public PropertyXLinkSub
{
    TYPESYSTEM_HEADER();

public:
    PropertyXLinkSubHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

/** Link to one or more (sub)object(s) of one or more object(s) from the same or different document
 */
class AppExport PropertyXLinkSubList: public PropertyLinkBase,
                                      public AtomicPropertyChangeInterface<PropertyXLinkSubList>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    using atomic_change =
        typename AtomicPropertyChangeInterface<PropertyXLinkSubList>::AtomicPropertyChange;
    friend atomic_change;

    PropertyXLinkSubList();
    ~PropertyXLinkSubList() override;

    void afterRestore() override;
    void onContainerRestored() override;

    int getSize() const;

    /** Sets the property.
     * setValue(0, whatever) clears the property
     */
    void setValue(DocumentObject*, const char*);
    void setValues(const std::vector<DocumentObject*>&);
    void set1Value(int idx, DocumentObject* value, const std::vector<std::string>& SubList = {});

    void setValues(const std::vector<DocumentObject*>&, const std::vector<const char*>&);
    void setValues(const std::vector<DocumentObject*>&, const std::vector<std::string>&);
    void setValues(std::map<App::DocumentObject*, std::vector<std::string>>&&);
    void setValues(const std::map<App::DocumentObject*, std::vector<std::string>>&);

    void addValue(App::DocumentObject* obj,
                  const std::vector<std::string>& SubList = {},
                  bool reset = false);
    void
    addValue(App::DocumentObject* obj, std::vector<std::string>&& SubList = {}, bool reset = false);

    /**
     * @brief setValue: PropertyLinkSub-compatible overload
     * @param SubList
     */
    void setValue(App::DocumentObject* lValue, const std::vector<std::string>& SubList = {});

    std::vector<DocumentObject*> getValues() const;

    const std::string getPyReprString() const;

    DocumentObject* getValue() const;

    const std::vector<std::string>& getSubValues(App::DocumentObject* obj) const;

    std::vector<std::string> getSubValues(App::DocumentObject* obj, bool newStyle) const;

    const std::vector<ShadowSub>& getShadowSubs(App::DocumentObject* obj) const;

    /**
     * @brief Removes all occurrences of \a lValue in the property
     * together with its sub-elements and returns the number of entries removed.
     */
    int removeValue(App::DocumentObject* lValue);

    void setSubListValues(const std::vector<SubSet>&);

    const std::list<PropertyXLinkSub>& getSubListValues() const
    {
        return _Links;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyLinkListItem";
    }

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

    bool upgrade(Base::XMLReader& reader, const char* typeName);

    int checkRestore(std::string* msg = nullptr) const override;

    void setAllowPartial(bool enable) override;

    void hasSetChildValue(Property&) override;
    void aboutToSetChildValue(Property&) override;

    void setSyncSubObject(bool enable);

protected:
    void _getLinksToList(std::vector<App::ObjectIdentifier>& identifiers,
                         App::DocumentObject* obj,
                         const char* subname,
                         const std::vector<std::string>& subs,
                         const std::vector<PropertyLinkBase::ShadowSub>& shadows) const;

protected:
    std::list<PropertyXLinkSub> _Links;
};


/** Link to one or more (sub)object(s) of one or more object(s) from the same or different document
 *
 * The only difference for PropertyXLinkList and PropertyXLinkSubList is in
 * their getPyObject().  PropertyXLinkList will return a list of object is
 * there is no sub-object/sub-elements in the property.
 */
class AppExport PropertyXLinkList: public PropertyXLinkSubList
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyXLinkList();
    ~PropertyXLinkList() override;

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;
};


/** Abstract property that can link to multiple external objects
 *
 * @sa See PropertyExpressionEngine for example usage
 */
class AppExport PropertyXLinkContainer: public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyXLinkContainer();
    ~PropertyXLinkContainer() override;

    void afterRestore() override;
    int checkRestore(std::string* msg = nullptr) const override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void breakLink(App::DocumentObject* obj, bool clear) override;
    void getLinks(std::vector<App::DocumentObject*>& objs,
                  bool all = false,
                  std::vector<std::string>* subs = nullptr,
                  bool newStyle = true) const override;

    bool isLinkedToDocument(const App::Document& doc) const;

protected:
    void aboutToSetChildValue(App::Property& prop) override;
    virtual PropertyXLink* createXLink();
    virtual void onBreakLink(App::DocumentObject* obj);
    virtual void onAddDep(App::DocumentObject*)
    {}
    virtual void onRemoveDep(App::DocumentObject*)
    {}
    void updateDeps(std::map<DocumentObject*, bool>&& newDeps);
    void clearDeps();

    void _onBreakLink(App::DocumentObject* obj);

protected:
    std::map<App::DocumentObject*, bool> _Deps;
    std::map<std::string, std::unique_ptr<PropertyXLink>> _XLinks;
    std::map<std::string, std::string> _DocMap;
    bool _LinkRestored;

private:
    struct RestoreInfo
    {
        std::unique_ptr<PropertyXLink> xlink;
        std::string docName;
        std::string docLabel;
        bool hidden = false;
    };
    std::unique_ptr<std::vector<RestoreInfo>> _XLinkRestores;
};

}

#endif