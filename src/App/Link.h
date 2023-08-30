/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <unordered_set>
#include <Base/Parameter.h>
#include <Base/Bitmask.h>
#include "DocumentObject.h"
#include "DocumentObjectExtension.h"
#include "FeaturePython.h"
#include "GroupExtension.h"
#include "PropertyLinks.h"


//FIXME: ISO C++11 requires at least one argument for the "..." in a variadic macro
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define LINK_THROW(_type,_msg) do{\
    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))\
        FC_ERR(_msg);\
    throw _type(_msg);\
}while(0)

namespace App
{

class AppExport LinkBaseExtension : public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    using inherited = App::DocumentObjectExtension;

public:
    LinkBaseExtension();
    ~LinkBaseExtension() override = default;

    PropertyBool _LinkTouched;
    PropertyInteger _LinkOwner;
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

#define LINK_PARAM_TRANSFORM(...) \
    (LinkTransform, bool, App::PropertyBool, false, \
      "Set to false to override linked object's placement", ##__VA_ARGS__)

#define LINK_PARAM_CLAIM_CHILD(...) \
    (LinkClaimChild, bool, App::PropertyBool, false, \
      "Claim the linked object as a child", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE(...) \
    (LinkCopyOnChange, long, App::PropertyEnumeration, ((long)0), \
      "Disabled: disable copy on change\n"\
      "Enabled: enable copy linked object on change of any of its properties marked as CopyOnChange\n"\
      "Owned: indicate the linked object has been copied and is own owned by the link. And the\n"\
      "       the link will try to sync any change of the original linked object back to the copy.",\
      ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_SOURCE(...) \
    (LinkCopyOnChangeSource, App::DocumentObject*, App::PropertyLink, 0, "The copy on change source object", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_GROUP(...) \
    (LinkCopyOnChangeGroup, App::DocumentObject*, App::PropertyLink, 0, \
     "Linked to a internal group object for holding on change copies", ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_TOUCHED(...) \
    (LinkCopyOnChangeTouched, bool, App::PropertyBool, 0, "Indicating the copy on change source object has been changed", ##__VA_ARGS__)

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

#define LINK_PARAM_LINK_EXECUTE(...) \
    (LinkExecute, const char*, App::PropertyString, (""),\
     "Link execute function. Default to 'appLinkExecute'. 'None' to disable.", ##__VA_ARGS__)

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
    LINK_PARAM(CLAIM_CHILD)\
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
    LINK_PARAM(LINK_EXECUTE)\
    LINK_PARAM(COLORED_ELEMENTS)\
    LINK_PARAM(COPY_ON_CHANGE)\
    LINK_PARAM(COPY_ON_CHANGE_SOURCE)\
    LINK_PARAM(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM(COPY_ON_CHANGE_TOUCHED)\

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
            : index(index), name(name), type(type), doc(doc)
        {}

        PropInfo() : index(0), name(nullptr), doc(nullptr) {}
    };

#define LINK_PROP_INFO(_1,_var,_param) \
    _var.push_back(PropInfo(BOOST_PP_CAT(Prop,LINK_PNAME(_param)),\
                            BOOST_PP_STRINGIZE(LINK_PNAME(_param)),\
                            LINK_PPTYPE(_param)::getClassTypeId(), \
                            LINK_PDOC(_param)));

    virtual const std::vector<PropInfo> &getPropertyInfo() const;

    using PropInfoMap = std::map<std::string, PropInfo>;
    virtual const PropInfoMap &getPropertyInfoMap() const;

    enum LinkCopyOnChangeType {
        CopyOnChangeDisabled = 0,
        CopyOnChangeEnabled = 1,
        CopyOnChangeOwned = 2,
        CopyOnChangeTracking = 3
    };

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

    // defines get##Name##Property() and get##Name##Value() accessor
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
        return !mySubName.empty()?mySubName.c_str():nullptr;
    }

    const std::vector<std::string> &getSubElements() const {
        parseSubName();
        return mySubElements;
    }

    bool extensionGetSubObject(DocumentObject *&ret, const char *subname,
            PyObject **pyObj=nullptr, Base::Matrix4D *mat=nullptr, bool transform=false, int depth=0) const override;

    bool extensionGetSubObjects(std::vector<std::string>&ret, int reason) const override;

    bool extensionGetLinkedObject(DocumentObject *&ret,
            bool recurse, Base::Matrix4D *mat, bool transform, int depth) const override;

    App::DocumentObjectExecReturn *extensionExecute() override;
    short extensionMustExecute() override;
    void extensionOnChanged(const Property* p) override;
    void onExtendedUnsetupObject () override;
    void onExtendedDocumentRestored() override;

    int extensionSetElementVisible(const char *, bool) override;
    int extensionIsElementVisible(const char *) override;
    bool extensionHasChildElement() const override;

    PyObject* getExtensionPyObject() override;

    Property *extensionGetPropertyByName(const char* name) const override;

    static int getArrayIndex(const char *subname, const char **psubname=nullptr);
    int getElementIndex(const char *subname, const char **psubname=nullptr) const;
    void elementNameFromIndex(int idx, std::ostream &ss) const;

    DocumentObject *getContainer();
    const DocumentObject *getContainer() const;

    void setLink(int index, DocumentObject *obj, const char *subname=nullptr,
        const std::vector<std::string> &subs = std::vector<std::string>());

    DocumentObject *getTrueLinkedObject(bool recurse,
            Base::Matrix4D *mat=nullptr,int depth=0, bool noElement=false) const;

    using LinkPropMap = std::map<const Property*,std::pair<LinkBaseExtension*,int> >;

    bool hasPlacement() const {
        return getLinkPlacementProperty() || getPlacementProperty();
    }

    void cacheChildLabel(int enable=-1) const;

    static bool setupCopyOnChange(App::DocumentObject *obj, App::DocumentObject *linked,
            std::vector<boost::signals2::scoped_connection> *copyOnChangeConns, bool checkExisting);

    static bool isCopyOnChangeProperty(App::DocumentObject *obj, const Property &prop);

    void syncCopyOnChange();

    /** Options used in setOnChangeCopyObject()
     * Multiple options can be combined by bitwise or operator
     */
    enum class OnChangeCopyOptions {
        /// No options set
        None = 0,
        /// If set, then exclude the input from object list to copy on change, or else, include the input object.
        Exclude = 1,
        /// If set , then apply the setting to all links to the input object, or else, apply only to this link.
        ApplyAll = 2,
    };

    /** Include or exclude object from list of objects to copy on change
     * @param obj: input object
     * @param options: control options. @sa OnChangeCopyOptions.
     */
    void setOnChangeCopyObject(App::DocumentObject *obj, OnChangeCopyOptions options);

    std::vector<App::DocumentObject *> getOnChangeCopyObjects(
            std::vector<App::DocumentObject *> *excludes = nullptr,
            App::DocumentObject *src = nullptr);

    bool isLinkedToConfigurableObject() const;

    void monitorOnChangeCopyObjects(const std::vector<App::DocumentObject*> &objs);

    /// Check if the linked object is a copy on change
    bool isLinkMutated() const;

protected:
    void _handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName);
    void parseSubName() const;
    void update(App::DocumentObject *parent, const Property *prop);
    void checkCopyOnChange(App::DocumentObject *parent, const App::Property &prop);
    void setupCopyOnChange(App::DocumentObject *parent, bool checkSource = false);
    App::DocumentObject *makeCopyOnChange();
    void syncElementList();
    void detachElement(App::DocumentObject *obj);
    void detachElements();
    void checkGeoElementMap(const App::DocumentObject *obj,
        const App::DocumentObject *linked, PyObject **pyObj, const char *postfix) const;
    void updateGroup();
    void slotChangedPlainGroup(const App::DocumentObject &, const App::Property &);

protected:
    std::vector<Property *> props;
    std::unordered_set<const App::DocumentObject*> myHiddenElements;
    mutable std::vector<std::string> mySubElements;
    mutable std::string mySubName;

    std::unordered_map<const App::DocumentObject*,
        boost::signals2::scoped_connection> plainGroupConns;

    long prevLinkedObjectID = 0;

    mutable std::unordered_map<std::string,int> myLabelCache; // for label based subname lookup
    mutable bool enableLabelCache{false};
    bool hasOldSubElement{false};

    std::vector<boost::signals2::scoped_connection> copyOnChangeConns;
    std::vector<boost::signals2::scoped_connection> copyOnChangeSrcConns;
    bool hasCopyOnChange{true};

    mutable bool checkingProperty = false;
    bool pauseCopyOnChange = false;

    boost::signals2::scoped_connection connCopyOnChangeSource;
};

///////////////////////////////////////////////////////////////////////////

using LinkBaseExtensionPython = ExtensionPythonT<LinkBaseExtension>;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkExtension : public LinkBaseExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    using inherited = LinkBaseExtension;

public:
    LinkExtension();
    ~LinkExtension() override = default;

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
    LINK_PARAM_EXT_ATYPE(SCALE_VECTOR,App::Prop_Hidden)\
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

using LinkExtensionPython = ExtensionPythonT<LinkExtension>;

///////////////////////////////////////////////////////////////////////////

class AppExport Link : public App::DocumentObject, public App::LinkExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::Link);
    using inherited = App::DocumentObject;
public:

#define LINK_PARAMS_LINK \
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(CLAIM_CHILD)\
    LINK_PARAM_EXT(TRANSFORM)\
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(SHOW_ELEMENT)\
    LINK_PARAM_EXT_TYPE(COUNT,App::PropertyIntegerConstraint)\
    LINK_PARAM_EXT(LINK_EXECUTE)\
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS,App::Prop_Hidden)\
    LINK_PARAM_EXT(COPY_ON_CHANGE)\
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)\

    LINK_PROPS_DEFINE(LINK_PARAMS_LINK)

    Link();

    const char* getViewProviderName() const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_LINK);
        inherited::onDocumentRestored();
    }

    void handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName) override
    {
        _handleChangedPropertyName(reader,TypeName,PropName);
    }

    bool canLinkProperties() const override;
};

using LinkPython = App::FeaturePythonT<Link>;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkElement : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkElement);
    using inherited = App::DocumentObject;
public:

#define LINK_PARAMS_ELEMENT \
    LINK_PARAM_EXT(SCALE)\
    LINK_PARAM_EXT_ATYPE(SCALE_VECTOR,App::Prop_Hidden)\
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)\
    LINK_PARAM_EXT(TRANSFORM) \
    LINK_PARAM_EXT(LINK_PLACEMENT)\
    LINK_PARAM_EXT(PLACEMENT)\
    LINK_PARAM_EXT(COPY_ON_CHANGE)\
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)\
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)\

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_ELEMENT)

    LinkElement();
    const char* getViewProviderName() const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_ELEMENT);
        inherited::onDocumentRestored();
    }

    bool canDelete() const;

    void handleChangedPropertyName(Base::XMLReader &reader,
            const char * TypeName, const char *PropName) override
    {
        _handleChangedPropertyName(reader,TypeName,PropName);
    }
};

using LinkElementPython = App::FeaturePythonT<LinkElement>;

///////////////////////////////////////////////////////////////////////////

class AppExport LinkGroup : public App::DocumentObject, public App::LinkBaseExtension {
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkGroup);
    using inherited = App::DocumentObject;
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

    const char* getViewProviderName() const override{
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override {
        LINK_PROPS_SET(LINK_PARAMS_GROUP);
        inherited::onDocumentRestored();
    }
};

using LinkGroupPython = App::FeaturePythonT<LinkGroup>;

} //namespace App

ENABLE_BITMASK_OPERATORS(App::Link::OnChangeCopyOptions)

/*[[[cog
import LinkParams
LinkParams.declare()
]]]*/

namespace App {
/** Convenient class to obtain App::Link related parameters

 * The parameters are under group "User parameter:BaseApp/Preferences/Link"
 *
 * This class is auto generated by LinkParams.py. Modify that file
 * instead of this one, if you want to add any parameter. You need
 * to install Cog Python package for code generation:
 * @code
 *     pip install cogapp
 * @endcode
 *
 * Once modified, you can regenerate the header and the source file,
 * @code
 *     python3 -m cogapp -r Link.h Link.cpp
 * @endcode
 *
 * You can add a new parameter by adding lines in LinkParams.py. Available
 * parameter types are 'Int, UInt, String, Bool, Float'. For example, to add
 * a new Int type parameter,
 * @code
 *     ParamInt(parameter_name, default_value, documentation, on_change=False)
 * @endcode
 *
 * If there is special handling on parameter change, pass in on_change=True.
 * And you need to provide a function implementation in Link.cpp with
 * the following signature.
 * @code
 *     void LinkParams:on<parameter_name>Changed()
 * @endcode
 */
class AppExport LinkParams {
public:
    static ParameterGrp::handle getHandle();

    //@{
    /// Accessor for parameter CopyOnChangeApplyToAll
    ///
    /// Stores the last user choice of whether to apply CopyOnChange setup to all link
    /// that links to the same configurable object
    static const bool & getCopyOnChangeApplyToAll();
    static const bool & defaultCopyOnChangeApplyToAll();
    static void removeCopyOnChangeApplyToAll();
    static void setCopyOnChangeApplyToAll(const bool &v);
    static const char *docCopyOnChangeApplyToAll();
    //@}

    // Auto generated code. See class document of LinkParams.
};
} // namespace App
//[[[end]]]


#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#endif // APP_LINK_H
