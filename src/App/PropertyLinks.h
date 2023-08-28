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


#ifndef APP_PROPERTYLINKS_H
#define APP_PROPERTYLINKS_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include "Property.h"

namespace Base {
class Writer;
}

namespace App
{
class DocumentObject;
class Document;

class DocInfo;
using DocInfoPtr = std::shared_ptr<DocInfo>;

class PropertyXLink;

/**
 * @brief Defines different scopes for which a link can be valid
 * The scopes defined in this enum describe the different possibilities of where a link can point to.
 * Local:    links are valid only within the same GeoFeatureGroup as the linkowner is in or in none.
 * Child:    links are valid within the same or any sub GeoFeatureGroup
 * Global:   all possible links are valid
 * Hidden:   links are not included in dependency calculation
 */
enum class LinkScope {
    Local,
    Child,
    Global,
    Hidden,
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
    LinkScope getScope() const {return _pcScope;};
    
protected:
    LinkScope _pcScope = LinkScope::Local;
};

/// Parent class of all link type properties
class AppExport PropertyLinkBase : public Property, public ScopedLink
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    using ShadowSub = std::pair<std::string,std::string>;

    PropertyLinkBase();
    ~PropertyLinkBase() override;

    friend class DocInfo;

    /** Link type property interface APIs
     * These APIs are moved here so that any type of property can have the
     * property link behavior, e.g. the PropertyExpressionEngine
     */
    //@{

    /** Called to update the element reference of this link property
     *
     * @sa _updateElementReference()
     */
    virtual void updateElementReference(App::DocumentObject *feature,
            bool reverse=false, bool notify=false)
    {
        (void)feature;
        (void)reverse;
        (void)notify;
    }

    /// Clear internal element reference registration
    void unregisterElementReference();

    /** Register label reference for future object relabel update
     *
     *  @param labels: labels to be registered
     *  @param reset: if true, then calls unregisterLabelReference() before
     *  registering
     */
    void registerLabelReferences(std::vector<std::string> &&labels, bool reset=true);

    /** Check subnames for label registration
     *
     *  @param subs: subname references
     *  @param reset: if true, then calls unregisterLabelReference() before
     *  registering
     *
     *  Check the give subname references and extract any label reference
     *  inside (by calling getLabelReferences()), and register them.
     */
    void checkLabelReferences(const std::vector<std::string> &subs, bool reset=true);

    /// Clear internal label references registration
    void unregisterLabelReferences();

    /// Test if the element reference has changed after restore
    virtual bool referenceChanged() const {
        return false;
    }

    /** Test if the link is restored unchanged
     *
     * @param msg: optional error message
     *
     * @return For external linked object, return 2 in case the link is
     * missing, and 1 if the time stamp has changed.
     */
    virtual int checkRestore(std::string *msg=nullptr) const {
        (void)msg;
        return 0;
    }

    /** Obtain the linked objects
     *
     * @param objs: hold the returned linked objects on output
     * @param all: if true, then return all the linked object regardless of
     *             this LinkScope. If false, then return only if the LinkScope
     *             is not hidden.
     * @param sub: if given, then return subname references.
     * @param newStyle: whether to return new or old style subname reference
     */
    virtual void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const = 0;

    /** Called to reset this link property
     *
     * @param obj: reset link property if it is linked to this object
     * @param clear: if true, then also reset property if the owner of this property is \a obj
     *
     * @sa breakLinks()
     */
    virtual void breakLink(App::DocumentObject *obj, bool clear) = 0;

    /** Called to adjust the link to avoid potential cyclic dependency
     *
     * @param inList: recursive in-list of the would-be parent
     *
     * @return Return whether the link has been adjusted
     *
     * This function tries to correct the link to avoid any (sub)object inside
     * in-list. If the adjustment is impossible, exception will be raised
     */
    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) = 0;

    /** Return a copy of the property if the link replacement affects this property
     *
     * @param owner: the parent object whose link property is to be replace.
     *               Note that The parent may not be the container of this
     *               property. Link sub property can use this opportunity to
     *               adjust its relative links.
     * @param oldObj: object to be replaced
     * @param newObj: object to replace with
     *
     * @return Return a copy of the property that is adjusted for the link
     * replacement operation.
     */
    virtual Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const = 0;

    /** Return a copy of the property if any changes caused by importing external linked object
     *
     * @param nameMap: a map from the original external object name to the
     * imported new object name
     *
     * @return Returns a copy of the property with the updated link reference if
     * affected. The copy will later be assgiend to this property by calling its
     * Paste().
     */
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const {
        (void)nameMap;
        return nullptr;
    }

    /** Update object label reference in this property
     *
     * @param obj: the object owner of the changing label
     * @param ref: subname reference to old label
     * @param newLabel: the future new label
     *
     * @return Returns a copy of the property if its link reference is affected.
     * The copy will later be assgiend to this property by calling its Paste().
     */
    virtual Property *CopyOnLabelChange(App::DocumentObject *obj,
                        const std::string &ref, const char *newLabel) const
    {
        (void)obj;
        (void)ref;
        (void)newLabel;
        return nullptr;
    }

    /// Helper function to return all linked objects of this property
    std::vector<App::DocumentObject *> linkedObjects(bool all=false) const {
        std::vector<App::DocumentObject*> ret;
        getLinks(ret,all);
        return ret;
    }

    /// Helper function to return linked objects using an std::inserter
    template<class T>
    void getLinkedObjects(T &inserter, bool all=false) const {
        std::vector<App::DocumentObject*> ret;
        getLinks(ret,all);
        std::copy(ret.begin(),ret.end(),inserter);
    }

    /// Helper function to return a map of linked object and its subname references
    void getLinkedElements(std::map<App::DocumentObject*, std::vector<std::string> > &elements,
            bool newStyle=true, bool all=true) const
    {
        std::vector<App::DocumentObject*> ret;
        std::vector<std::string> subs;
        getLinks(ret,all,&subs,newStyle);
        assert(ret.size()==subs.size());
        int i=0;
        for(auto obj : ret)
            elements[obj].push_back(subs[i++]);
    }

    /// Helper function to return a map of linked object and its subname references
    std::map<App::DocumentObject*, std::vector<std::string> >
        linkedElements(bool newStyle=true, bool all=true) const
    {
        std::map<App::DocumentObject*, std::vector<std::string> > ret;
        getLinkedElements(ret,newStyle,all);
        return ret;
    }
    //@}

    bool isSame(const Property &other) const override;

    /** Enable/disable temporary holding external object without throwing exception
     *
     * Warning, non-PropertyXLink related property does not have internal
     * tracking of external objects, therefore the link will not by auto broken
     * when external document is closed. Only use this for temporary case, or
     * if you handle signalDeleteDocument yourself, or use one of the
     * PropertyXLink related property.
     */
    void setAllowExternal(bool allow);

    /// Helper functions
    //@{

    /** Helper function to check and replace a link
     *
     * @param owner: the owner of the current property
     * @param obj: the current linked object
     * @param parent: the parent of the changing link property, may or may not
     * be equal to \c owner
     * @param oldObj: the object to be replaced
     * @param newObj: the object to replace with
     * @param sub: optional the current subname reference
     *
     * @return Returns a pair(obj,subname). If no replacement is found,
     * pair.first will be NULL
     *
     * Say a group has one of its child object replaced with another. Any
     * existing link sub reference that refer to the original child object
     * through the group will be broken. This helper function is used to check
     * and correct any link sub reference.
     */
    static std::pair<App::DocumentObject*,std::string> tryReplaceLink(
            const App::PropertyContainer *owner, App::DocumentObject *obj,
            const App::DocumentObject *parent, App::DocumentObject *oldObj,
            App::DocumentObject *newObj, const char *sub=nullptr);

    /** Helper function to check and replace a link with multiple subname references
     *
     * @param owner: the owner of the current property
     * @param obj: the current linked object
     * @param parent: the parent of the changing link property, may or may not
     * be equal to \c owner
     * @param oldObj: the object to be replaced
     * @param newObj: the object to replace with
     * @param subs: the current subname references
     *
     * @return Returns the a pair(obj,subs). If no replacement is found,
     * pair.first will be NULL
     * @sa tryReplaceLink()
     */
    static std::pair<App::DocumentObject*, std::vector<std::string> >
        tryReplaceLinkSubs( const App::PropertyContainer *owner,
                            App::DocumentObject *obj,
                            const App::DocumentObject *parent,
                            App::DocumentObject *oldObj,
                            App::DocumentObject *newObj,
                            const std::vector<std::string> &subs);

    /// Update all element references in all link properties of \a feature
    static void updateElementReferences(DocumentObject *feature, bool reverse=false);


    /** Helper function for update individual element reference
     *
     * @param feature: if given, than only update element reference belonging
     *                 to this feature. If not, then update geometry element
     *                 references.
     * @param sub: the subname reference to be updated.
     * @param shadow: a pair of new and old style element references to be updated.
     * @param reverse: if true, then use the old style, i.e. non-mapped element
     *                 reference to query for the new style, i.e. mapped
     *                 element reference when update. If false, then the other
     *                 way around.
     * @param notify: if true, call aboutToSetValue() before change
     *
     * This helper function is to be called by each link property in the event of
     * geometry element reference change due to geometry model changes.
     */
     bool _updateElementReference(App::DocumentObject *feature,
        App::DocumentObject *obj, std::string &sub, ShadowSub &shadow,
        bool reverse, bool notify=false);

    /** Helper function to register geometry element reference
     *
     * @param obj: the linked object
     * @param sub: the subname reference
     * @param shadow: a pair of new and old style element references to be updated.
     *
     * Search for any geometry element reference inside the subname, and
     * register for future update in case of geometry model update.
     */
    void _registerElementReference(App::DocumentObject *obj, std::string &sub, ShadowSub &shadow);

    /** Helper function for breaking link properties
     *
     * @param link: reset link property if it is linked to this object
     * @param objs: the objects to check for the link properties
     * @param clear: if true, then also reset property if the owner of the link property is \a link
     *
     * App::Document::breakDependency() calls this function to break the link property
     */
    static void breakLinks(App::DocumentObject *link, const std::vector<App::DocumentObject*> &objs, bool clear);

    /** Helper function for link import operation
     *
     * @param obj: the linked object
     * @param sub: subname reference
     * @param doc: importing document
     * @param nameMap: a name map from source object to its imported counter part
     *
     * @return Return a changed subname reference, or empty string if no change.
     *
     * Link import operation will go through all link property and imports all
     * externally linked object. After import, the link property must be
     * changed to point to the newly imported objects, which should happen inside
     * the API CopyOnImportExternal(). This function helps to rewrite subname
     * reference to point to the correct sub objects that are imported.
     */
    static std::string tryImportSubName(const App::DocumentObject *obj, const char *sub,
            const App::Document *doc, const std::map<std::string,std::string> &nameMap);

    /** Helper function for link import operation
     *
     * @param doc: owner document of the imported objects
     * @param obj: the linked object
     * @param nameMap: a name map from source object to its imported counter part
     *
     * @return Return the imported object if found, or the input \c obj if no change.
     * @sa tryImportSubNames
     *
     * This function searches for the name map and tries to find the imported
     * object from the given source object.
     */
    static App::DocumentObject *tryImport(const App::Document *doc, const App::DocumentObject *obj,
                                          const std::map<std::string,std::string> &nameMap);

    /** Helper function to export a subname reference
     *
     * @param output: output subname if the subname is modified
     * @param obj: linked object
     * @param sub: input subname reference
     * @param first_obj: if true, then the first object referenced in subname
     *                   is obtained by searching the owner document of obj,
     *                   otherwise the subname search among obj's sub-objects.
     *
     * @return Return output.c_str() if the subname is modified for exporting
     * otherwise, return the input subname
     *
     * @sa importSubName(), restoreLabelReference()
     *
     * The function go through the input subname reference and changes any sub
     * object references inside for exporting. If the sub object is referenced
     * by its internal object name, then the reference is changed from
     * 'objName' to 'objName@docName'. If referenced by label, then it will be
     * changed to 'objName@docName@' instead. importSubName() and
     * restoreLabelReference() can be used together to restore the reference
     * during import.
     */
    static const char *exportSubName(std::string &output,
            const App::DocumentObject *obj, const char *subname, bool first_obj=false);

    /** Helper function to import a subname reference
     *
     * @param reader: the import reader
     * @param sub: input subname reference
     * @param restoreLabel: output indicate whether post process is required
     *                      after restore.
     *
     * @sa exportSubName(), restoreLabelReference()
     *
     * @return return either an updated subname reference or the input
     * reference if no change. If restoreLabel is set to true on output, it
     * means there are some label reference changes that must be corrected
     * after restore, by calling restoreLabelReference() in property's
     * afterRestore().
     */
    static std::string importSubName(Base::XMLReader &reader, const char *sub, bool &restoreLabel);

    /** Helper function to restore label references during import
     *
     * @param obj: linked object
     * @param sub: subname reference
     * @param shadow: optional shadow subname reference
     *
     * @sa exportSubName(), importSubName()
     *
     * When exporting and importing (i.e. copy and paste) objects into the same
     * document, the new object must be renamed, both the internal name and the
     * label. Therefore, the link reference of the new objects must be
     * corrected accordingly. The basic idea is that when exporting object, all
     * object name references are changed to 'objName@docName', and label
     * references are changed to 'objName@docName@'. During import,
     * MergeDocument will maintain a map from objName@docName to object's new
     * name. Object name reference can be restored on spot by consulting the
     * map, while label reference will be restored later in property's
     * afterRestore() function, which calls this function to do the string
     * parsing.
     */
    static void restoreLabelReference(const App::DocumentObject *obj, std::string &sub, ShadowSub *shadow=nullptr);

    /** Helper function to extract labels from a subname reference
     *
     * @param labels: output vector of extracted labels
     * @param subname: subname reference
     *
     * @sa registerLabelReferences()
     *
     * This function is used to extract label from subname reference for
     * registering of label changes.
     */
    static void getLabelReferences(std::vector<std::string> &labels, const char *subname);

    /** Helper function to collect changed property when an object re-label
     *
     * @param obj: the object that owns the label
     * @param newLabel: the new label
     *
     * @return return a map from the affected property to a copy of it with
     * updated subname references
     */
    static std::vector<std::pair<Property*, std::unique_ptr<Property> > > updateLabelReferences(
            App::DocumentObject *obj, const char *newLabel);

    /** Helper function to update subname reference on label change
     *
     * @param linked: linked object
     * @param subname: subname reference
     * @param obj: the object that owns the label
     * @param ref: label reference in the format of '$<old_label>.', which is
     *             the format used in subname reference for label reference.
     *             This parameter is provided for easy search of label
     *             reference.
     * @param newLabel: new label
     *
     * @return Returns an updated subname reference, or empty string if no change.
     *
     * This function helps to update subname reference on label change. It is
     * usually called inside CopyOnLabelChange(), the API for handling label
     * change, which is called just before label change. In other word, when
     * called, the sub object can still be reached using the original label
     * references, but not the new labels.
     */
    static std::string updateLabelReference(const App::DocumentObject *linked, const char *subname,
            App::DocumentObject *obj, const std::string &ref, const char *newLabel);
    //@}

    enum LinkFlags {
        LinkAllowExternal,
        LinkDetached,
        LinkRestoring,
        LinkAllowPartial,
        LinkRestoreLabel,
        LinkSyncSubObject, // used by DlgPropertyLink
    };
    inline bool testFlag(int flag) const {
        return _Flags.test((std::size_t)flag);
    }

    virtual void setAllowPartial(bool enable) { (void)enable; }

protected:
    void hasSetValue() override;

protected:
    std::bitset<32> _Flags;
    inline void setFlag(int flag, bool value=true) {
        _Flags.set((std::size_t)flag,value);
    }

private:
    std::set<std::string> _LabelRefs;
    std::set<App::DocumentObject*> _ElementRefs;
};

/** The general Link Property
 *  Main Purpose of this property is to Link Objects and Features in a document. Like all links this
 *  property is scope aware, meaning it does define which objects are allowed to be linked depending
 *  of the GeoFeatureGroup where it is in. Default is Local.
 *
 *  @note Links that are invalid in respect to the scope of this property is set to are not rejected.
 *        They are only detected to be invalid and prevent the feature from recomputing.
 */
class AppExport PropertyLink : public PropertyLinkBase
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
    virtual void setValue(App::DocumentObject *);

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject * getValue() const;

    /** Returns the link type checked
     */
    App::DocumentObject * getValue(Base::Type t) const;

   /** Returns the link type checked
     */
    template <typename _type>
    inline _type getValue() const {
        return _pcLink ? dynamic_cast<_type>(_pcLink) : 0;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize () const override {
        return sizeof(App::DocumentObject *);
    }
    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkItem"; }

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    void breakLink(App::DocumentObject *obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;
protected:
    App::DocumentObject *_pcLink{nullptr};
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkChild : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkChild() {_pcScope = LinkScope::Child;}
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkGlobal : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkGlobal() {_pcScope = LinkScope::Global;}
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkHidden : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkHidden() {_pcScope = LinkScope::Hidden;}
};


class AppExport PropertyLinkListBase: public PropertyLinkBase, public PropertyListsBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    void setPyObject(PyObject *obj) override {
        _setPyObject(obj);
    }
};

class AppExport PropertyLinkList :
    public PropertyListsT<DocumentObject*,std::vector<DocumentObject*>, PropertyLinkListBase>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited = PropertyListsT<DocumentObject*,std::vector<DocumentObject*>,PropertyLinkListBase>;

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
    void setValues(const std::vector<DocumentObject*>&) override;

    void set1Value(int idx, DocumentObject * const &value) override;

    PyObject *getPyObject() override;

    void Save(Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize() const override;
    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkListItem"; }

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    void breakLink(App::DocumentObject *obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    DocumentObject *find(const std::string &, int *pindex=nullptr) const;
    DocumentObject *find(const char *sub, int *pindex=nullptr) const {
        if(!sub) return nullptr;
        return find(std::string(sub),pindex);
    }

protected:
    DocumentObject *getPyValue(PyObject *item) const override;

protected:
    mutable std::map<std::string, int> _nameMap;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkListChild : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListChild() {_pcScope = LinkScope::Child;}
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkListGlobal : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListGlobal() {_pcScope = LinkScope::Global;}
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkListHidden : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListHidden() {_pcScope = LinkScope::Hidden;}
};

/** the Link Property with sub elements
 *  This property links an object and a defined sequence of
 *  sub elements. These subelements (like Edges of a Shape)
 *  are stored as names, which can be resolved by the
 *  ComplexGeoDataType interface to concrete sub objects.
 */
class AppExport PropertyLinkSub : public PropertyLinkBase
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
    void setValue(App::DocumentObject *,const std::vector<std::string> &SubList,
                    std::vector<ShadowSub> &&ShadowSubList={});
    void setValue(App::DocumentObject *,std::vector<std::string> &&SubList={},
                    std::vector<ShadowSub> &&ShadowSubList={});

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject * getValue() const;

    /// return the list of sub elements
    const std::vector<std::string>& getSubValues() const;

    /// return the list of sub elements with mapped names
    const std::vector<ShadowSub> &getShadowSubs() const {
        return _ShadowSubList;
    }

    std::vector<std::string> getSubValues(bool newStyle) const;

    /// return the list of sub elements starts with a special string
    std::vector<std::string> getSubValuesStartsWith(const char*, bool newStyle=false) const;

    /** Returns the link type checked
     */
    App::DocumentObject * getValue(Base::Type t) const;

   /** Returns the link type checked
     */
    template <typename _type>
    inline _type getValue() const {
        return _pcLinkSub ? dynamic_cast<_type>(_pcLinkSub) : 0;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkItem"; }

    /// Return a copy of the property if any changes caused by importing external object
    Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    Property *CopyOnLabelChange(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    unsigned int getMemSize () const override {
        return sizeof(App::DocumentObject *);
    }

    void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    bool referenceChanged() const override;

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    void breakLink(App::DocumentObject *obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    void setSyncSubObject(bool enable);
protected:
    App::DocumentObject*     _pcLinkSub{nullptr};
    std::vector<std::string> _cSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
    bool _restoreLabel{false};
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubChild : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubChild() {_pcScope = LinkScope::Child;}
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubGlobal : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubGlobal() {_pcScope = LinkScope::Global;}
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubHidden : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubHidden() {_pcScope = LinkScope::Hidden;}
};

class AppExport PropertyLinkSubList : public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    using SubSet = std::pair<DocumentObject*, std::vector<std::string> >;
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
    void setValue(DocumentObject*,const char*);
    void setValues(const std::vector<DocumentObject*>&,const std::vector<const char*>&);
    void setValues(const std::vector<DocumentObject*>&,const std::vector<std::string>&,
                    std::vector<ShadowSub> &&ShadowSubList={});
    void setValues(std::vector<DocumentObject*>&&, std::vector<std::string> &&subs,
                    std::vector<ShadowSub> &&ShadowSubList={});

    /**
     * @brief setValue: PropertyLinkSub-compatible overload
     * @param SubList
     */
    void setValue(App::DocumentObject *lValue, const std::vector<std::string> &SubList=std::vector<std::string>());

    void addValue(App::DocumentObject *obj, const std::vector<std::string> &SubList={}, bool reset = false);

    const std::vector<DocumentObject*> &getValues() const {
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

    const std::vector<std::string> &getSubValues() const {
        return _lSubList;
    }

    std::vector<std::string> getSubValues(bool newStyle) const;

    const std::vector<ShadowSub> &getShadowSubs() const {
        return _ShadowSubList;
    }

    /**
     * @brief Removes all occurrences of \a lValue in the property
     * together with its sub-elements and returns the number of entries removed.
     */
    int removeValue(App::DocumentObject *lValue);

    void setSubListValues(const std::vector<SubSet>&);
    std::vector<SubSet> getSubListValues(bool newStyle=false) const;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;
    bool upgrade(Base::XMLReader &reader, const char *typeName);

    Property *Copy() const override;
    void Paste(const Property &from) override;

    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkListItem"; }

    /// Return a copy of the property if any changes caused by importing external object
    Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    Property *CopyOnLabelChange(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    unsigned int getMemSize () const override;

    void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    bool referenceChanged() const override;

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    void breakLink(App::DocumentObject *obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    void setSyncSubObject(bool enable);

private:
    void verifyObject(App::DocumentObject *, App::DocumentObject *);

private:
    //FIXME: Do not make two independent lists because this will lead to some inconsistencies!
    std::vector<DocumentObject*> _lValueList;
    std::vector<std::string>     _lSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
};

/** The general Link Property with Child scope
 */
class AppExport PropertyLinkSubListChild : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListChild() {_pcScope = LinkScope::Child;}
};

/** The general Link Property with Global scope
 */
class AppExport PropertyLinkSubListGlobal : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListGlobal() {_pcScope = LinkScope::Global;}
};

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubListHidden : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListHidden() {_pcScope = LinkScope::Hidden;}
};

class PropertyXLinkSubList;

/** Link to an (sub)object in the same or different document
 */
class AppExport PropertyXLink : public PropertyLinkGlobal
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit PropertyXLink(bool allowPartial=false, PropertyLinkBase *parent=nullptr);

    ~PropertyXLink() override;

    PropertyLinkBase *parent() const { return parentProp; }

    void afterRestore() override;
    void onContainerRestored() override;

    void setValue(App::DocumentObject *) override;
    void setValue(App::DocumentObject *, const char *subname);

    void setValue(std::string &&filePath, std::string &&objectName, std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    void setValue(App::DocumentObject *,std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    void setValue(App::DocumentObject *,const std::vector<std::string> &SubList,
            std::vector<ShadowSub > &&ShadowSubList={});

    void setSubValues(std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    const char *getSubName(bool newStyle=true) const;
    void setSubName(const char *subname);

    bool hasSubName() const {return !_SubList.empty();}

    App::Document *getDocument() const;
    const char *getDocumentPath() const;
    const char *getObjectName() const;

    int checkRestore(std::string *msg=nullptr) const override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    /// Return a copy of the property if any changes caused by importing external object
    Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    Property *CopyOnLabelChange(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    friend class DocInfo;

    static bool supportXLink(const App::Property *prop);
    static bool hasXLink(const App::Document *doc);
    static bool hasXLink(const std::vector<App::DocumentObject*> &objs, std::vector<App::Document*> *unsaved=nullptr);
    static std::map<App::Document*,std::set<App::Document*> > getDocumentOutList(App::Document *doc=nullptr);
    static std::map<App::Document*,std::set<App::Document*> > getDocumentInList(App::Document *doc=nullptr);
    static void restoreDocument(const App::Document &doc);

    void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    bool referenceChanged() const override;

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    const std::vector<std::string>& getSubValues() const {
        return _SubList;
    }
    const std::vector<ShadowSub > &getShadowSubs() const {
        return _ShadowSubList;
    }
    std::vector<std::string> getSubValues(bool newStyle) const;
    std::vector<std::string> getSubValuesStartsWith(const char*, bool newStyle=false) const;

    void setAllowPartial(bool enable) override;

    const char *getFilePath() const {
        return filePath.c_str();
    }

    virtual bool upgrade(Base::XMLReader &reader, const char *typeName);

    void setSyncSubObject(bool enable);

protected:
    void unlink();
    void detach();

    void restoreLink(App::DocumentObject *);

    void copyTo(PropertyXLink &other, App::DocumentObject *linked=nullptr, std::vector<std::string> *subs=nullptr) const;

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
    PropertyLinkBase *parentProp;
};


/** Link to one or more (sub)object from the same or different document
 */
class AppExport PropertyXLinkSub: public PropertyXLink {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    explicit PropertyXLinkSub(bool allowPartial=false, PropertyLinkBase *parent=nullptr);

    ~PropertyXLinkSub() override;

    bool upgrade(Base::XMLReader &reader, const char *typeName) override;

    PyObject *getPyObject() override;

    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkItem"; }
};


/** Link to one or more (sub)object(s) of one or more object(s) from the same or different document
 */
class AppExport PropertyXLinkSubList: public PropertyLinkBase
                                    , public AtomicPropertyChangeInterface<PropertyXLinkSubList>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

    using atomic_change = typename AtomicPropertyChangeInterface<PropertyXLinkSubList>::AtomicPropertyChange;
    friend atomic_change;

public:
    PropertyXLinkSubList();
    ~PropertyXLinkSubList() override;

    void afterRestore() override;
    void onContainerRestored() override;

    int getSize() const;

    /** Sets the property.
     * setValue(0, whatever) clears the property
     */
    void setValue(DocumentObject*,const char*);
    void setValues(const std::vector<DocumentObject*>&);
    void set1Value(int idx, DocumentObject *value, const std::vector<std::string> &SubList={});

    void setValues(const std::vector<DocumentObject*>&,const std::vector<const char*>&);
    void setValues(const std::vector<DocumentObject*>&,const std::vector<std::string>&);
    void setValues(std::map<App::DocumentObject*,std::vector<std::string> > &&);
    void setValues(const std::map<App::DocumentObject*,std::vector<std::string> > &);

    void addValue(App::DocumentObject *obj, const std::vector<std::string> &SubList={}, bool reset=false);
    void addValue(App::DocumentObject *obj, std::vector<std::string> &&SubList={}, bool reset=false);

    /**
     * @brief setValue: PropertyLinkSub-compatible overload
     * @param SubList
     */
    void setValue(App::DocumentObject *lValue, const std::vector<std::string> &SubList={});

    std::vector<DocumentObject*> getValues() const;

    const std::string getPyReprString() const;

    DocumentObject* getValue() const;

    const std::vector<std::string> &getSubValues(App::DocumentObject *obj) const;

    std::vector<std::string> getSubValues(App::DocumentObject *obj, bool newStyle) const;

    const std::vector<ShadowSub> &getShadowSubs(App::DocumentObject *obj) const;

    /**
     * @brief Removes all occurrences of \a lValue in the property
     * together with its sub-elements and returns the number of entries removed.
     */
    int removeValue(App::DocumentObject *lValue);

    void setSubListValues(const std::vector<PropertyLinkSubList::SubSet>&);

    const std::list<PropertyXLinkSub> &getSubListValues() const {
        return _Links;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    const char* getEditorName() const override
    { return "Gui::PropertyEditor::PropertyLinkListItem"; }

    Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    Property *CopyOnLabelChange(App::DocumentObject *obj,
            const std::string &ref, const char *newLabel) const override;

    Property *CopyOnLinkReplace(const App::DocumentObject *parent,
            App::DocumentObject *oldObj, App::DocumentObject *newObj) const override;

    unsigned int getMemSize () const override;

    void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    bool referenceChanged() const override;

    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    void breakLink(App::DocumentObject *obj, bool clear) override;

    bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    bool upgrade(Base::XMLReader &reader, const char *typeName);

    int checkRestore(std::string *msg=nullptr) const override;

    void setAllowPartial(bool enable) override;

    void hasSetChildValue(Property &) override;
    void aboutToSetChildValue(Property &) override;

    void setSyncSubObject(bool enable);

protected:
    std::list<PropertyXLinkSub> _Links;
};


/** Link to one or more (sub)object(s) of one or more object(s) from the same or different document
 *
 * The only difference for PropertyXLinkList and PropertyXLinkSubList is in
 * their getPyObject().  PropertyXLinkList will return a list of object is
 * there is no sub-object/sub-elements in the property.
 */
class AppExport PropertyXLinkList: public PropertyXLinkSubList {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyXLinkList();
    ~PropertyXLinkList() override;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;
};


/** Abstract property that can link to multiple external objects
 *
 * @sa See PropertyExpressionEngine for example usage
 */
class AppExport PropertyXLinkContainer : public PropertyLinkBase {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    PropertyXLinkContainer();
    ~PropertyXLinkContainer() override;

    void afterRestore() override;
    int checkRestore(std::string *msg=nullptr) const override;
    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;
    void breakLink(App::DocumentObject *obj, bool clear) override;
    void getLinks(std::vector<App::DocumentObject *> &objs,
            bool all=false, std::vector<std::string> *subs=nullptr, bool newStyle=true) const override;

    bool isLinkedToDocument(const App::Document &doc) const;

protected:
    void aboutToSetChildValue(App::Property &prop) override;
    virtual PropertyXLink *createXLink();
    virtual void onBreakLink(App::DocumentObject *obj);
    virtual void onAddDep(App::DocumentObject *) {}
    virtual void onRemoveDep(App::DocumentObject *) {}
    void updateDeps(std::map<DocumentObject*,bool> &&newDeps);
    void clearDeps();

protected:
    std::map<App::DocumentObject*,bool> _Deps;
    std::map<std::string, std::unique_ptr<PropertyXLink> > _XLinks;
    std::map<std::string, std::string> _DocMap;
    bool _LinkRestored;

private:
    struct RestoreInfo {
        std::unique_ptr<PropertyXLink> xlink;
        std::string docName;
        std::string docLabel;
        bool hidden=false;
    };
    std::unique_ptr<std::vector<RestoreInfo> > _XLinkRestores;
};

} // namespace App


#endif // APP_PROPERTYLINKS_H
