/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef APP_LINK_H
#define APP_LINK_H

#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include "DocumentObject.h"
#include "FeaturePython.h"
#include "PropertyLinks.h"
#include "DocumentObjectExtension.h"
#include "FeaturePython.h"
#include "GroupExtension.h"

#define LINK_THROW(_type,_msg) do{\
    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))\
        FC_ERR(_msg);\
    throw _type(_msg);\
}while(0)

namespace App
{

class AppExport LinkBaseExtension : public App::DocumentObjectExtension 
{
    EXTENSION_PROPERTY_HEADER(App::LinkExtension);
    typedef App::DocumentObjectExtension inherited;

public:
    LinkBaseExtension();
    virtual ~LinkBaseExtension();

    PropertyBool _LinkRecomputed;
    PropertyLinkList _ChildCache; // cache for plain group expansion

    enum {
        LinkModeNone,
        LinkModeAutoDelete,
        LinkModeAutoLink,
        LinkModeAutoUnlink,
    };

    /** \name Parameter definition
     *
     * Parameter definition (Name, Type, Property Type, Default, Document).
     * The variadic is here so that the parameter can be extended by adding
     * extra fields.  See LINK_PARAM_EXT() for an example
     */
    //@{

#define LINK_PARAM_LINK_PLACEMENT(...) \
    (LinkPlacement, Base::Placement, App::PropertyPlacement, Base::Placement(), "Link placement", ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENT(...) \
    (Placement, Base::Placement, App::PropertyPlacement, Base::Placement(), \
     "Alias to LinkPlacement to make the link object compatibale with other objects", ##__VA_ARGS__)

#define LINK_PARAM_OBJECT(...) \
    (LinkedObject, App::DocumentObject*, App::PropertyLink, 0, "Linked object", ##__VA_ARGS__)

#define LINK_PARAM_SUB_ELEMENT(...) \
    (SubElements, std::vector<std::string>, App::PropertyStringList, std::vector<std::string>(), \
     "Non-object Sub-element list of the linked object, e.g. Face1", ##__VA_ARGS__)

#define LINK_PARAM_TRANSFORM(...) \
    (LinkTransform, bool, App::PropertyBool, false, \
      "Set to false to override linked object's placement", ##__VA_ARGS__)

#define LINK_PARAM_SCALE(...) \
    (Scale, double, App::PropertyFloat, 1.0, "Scale factor", ##__VA_ARGS__)

#define LINK_PARAM_SCALE_VECTOR(...) \
    (ScaleVector, Base::Vector3d, App::PropertyVector, Base::Vector3d(1,1,1), "Scale factors", ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENTS(...) \
    (PlacementList, std::vector<Base::Placement>, App::PropertyPlacementList, std::vector<Base::Placement>(),\
      "The placement for each link element", ##__VA_ARGS__)

#define LINK_PARAM_SCALES(...) \
    (ScaleList, std::vector<Base::Vector3d>, App::PropertyVectorList, std::vector<Base::Vector3d>(),\
      "The scale factors for each link element", ##__VA_ARGS__)

#define LINK_PARAM_VISIBILITIES(...) \
    (VisibilityList, boost::dynamic_bitset<>, App::PropertyBoolList, boost::dynamic_bitset<>(),\
      "The visibility state of each link element", ##__VA_ARGS__)

#define LINK_PARAM_COUNT(...) \
    (ElementCount, int, App::PropertyInteger, 0, "Link element count", ##__VA_ARGS__)

#define LINK_PARAM_ELEMENTS(...) \
    (ElementList, std::vector<App::DocumentObject*>, App::PropertyLinkList, std::vector<App::DocumentObject*>(),\
      "The link element object list", ##__VA_ARGS__)

#define LINK_PARAM_SHOW_ELEMENT(...) \
    (ShowElement, bool, App::PropertyBool, true, "Enable link element list", ##__VA_ARGS__)

#define LINK_PARAM_MODE(...) \
    (LinkMode, long, App::PropertyEnumeration, ((long)0), "Link group mode", ##__VA_ARGS__)

#define LINK_PARAM_COLORED_ELEMENTS(...) \
    (ColoredElements, App::DocumentObject*, App::PropertyLinkSubHidden, \
     0, "Link colored elements", ##__VA_ARGS__)

#define LINK_PARAM(_param) (LINK_PARAM_##_param())

#define LINK_PNAME(_param) BOOST_PP_TUPLE_ELEM(0,_param)
#define LINK_PTYPE(_param) BOOST_PP_TUPLE_ELEM(1,_param)
#define LINK_PPTYPE(_param) BOOST_PP_TUPLE_ELEM(2,_param)
#define LINK_PDEF(_param) BOOST_PP_TUPLE_ELEM(3,_param)
#define LINK_PDOC(_param) BOOST_PP_TUPLE_ELEM(4,_param)

#define LINK_PINDEX(_param) BOOST_PP_CAT(Prop,LINK_PNAME(_param))
    //@}

#define LINK_PARAMS \
    LINK_PARAM(PLACEMENT)\
    LINK_PARAM(LINK_PLACEMENT)\
    LINK_PARAM(OBJECT)\
    LINK_PARAM(SUB_ELEMENT)\
    LINK_PARAM(TRANSFORM)\
    LINK_PARAM(SCALE)\
    LINK_PARAM(SCALE_VECTOR)\
    LINK_PARAM(PLACEMENTS)\
    LINK_PARAM(SCALES)\
    LINK_PARAM(VISIBILITIES)\
    LINK_PARAM(COUNT)\
    LINK_PARAM(ELEMENTS)\
    LINK_PARAM(SHOW_ELEMENT)\
    LINK_PARAM(MODE)\
    LINK_PARAM(COLORED_ELEMENTS)

    enum PropIndex {
#define LINK_PINDEX_DEFINE(_1,_2,_param) LINK_PINDEX(_param),

        // defines Prop##Name enumeration value
        BOOST_PP_SEQ_FOR_EACH(LINK_PINDEX_DEFINE,_,LINK_PARAMS)
        PropMax
    };

    virtual void setProperty(int idx, Property *prop);
    Property *getProperty(int idx);
    Property *getProperty(const char *);

    struct PropInfo {
        int index;
        const char *name;
        Base::Type type;
        const char *doc;

        PropInfo(int index, const char *name,Base::Type type,const char *doc)
            :index(index),name(name),type(type),doc(doc)
        {}

        PropInfo() {}
    };

#define LINK_PROP_INFO(_1,_var,_param) \
    _var.push_back(PropInfo(BOOST_PP_CAT(Prop,LINK_PNAME(_param)),\
                            BOOST_PP_STRINGIZE(LINK_PNAME(_param)),\
                            LINK_PPTYPE(_param)::getClassTypeId(), \
                            LINK_PDOC(_param)));

    virtual const std::vector<PropInfo> &getPropertyInfo() const;

    typedef std::map<std::string, PropInfo> PropInfoMap;
    virtual const PropInfoMap &getPropertyInfoMap() const;

#define LINK_PROP_GET(_1,_2,_param) \
    LINK_PTYPE(_param) BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Value)) () const {\
        auto prop = props[LINK_PINDEX(_param)];\
        if(!prop) return LINK_PDEF(_param);\
        return static_cast<const LINK_PPTYPE(_param) *>(prop)->getValue();\
    }\
    const LINK_PPTYPE(_param) *BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property)) () const {\
        auto prop = props[LINK_PINDEX(_param)];\
        return static_cast<const LINK_PPTYPE(_param) *>(prop);\
    }\
    LINK_PPTYPE(_param) *BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property)) () {\
        auto prop = props[LINK_PINDEX(_param)];\
        return static_cast<LINK_PPTYPE(_param) *>(prop);\
    }\

    // defines get##Name() and get##Name##Property() accessor
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_GET,_,LINK_PARAMS)

    PropertyLinkList *_getElementListProperty() const;
    const std::vector<App::DocumentObject*> &_getElementListValue() const;

    PropertyBool *_getShowElementProperty() const;
    bool _getShowElementValue() const;

    PropertyInteger *_getElementCountProperty() const;
    int _getElementCountValue() const;

    std::vector<DocumentObject*> getLinkedChildren(bool filter=true) const;

    const char *flattenSubname(const char *subname) const;
    void expandSubname(std::string &subname) const;

    DocumentObject *getLink(int depth=0) const;

    Base::Matrix4D getTransform(bool transform) const;
    Base::Vector3d getScaleVector() const;

    App::GroupExtension *linkedPlainGroup() const;

    bool linkTransform() const;

    const char *getSubName() const { 
        parseSubName();
        return mySubName.size()?mySubName.c_str():0;
    }
    const char *getSubElement() const { 
        parseSubName();
        return mySubElement.size()?mySubElement.c_str():0;
    }

    bool extensionGetSubObject(DocumentObject *&ret, const char *subname, 
            PyObject **pyObj=0, Base::Matrix4D *mat=0, bool transform=false, int depth=0) const override;

    bool extensionGetSubObjects(std::vector<std::string>&ret, int reason) const override;

    bool extensionGetLinkedObject(DocumentObject *&ret, 
            bool recurse, Base::Matrix4D *mat, bool transform, int depth) const override;

    virtual App::DocumentObjectExecReturn *extensionExecute(void) override;
    virtual short extensionMustExecute(void) override;
    virtual void extensionOnChanged(const Property* p) override;
    virtual bool extensionOnNotification(App::DocumentObject *obj, const App::Property *prop) override;
    virtual void onExtendedUnsetupObject () override;
    virtual void onExtendedDocumentRestored() override;

    virtual int extensionSetElementVisible(const char *, bool) override;
    virtual int extensionIsElementVisible(const char *) override;
    virtual bool extensionHasChildElement() const override;

    virtual PyObject* getExtensionPyObject(void) override;

    static int getArrayIndex(const char *subname, const char **psubname=0);
    int getElementIndex(const char *subname, const char **psubname=0) const;
    void elementNameFromIndex(int idx, std::ostream &ss) const;

    DocumentObject *getContainer();
    const DocumentObject *getContainer() const;

    void setLink(int index, DocumentObject *obj, const char *subname=0,
        const std::vector<std::string> &subs = std::vector<std::string>());

    DocumentObject *getTrueLinkedObject(bool recurse,
            Base::Matrix4D *mat=0,int depth=0, bool noElement=false) const;

    typedef std::map<const Property*,std::pair<LinkBaseExtension*,int> > LinkPropMap;

    bool hasPlacement() const {
        return getLinkPlacementProperty() || getPlacementProperty();
    }

    void cacheChildLabel(int enable=-1) const;

protected:
    void parseSubName() const;
    void update(App::DocumentObject *parent, const Property *prop);
    void syncElementList();
    void detachElement(App::DocumentObject *obj);
    void checkGeoElementMap(const App::DocumentObject *obj, 
        const App::DocumentObject *linked, PyObject **pyObj, const char *postfix) const;
    bool updateGroup(App::DocumentObject *obj = 0);

protected:
    std::vector<Property *> props;
    std::set<const App::DocumentObject*> myHiddenElements;
    mutable std::string mySubElement;
    mutable std::string mySubName;

    mutable std::unordered_map<std::string,int> myLabelCache; // for label based subname lookup
    mutable bool enableLabelCache;

    // WARNING! Do not try to access through myOwner, the object may have been
    // deleted. Its purpose here is just to distinguish the owner.
    LinkBaseExtension *myOwner;
};

///////////////////////////////////////////////////////////////////////////

typedef ExtensionPythonT<LinkBaseExtension> LinkBaseExtensionPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkExtension : public LinkBaseExtension
{
    EXTENSION_PROPERTY_HEADER(App::LinkExtension);
    typedef LinkBaseExtension inherited;

public:
    LinkExtension();
    virtual ~LinkExtension();

    /** \name Helpers for defining extended parameter 
     *
     * extended parameter definition 
     * (Name, Type, Property_Type, Default, Document, Property_Name, 
     *  Derived_Property_Type, App_Property_Type, Group)
     *
     * This helper simply reuses Name as Property_Name, Property_Type as
     * Derived_Property_type, Prop_None as App_Propert_Type
     *
     * Note: Because PropertyView will merge linked object's properties into
     * ours, we set the default group name as ' Link' with a leading space to
     * try to make our group before others
     */
    //@{

#define LINK_ENAME(_param) BOOST_PP_TUPLE_ELEM(5,_param)
#define LINK_ETYPE(_param) BOOST_PP_TUPLE_ELEM(6,_param)
#define LINK_EPTYPE(_param) BOOST_PP_TUPLE_ELEM(7,_param)
#define LINK_EGROUP(_param) BOOST_PP_TUPLE_ELEM(8,_param)

#define _LINK_PROP_ADD(_add_property, _param) \
    _add_property(BOOST_PP_STRINGIZE(LINK_ENAME(_param)),LINK_ENAME(_param),\
            (LINK_PDEF(_param)),LINK_EGROUP(_param),LINK_EPTYPE(_param),LINK_PDOC(_param));\
    setProperty(LINK_PINDEX(_param),&LINK_ENAME(_param));

#define LINK_PROP_ADD(_1,_2,_param) \
    _LINK_PROP_ADD(_ADD_PROPERTY_TYPE,_param);

#define LINK_PROP_ADD_EXTENSION(_1,_2,_param) \
    _LINK_PROP_ADD(_EXTENSION_ADD_PROPERTY_TYPE,_param);

#define LINK_PROPS_ADD(_seq) \
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD,_,_seq)

#define LINK_PROPS_ADD_EXTENSION(_seq) \
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD_EXTENSION,_,_seq)

#define _LINK_PROP_SET(_1,_2,_param) \
    setProperty(LINK_PINDEX(_param),&LINK_ENAME(_param));

#define LINK_PROPS_SET(_seq) BOOST_PP_SEQ_FOR_EACH(_LINK_PROP_SET,_,_seq)

    /// Helper for defining default extended parameter
#define _LINK_PARAM_EXT(_name,_type,_ptype,_def,_doc,...) \
    ((_name,_type,_ptype,_def,_doc,_name,_ptype,App::Prop_None," Link"))

    /** Define default extended parameter
     * It simply reuses Name as Property_Name, Property_Type as
     * Derived_Property_Type, and App::Prop_None as App::PropertyType
     */
#define LINK_PARAM_EXT(_param) BOOST_PP_EXPAND(_LINK_PARAM_EXT LINK_PARAM_##_param())

    /// Helper for extended parameter with app property type
#define _LINK_PARAM_EXT_ATYPE(_name,_type,_ptype,_def,_doc,_atype) \
    ((_name,_type,_ptype,_def,_doc,_name,_ptype,_atype," Link"))

    /// Define extended parameter with app property type
#define LINK_PARAM_EXT_ATYPE(_param,_atype) \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_ATYPE LINK_PARAM_##_param(_atype))

    /// Helper for extended parameter with derived property type
#define _LINK_PARAM_EXT_TYPE(_name,_type,_ptype,_def,_doc,_dtype) \
    ((_name,_type,_ptype,_def,_doc,_name,_dtype,App::Prop_None," Link"))

    /// Define extended parameter with derived property type
#define LINK_PARAM_EXT_TYPE(_param,_dtype) \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_TYPE LINK_PARAM_##_param(_dtype))

    /// Helper for extended parameter with a different property name
#define _LINK_PARAM_EXT_NAME(_name,_type,_ptype,_def,_doc,_pname) \
    ((_name,_type,_ptype,_def,_doc,_pname,_ptype,App::Prop_None," Link"))

    /// Define extended parameter with a different property name
#define LINK_PARAM_EXT_NAME(_param,_pname) BOOST_PP_EXPAND(_LINK_PARAM_EXT_NAME LINK_PARAM_##_param(_pname))
    //@}

#define LINK_PARAMS_EXT \
    LINK_PARAM_EXT(SCALE)\
    LINK_PARAM_EXT(SCALES)\
    LINK_PARAM_EXT(VISIBILITIES)\
    LINK_PARAM_EXT(PLACEMENTS)\
    LINK_PARAM_EXT(ELEMENTS)

#define LINK_PROP_DEFINE(_1,_2,_param) LINK_ETYPE(_param) LINK_ENAME(_param);
#define LINK_PROPS_DEFINE(_seq) BOOST_PP_SEQ_FOR_EACH(LINK_PROP_DEFINE,_,_seq)
    
    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_EXT)

    void onExtendedDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_EXT);
        inherited::onExtendedDocumentRestored();
    }
};

///////////////////////////////////////////////////////////////////////////

typedef ExtensionPythonT<LinkExtension> LinkExtensionPython;

///////////////////////////////////////////////////////////////////////////

class AppExport Link : public App::DocumentObject, public App::LinkExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::Link);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_LINK \
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(TRANSFORM)\
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(SUB_ELEMENT)\
    LINK_PARAM_EXT(SHOW_ELEMENT)\
    LINK_PARAM_EXT_TYPE(COUNT,App::PropertyIntegerConstraint)\
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS,App::Prop_Hidden)

    LINK_PROPS_DEFINE(LINK_PARAMS_LINK)

    Link(void);

    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_LINK);
        inherited::onDocumentRestored();
    }

    bool canLinkProperties() const override;
};

typedef App::FeaturePythonT<Link> LinkPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkElement : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkElement);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_ELEMENT \
    LINK_PARAM_EXT(SCALE)\
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(TRANSFORM) \
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(SUB_ELEMENT)

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_ELEMENT)

    LinkElement();
    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_ELEMENT);
        inherited::onDocumentRestored();
    }

    bool canDelete() const {return myOwner==0;}
};

typedef App::FeaturePythonT<LinkElement> LinkElementPython;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkGroup : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkGroup);
    typedef App::DocumentObject inherited;
public:

#define LINK_PARAMS_GROUP \
    LINK_PARAM_EXT(ELEMENTS)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(VISIBILITIES)\
    LINK_PARAM_EXT(MODE)\
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS,App::Prop_Hidden)

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_GROUP)

    LinkGroup();

    const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_GROUP);
        inherited::onDocumentRestored();
    }
};

typedef App::FeaturePythonT<LinkGroup> LinkGroupPython;

} //namespace App


#endif // APP_LINK_H
