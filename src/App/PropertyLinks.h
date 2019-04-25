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
#include <map>
#include <list>
#include <string>
#include <memory>
#include <cinttypes>
#include "Property.h"

namespace Base {
class Writer;
}

namespace App
{
class DocumentObject;
class Document;

class DocInfo;
typedef std::shared_ptr<DocInfo> DocInfoPtr;

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
    LinkScope getScope() {return _pcScope;};
    
protected:
    LinkScope _pcScope = LinkScope::Local;
};

/// Parent class of all link type properties
class AppExport PropertyLinkBase : public Property, public ScopedLink
{
    TYPESYSTEM_HEADER();
public:
    typedef std::pair<std::string,std::string> ShadowSub;

    PropertyLinkBase();
    virtual ~PropertyLinkBase();

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
     *  @param reset: if ture, then calls unregisterLabelReference() before
     *  registering
     */
    void registerLabelReferences(std::vector<std::string> &&labels, bool reset=true);

    /** Check subnames for label registeration
     *
     *  @param subs: subname references
     *  @param reset: if ture, then calls unregisterLabelReference() before
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
     * @return For external linked object, return non zero in case the link is
     * missing, or the time stamp has changed.
     */
    virtual int checkRestore() const {return 0;}

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
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const = 0;

    /** Called to reset this link property
     *
     * @param obj: reset link property if it is linked to this object
     * @param clear: if true, then also reset property if the owner of this proeprty is \a obj
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
        return 0;
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
        return 0;
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

    void setAllowExternal(bool allow);

    /// Helper functions
    //@{

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
     * @param clear: if ture, then also reset property if the owner of the link property is \a link
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
     * @param obj: linked object
     * @param sub: subname reference
     * @param check: if Ture, then the return string will be empty if non of
     *               sub object referenced is exporting.
     * 
     * @return The subname reference suitable for exporting
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
    static std::string exportSubName(const App::DocumentObject *obj, const char *sub, bool check=false);

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
    static void restoreLabelReference(const App::DocumentObject *obj, std::string &sub, ShadowSub *shadow=0);

    /** Helper function to extract labels from a subname reference
     *
     * @param labels: output vector of extracted labels
     * @param subname: subname reference
     *
     * @sa registerLabelReferences()
     *
     * This function is used to extrac label from subname reference for
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
    };
    inline bool testFlag(int flag) const {
        return _Flags.test((std::size_t)flag);
    }

    virtual void setAllowPartial(bool enable) { (void)enable; }

protected:
    virtual void hasSetValue() override;

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

    void resetLink();

    /** Sets the property
     */
    virtual void setValue(App::DocumentObject *);

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

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

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

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkHidden : public PropertyLink
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkHidden() {_pcScope = LinkScope::Hidden;};
};


class AppExport PropertyLinkListBase: public PropertyLinkBase, public PropertyListsBase
{
    TYPESYSTEM_HEADER();
public:
    virtual void setPyObject(PyObject *obj) override {
        _setPyObject(obj);
    }
};

class AppExport PropertyLinkList : 
    public PropertyListsT<DocumentObject*,std::vector<DocumentObject*>, PropertyLinkListBase>
{
    TYPESYSTEM_HEADER();
    typedef PropertyListsT<DocumentObject*,std::vector<DocumentObject*>,PropertyLinkListBase> inherited;

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
    virtual void setSize(int newSize, const_reference def);

    /** Sets the property
    */
    void setValues(const std::vector<DocumentObject*>&) override;

    void set1Value(int idx, DocumentObject * const &value, bool touch=false) override;

    virtual PyObject *getPyObject(void);

    virtual void Save(Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize(void) const;
    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyLinkListItem"; }

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    DocumentObject *find(const std::string &, int *pindex=0) const;
    DocumentObject *find(const char *sub, int *pindex=0) const {
        if(!sub) return 0;
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

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkListHidden : public PropertyLinkList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkListHidden() {_pcScope = LinkScope::Hidden;};
};

class PropertyXLinkSub;

/** the Link Property with sub elements
 *  This property links an object and a defined sequence of
 *  sub elements. These subelements (like Edges of a Shape)
 *  are stored as names, which can be resolved by the 
 *  ComplexGeoDataType interface to concrete sub objects.
 */
class AppExport PropertyLinkSub : public PropertyLinkBase
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

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    /** Sets the property
     */
    void setValue(App::DocumentObject *,const std::vector<std::string> &SubList,
                    std::vector<ShadowSub> &&ShadowSubList={});
    void setValue(App::DocumentObject *,std::vector<std::string> &&SubList={},
                    std::vector<ShadowSub> &&ShadowSubList={});

    /** This method returns the linked DocumentObject
     */
    App::DocumentObject * getValue(void) const;

    /// return the list of sub elements 
    const std::vector<std::string>& getSubValues(void) const;

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
    inline _type getValue(void) const {
        return _pcLinkSub ? dynamic_cast<_type>(_pcLinkSub) : 0;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    /// Return a copy of the property if any changes caused by importing external object 
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const override;

    virtual unsigned int getMemSize (void) const{
        return sizeof(App::DocumentObject *);
    }

    virtual void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    virtual bool referenceChanged() const override;

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

protected:
    App::DocumentObject*     _pcLinkSub;
    std::vector<std::string> _cSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
    bool _restoreLabel;
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

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubHidden : public PropertyLinkSub
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubHidden() {_pcScope = LinkScope::Hidden;};
};

class AppExport PropertyLinkSubList : public PropertyLinkBase
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

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    int getSize(void) const;
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

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    /// Return a copy of the property if any changes caused by importing external object 
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const override;

    virtual unsigned int getMemSize (void) const;

    virtual void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    virtual bool referenceChanged() const override;

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

private:
    //FIXME: Do not make two independent lists because this will lead to some inconsistencies!
    std::vector<DocumentObject*> _lValueList;
    std::vector<std::string>     _lSubList;
    std::vector<ShadowSub> _ShadowSubList;
    std::vector<int> _mapped;
    bool _restoreLabel;
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

/** The general Link Property that are hidden from dependency checking
 */
class AppExport PropertyLinkSubListHidden : public PropertyLinkSubList
{
    TYPESYSTEM_HEADER();
public:
    PropertyLinkSubListHidden() {_pcScope = LinkScope::Hidden;};
};

class PropertyXLinkSubList;

/** Link to an (sub)object in the same or different document
 */
class AppExport PropertyXLink : public PropertyLinkGlobal
{
    TYPESYSTEM_HEADER();

public:
    PropertyXLink(bool allowPartial=false, PropertyLinkBase *parent=0);

    virtual ~PropertyXLink();

    PropertyLinkBase *parent() const { return parentProp; }

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    void setValue(App::DocumentObject *) override;
    void setValue(App::DocumentObject *, const char *subname);

    const char *getSubName(bool newStyle=true) const;
    void setSubName(const char *subname);
    
    bool hasSubName() const {return !_SubList.empty();}

    App::Document *getDocument() const;
    const char *getDocumentPath() const;
    const char *getObjectName() const;

    virtual int checkRestore() const override;

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    /// Return a copy of the property if any changes caused by importing external object 
    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const override;

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    friend class DocInfo;

    static bool supportXLink(const App::Property *prop);
    static bool hasXLink(const App::Document *doc);
    static bool hasXLink(const std::vector<App::DocumentObject*> &objs, std::vector<App::Document*> *unsaved=0);
    static std::map<App::Document*,std::set<App::Document*> > getDocumentOutList(App::Document *doc=0);
    static std::map<App::Document*,std::set<App::Document*> > getDocumentInList(App::Document *doc=0);

    virtual void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    virtual bool referenceChanged() const override;

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    // The following APIs are provided to be compatible with PropertyLinkSub.
    // Note that although PropertyXLink is capable of holding multiple subnames,
    // there no public APIs allowing user to set more that one subname. Multiple
    // subname adding API is published in PropertyXLinkSub.

    const std::vector<std::string>& getSubValues(void) const {
        return _SubList;
    }
    const std::vector<ShadowSub > &getShadowSubs() const {
        return _ShadowSubList;
    }
    std::vector<std::string> getSubValues(bool newStyle) const;
    std::vector<std::string> getSubValuesStartsWith(const char*, bool newStyle=false) const;

    virtual void setAllowPartial(bool enable) override;

protected:
    void unlink();
    void detach();

    void restoreLink(App::DocumentObject *);

    void _setSubValues(std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    void _setValue(std::string &&filePath, std::string &&objectName, std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    void _setValue(App::DocumentObject *,std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList = {});

    virtual PropertyXLink *createInstance() const;

    virtual bool upgrade(Base::XMLReader &reader, const char *typeName);

    void copyTo(PropertyXLink &other, App::DocumentObject *linked=0, std::vector<std::string> *subs=0) const;

    virtual void aboutToSetValue() override;

    virtual void hasSetValue() override;

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
    TYPESYSTEM_HEADER();

public:
    PropertyXLinkSub(bool allowPartial=false, PropertyLinkBase *parent=0);

    virtual ~PropertyXLinkSub();

    void setValue(App::DocumentObject *,const std::vector<std::string> &SubList, 
            std::vector<ShadowSub > &&ShadowSubList={});

    void setValue(App::DocumentObject *,std::vector<std::string> &&SubList={},
            std::vector<ShadowSub > &&ShadowSubList={});

    void setSubValues(std::vector<std::string> &&SubList,
            std::vector<ShadowSub> &&ShadowSubList={});

    virtual bool upgrade(Base::XMLReader &reader, const char *typeName) override;

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

protected:
    virtual PropertyXLink *createInstance() const;
};


/** Link to one or more (sub)object(s) of one or more object(s) from the same or different document
 */
class AppExport PropertyXLinkSubList: public PropertyLinkBase {
    TYPESYSTEM_HEADER();

public:
    PropertyXLinkSubList();
    virtual ~PropertyXLinkSubList();

    virtual void afterRestore() override;
    virtual void onContainerRestored() override;

    int getSize(void) const;

    /** Sets the property.
     * setValue(0, whatever) clears the property
     */
    void setValue(DocumentObject*,const char*);
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
    void setValue(App::DocumentObject *lValue, const std::vector<std::string> &SubList=std::vector<std::string>());

    std::vector<DocumentObject*> getValues(void);

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

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual Property *CopyOnImportExternal(const std::map<std::string,std::string> &nameMap) const override;

    virtual Property *CopyOnLabelChange(App::DocumentObject *obj, 
            const std::string &ref, const char *newLabel) const override;

    virtual unsigned int getMemSize (void) const;

    virtual void updateElementReference(
            DocumentObject *feature,bool reverse=false, bool notify=false) override;

    virtual bool referenceChanged() const override;

    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;

    virtual void breakLink(App::DocumentObject *obj, bool clear) override;

    virtual bool adjustLink(const std::set<App::DocumentObject *> &inList) override;

    bool upgrade(Base::XMLReader &reader, const char *typeName);

    virtual int checkRestore() const override;

    virtual void setAllowPartial(bool enable) override;

    virtual void hasSetChildValue(Property &) override;
    virtual void aboutToSetChildValue(Property &) override;

protected:
    std::list<PropertyXLinkSub> _Links;
};

/** Abstract property that can link to multiple external objects
 *
 * @sa See PropertyExpressionEngine for example usage
 */
class AppExport PropertyXLinkContainer : public PropertyLinkBase {
    TYPESYSTEM_HEADER();
public:
    PropertyXLinkContainer();
    ~PropertyXLinkContainer();

    virtual void afterRestore() override;
    virtual int checkRestore() const override;
    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;
    virtual void breakLink(App::DocumentObject *obj, bool clear) override;
    virtual void getLinks(std::vector<App::DocumentObject *> &objs, 
            bool all=false, std::vector<std::string> *subs=0, bool newStyle=true) const override;
    
    bool isLinkedToDocument(const App::Document &doc) const;

protected:
    virtual void aboutToSetChildValue(App::Property &prop) override;
    virtual PropertyXLink *createXLink();
    virtual void onBreakLink(App::DocumentObject *obj);
    virtual void onAddDep(App::DocumentObject *) {}
    virtual void onRemoveDep(App::DocumentObject *) {}
    void updateDeps(std::set<DocumentObject*> &&newDeps);
    void clearDeps();

protected:
    std::set<App::DocumentObject*> _Deps;
    std::map<std::string, std::unique_ptr<PropertyXLink> > _XLinks;
    std::map<std::string, std::string> _DocMap;
    bool _LinkRestored;

private:
    struct RestoreInfo {
        std::unique_ptr<PropertyXLink> xlink;
        std::string docName;
        std::string docLabel;
    };
    std::unique_ptr<std::vector<RestoreInfo> > _XLinkRestores;
};

} // namespace App


#endif // APP_PROPERTYLINKS_H
