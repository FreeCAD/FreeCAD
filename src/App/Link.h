// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <unordered_set>
#include <Base/Parameter.h>
#include <Base/Bitmask.h>
#include "DocumentObject.h"
#include "DocumentObjectExtension.h"
#include "FeaturePython.h"
#include "GroupExtension.h"
#include "PropertyLinks.h"


// FIXME: ISO C++11 requires at least one argument for the "..." in a variadic macro
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define LINK_THROW(_type, _msg)                                                                    \
    do {                                                                                           \
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))                                            \
            FC_ERR(_msg);                                                                          \
        throw _type(_msg);                                                                         \
    } while (0)

namespace App
{

/**
 * @brief The base class of the link extension.
 * @ingroup LinksGroup
 *
 * The functionality in this class is reused in LinkExtension, LinkElement, and
 * LinkGroup.
 */
class AppExport LinkBaseExtension: public App::DocumentObjectExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    using inherited = App::DocumentObjectExtension;

public:
    LinkBaseExtension();
    ~LinkBaseExtension() override = default;

    /// Whether the link has been touched (i.e. its linked object changed)
    PropertyBool _LinkTouched;
    /// Contains the object ID of the object that owns the link.
    PropertyInteger _LinkOwner;
    /// Cache of children of the link group.
    PropertyLinkList _ChildCache;  // cache for plain group expansion

    /// Options for the link mode.
    enum
    {
        LinkModeNone, ///< No mode for the link.
        LinkModeAutoDelete, ///< Delete the linked object when the link is deleted.
        LinkModeAutoLink,   ///< Create link elements of sub-element automatically (unused).
        LinkModeAutoUnlink, ///< Unused option.
    };

    /**
     * @name Parameter definition.
     * @brief Parameter definition (Name, Type, Property Type, Default, Document).
     *
     * The variadic is here so that the parameter can be extended by adding
     * extra fields.  See LINK_PARAM_EXT() for an example.
     *
     * @{
     */

#define LINK_PARAM_LINK_PLACEMENT(...)                                                             \
    (LinkPlacement,                                                                                \
     Base::Placement,                                                                              \
     App::PropertyPlacement,                                                                       \
     Base::Placement(),                                                                            \
     "Link placement",                                                                             \
     ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENT(...)                                                                  \
    (Placement,                                                                                    \
     Base::Placement,                                                                              \
     App::PropertyPlacement,                                                                       \
     Base::Placement(),                                                                            \
     "Alias to LinkPlacement to make the link object compatibale with other objects",              \
     ##__VA_ARGS__)

#define LINK_PARAM_OBJECT(...)                                                                     \
    (LinkedObject, App::DocumentObject*, App::PropertyLink, 0, "Linked object", ##__VA_ARGS__)

#define LINK_PARAM_TRANSFORM(...)                                                                  \
    (LinkTransform,                                                                                \
     bool,                                                                                         \
     App::PropertyBool,                                                                            \
     false,                                                                                        \
     "Set to false to override linked object's placement",                                         \
     ##__VA_ARGS__)

#define LINK_PARAM_CLAIM_CHILD(...)                                                                \
    (LinkClaimChild,                                                                               \
     bool,                                                                                         \
     App::PropertyBool,                                                                            \
     false,                                                                                        \
     "Claim the linked object as a child",                                                         \
     ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE(...)                                                             \
    (LinkCopyOnChange,                                                                             \
     long,                                                                                         \
     App::PropertyEnumeration,                                                                     \
     ((long)0),                                                                                    \
     "Disabled: turns off copy-on-change\n"                                                        \
     "Enabled: copies the linked object when any property marked CopyOnChange changes\n"           \
     "Owned: the linked object has been copied and is owned by the link; changes to the original\n"\
     "       will be synced back to the copy\n"                                                    \
     "Tracking: copies the linked object when any property marked CopyOnChange changes, and keeps\n"\
     "       future edits to the link in sync with the original",                                  \
     ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_SOURCE(...)                                                      \
    (LinkCopyOnChangeSource,                                                                       \
     App::DocumentObject*,                                                                         \
     App::PropertyLink,                                                                            \
     0,                                                                                            \
     "The copy-on-change source object",                                                           \
     ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_GROUP(...)                                                       \
    (LinkCopyOnChangeGroup,                                                                        \
     App::DocumentObject*,                                                                         \
     App::PropertyLink,                                                                            \
     0,                                                                                            \
     "Linked to an internal group object for holding on change copies",                             \
     ##__VA_ARGS__)

#define LINK_PARAM_COPY_ON_CHANGE_TOUCHED(...)                                                     \
    (LinkCopyOnChangeTouched,                                                                      \
     bool,                                                                                         \
     App::PropertyBool,                                                                            \
     0,                                                                                            \
     "Indicating the copy on change source object has been changed",                               \
     ##__VA_ARGS__)

#define LINK_PARAM_SCALE(...)                                                                      \
    (Scale, double, App::PropertyFloat, 1.0, "Scale factor", ##__VA_ARGS__)

#define LINK_PARAM_SCALE_VECTOR(...)                                                               \
    (ScaleVector,                                                                                  \
     Base::Vector3d,                                                                               \
     App::PropertyVector,                                                                          \
     Base::Vector3d(1, 1, 1),                                                                      \
     "Scale factors",                                                                              \
     ##__VA_ARGS__)

#define LINK_PARAM_PLACEMENTS(...)                                                                 \
    (PlacementList,                                                                                \
     std::vector<Base::Placement>,                                                                 \
     App::PropertyPlacementList,                                                                   \
     std::vector<Base::Placement>(),                                                               \
     "The placement for each link element",                                                        \
     ##__VA_ARGS__)

#define LINK_PARAM_SCALES(...)                                                                     \
    (ScaleList,                                                                                    \
     std::vector<Base::Vector3d>,                                                                  \
     App::PropertyVectorList,                                                                      \
     std::vector<Base::Vector3d>(),                                                                \
     "The scale factors for each link element",                                                    \
     ##__VA_ARGS__)

#define LINK_PARAM_VISIBILITIES(...)                                                               \
    (VisibilityList,                                                                               \
     boost::dynamic_bitset<>,                                                                      \
     App::PropertyBoolList,                                                                        \
     boost::dynamic_bitset<>(),                                                                    \
     "The visibility state of each link element",                                                  \
     ##__VA_ARGS__)

#define LINK_PARAM_COUNT(...)                                                                      \
    (ElementCount, int, App::PropertyInteger, 0, "Link element count", ##__VA_ARGS__)

#define LINK_PARAM_ELEMENTS(...)                                                                   \
    (ElementList,                                                                                  \
     std::vector<App::DocumentObject*>,                                                            \
     App::PropertyLinkList,                                                                        \
     std::vector<App::DocumentObject*>(),                                                          \
     "The link element object list",                                                               \
     ##__VA_ARGS__)

#define LINK_PARAM_SHOW_ELEMENT(...)                                                               \
    (ShowElement, bool, App::PropertyBool, true, "Enable link element list", ##__VA_ARGS__)

#define LINK_PARAM_MODE(...)                                                                       \
    (LinkMode, long, App::PropertyEnumeration, ((long)0), "Link group mode", ##__VA_ARGS__)

#define LINK_PARAM_LINK_EXECUTE(...)                                                               \
    (LinkExecute,                                                                                  \
     const char*,                                                                                  \
     App::PropertyString,                                                                          \
     (""),                                                                                         \
     "Link execute function. Default to 'appLinkExecute'. 'None' to disable.",                     \
     ##__VA_ARGS__)

#define LINK_PARAM_COLORED_ELEMENTS(...)                                                           \
    (ColoredElements,                                                                              \
     App::DocumentObject*,                                                                         \
     App::PropertyLinkSubHidden,                                                                   \
     0,                                                                                            \
     "Link colored elements",                                                                      \
     ##__VA_ARGS__)

#define LINK_PARAM(_param) (LINK_PARAM_##_param())

#define LINK_PNAME(_param) BOOST_PP_TUPLE_ELEM(0, _param)
#define LINK_PTYPE(_param) BOOST_PP_TUPLE_ELEM(1, _param)
#define LINK_PPTYPE(_param) BOOST_PP_TUPLE_ELEM(2, _param)
#define LINK_PDEF(_param) BOOST_PP_TUPLE_ELEM(3, _param)
#define LINK_PDOC(_param) BOOST_PP_TUPLE_ELEM(4, _param)

#define LINK_PINDEX(_param) BOOST_PP_CAT(Prop, LINK_PNAME(_param))
    /// @}

#define LINK_PARAMS                                                                                \
    LINK_PARAM(PLACEMENT)                                                                          \
    LINK_PARAM(LINK_PLACEMENT)                                                                     \
    LINK_PARAM(OBJECT)                                                                             \
    LINK_PARAM(CLAIM_CHILD)                                                                        \
    LINK_PARAM(TRANSFORM)                                                                          \
    LINK_PARAM(SCALE)                                                                              \
    LINK_PARAM(SCALE_VECTOR)                                                                       \
    LINK_PARAM(PLACEMENTS)                                                                         \
    LINK_PARAM(SCALES)                                                                             \
    LINK_PARAM(VISIBILITIES)                                                                       \
    LINK_PARAM(COUNT)                                                                              \
    LINK_PARAM(ELEMENTS)                                                                           \
    LINK_PARAM(SHOW_ELEMENT)                                                                       \
    LINK_PARAM(MODE)                                                                               \
    LINK_PARAM(LINK_EXECUTE)                                                                       \
    LINK_PARAM(COLORED_ELEMENTS)                                                                   \
    LINK_PARAM(COPY_ON_CHANGE)                                                                     \
    LINK_PARAM(COPY_ON_CHANGE_SOURCE)                                                              \
    LINK_PARAM(COPY_ON_CHANGE_GROUP)                                                               \
    LINK_PARAM(COPY_ON_CHANGE_TOUCHED)

    /// The property indices.
    enum PropIndex
    {
#define LINK_PINDEX_DEFINE(_1, _2, _param) LINK_PINDEX(_param),

        // defines Prop##Name enumeration value
        BOOST_PP_SEQ_FOR_EACH(LINK_PINDEX_DEFINE, _, LINK_PARAMS) PropMax
    };

    /**
     * @brief Set a property to a given index..
     *
     * @param[in] idx The property index obtained from the PropIndex enum.
     * @param[in] prop The property to set.
     */
    virtual void setProperty(int idx, Property* prop);

    /**
     * @brief Get a property by its index.
     *
     * @param[in] idx The property index obtained from the PropIndex enum.
     *
     * @return The property at the given index, or nullptr if the index is out
     * of range or the property is not set.
     */
    Property* getProperty(int idx);

    /**
     *
     * @brief Get a property by its name.
     *
     * @param[in] name The name of the property to get.
     * @return The property with the given name, or nullptr if not found.
     */
    Property* getProperty(const char*);

    /// Information about a link property.
    struct PropInfo
    {
        int index; ///< The property index obtained from the PropIndex enum.
        const char* name; ///< The name of the property.
        Base::Type type; ///< The type of the property.
        const char* doc; ///< The documentation string of the property.

        /**
         * @brief Construct a property info.
         *
         * @param[in] index The property index obtained from the PropIndex enum.
         * @param[in] name The name of the property.
         * @param[in] type The type of the property.
         * @param[in] doc The documentation string of the property.
         */
        PropInfo(int index, const char* name, Base::Type type, const char* doc)
            : index(index)
            , name(name)
            , type(type)
            , doc(doc)
        {}

        PropInfo()
            : index(0)
            , name(nullptr)
            , doc(nullptr)
        {}
    };

#define LINK_PROP_INFO(_1, _var, _param)                                                           \
    _var.push_back(PropInfo(BOOST_PP_CAT(Prop, LINK_PNAME(_param)),                                \
                            BOOST_PP_STRINGIZE(LINK_PNAME(_param)),                                \
                                               LINK_PPTYPE(_param)::getClassTypeId(),              \
                                               LINK_PDOC(_param)));

    /// Get the property info of this link.
    virtual const std::vector<PropInfo>& getPropertyInfo() const;

    /// A mapping from property name to its info.
    using PropInfoMap = std::map<std::string, PropInfo>;

    /// Get the property info map of this link.
    virtual const PropInfoMap& getPropertyInfoMap() const;

    /// The types for copy on change links.
    enum LinkCopyOnChangeType
    {
        CopyOnChangeDisabled = 0, ///< No copy on change behavior.
        CopyOnChangeEnabled = 1, ///< Copy on change is enabled but not necessarily in effect.
        CopyOnChangeOwned = 2, ///< Copy on change is enabled and in effect.
        CopyOnChangeTracking = 3 ///< Tracking copy-on-change behavior.
    };

#define LINK_PROP_GET(_1, _2, _param)                                                              \
    LINK_PTYPE(_param) BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Value))() const                  \
    {                                                                                              \
        auto prop = props[LINK_PINDEX(_param)];                                                    \
        if (!prop)                                                                                 \
            return LINK_PDEF(_param);                                                              \
        return static_cast<const LINK_PPTYPE(_param)*>(prop)->getValue();                          \
    }                                                                                              \
    const LINK_PPTYPE(_param) * BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property))() const      \
    {                                                                                              \
        auto prop = props[LINK_PINDEX(_param)];                                                    \
        return static_cast<const LINK_PPTYPE(_param)*>(prop);                                      \
    }                                                                                              \
    LINK_PPTYPE(_param) * BOOST_PP_SEQ_CAT((get)(LINK_PNAME(_param))(Property))()                  \
    {                                                                                              \
        auto prop = props[LINK_PINDEX(_param)];                                                    \
        return static_cast<LINK_PPTYPE(_param)*>(prop);                                            \
    }

    // defines get##Name##Property() and get##Name##Value() accessor
    BOOST_PP_SEQ_FOR_EACH(LINK_PROP_GET, _, LINK_PARAMS)

    /// Get the element list of this link.
    PropertyLinkList* _getElementListProperty() const;

    /// Get the element value list of this link.
    const std::vector<App::DocumentObject*>& _getElementListValue() const;

    /// Get the show element property.
    PropertyBool* _getShowElementProperty() const;

    /// Get the show element value.
    bool _getShowElementValue() const;

    /// Get the element count property.
    PropertyInteger* _getElementCountProperty() const;

    /// Get the element count value.
    int _getElementCountValue() const;

    /**
     * @brief Get the linked children of this link.
     *
     * @param[in] filter If true, it will filter out objects that are a group.
     *
     * @return A vector of linked children.
     */
    std::vector<DocumentObject*> getLinkedChildren(bool filter = true) const;

    /**
     * @brief Get a flattened subname.
     *
     * Get a flattened subname in case it references an object inside a linked
     * plain group.
     *
     * @param[in] subname The subname to flatten.
     * @return Returns the flattened subname.
     */
    const char* flattenSubname(const char* subname) const;

    /**
     * @brief Expand the subname.
     *
     * Expand the subname in case it references an object inside a linked plain
     * group.
     *
     * @param[in,out] subname The subname to expand. It will be modified in place.
     */
    void expandSubname(std::string& subname) const;

    /**
     * @brief Get the object the link points to.
     *
     * This method returns the linked object of this link. The @p depth
     * parameter is used as an indication of how deep the recursion is as a
     * safeguard against infinite recursion caused by cyclic dependencies.
     *
     * @param[in] depth The level on which we are resolving the link.
     *
     * @return Returns the linked object or @c nullptr if there is no linked value.
     */
    DocumentObject* getLink(int depth = 0) const;

    /**
     * @brief Get the transformation matrix of the link.
     *
     * @param[in] transform If true, it will take into account the placement of
     * the link or the original object, if false, it will only provide the
     * scaling.
     *
     * @return The transformation matrix of the link.
     */
    Base::Matrix4D getTransform(bool transform) const;

    /// Get the scale vector of the link.
    Base::Vector3d getScaleVector() const;

    /// Get the linked plain group if the linked object is a plain group.
    App::GroupExtension* linkedPlainGroup() const;

    /// Whether to transform the link together with the linked object.
    bool linkTransform() const;

    /// Get the subname of the link.
    const char* getSubName() const
    {
        parseSubName();
        return !mySubName.empty() ? mySubName.c_str() : nullptr;
    }

    /// Get the sub-elements of the link.
    const std::vector<std::string>& getSubElements() const
    {
        parseSubName();
        return mySubElements;
    }

    bool extensionGetSubObject(DocumentObject*& ret,
                               const char* subname,
                               PyObject** pyObj = nullptr,
                               Base::Matrix4D* mat = nullptr,
                               bool transform = false,
                               int depth = 0) const override;

    bool extensionGetSubObjects(std::vector<std::string>& ret, int reason) const override;

    bool extensionGetLinkedObject(DocumentObject*& ret,
                                  bool recurse,
                                  Base::Matrix4D* mat,
                                  bool transform,
                                  int depth) const override;

    App::DocumentObjectExecReturn* extensionExecute() override;
    short extensionMustExecute() override;
    void extensionOnChanged(const Property* p) override;
    void onExtendedUnsetupObject() override;
    void onExtendedDocumentRestored() override;

    int extensionSetElementVisible(const char*, bool) override;
    int extensionIsElementVisible(const char*) override;
    bool extensionHasChildElement() const override;

    PyObject* getExtensionPyObject() override;

    Property* extensionGetPropertyByName(const char* name) const override;

    /**
     * @brief Get the array index from a subname.
     *
     * @param[in] subname The subname to get the array index from.
     * @param[in,out] psubname If not null, it will point to the position
     * in the subname after the array index.
     *
     * @return The array index, or -1 if there is no array index.
     */
    static int getArrayIndex(const char* subname, const char** psubname = nullptr);

    /**
     * @brief Get the element index from a subname.
     *
     * This method will acquire the element index from a subname using various
     * strategies.
     *
     * @param[in] subname The subname to get the element index from.
     * @param[in,out] psubname If not null, it will point to the position
     * in the subname after the element index.
     *
     * @return The element index, or -1 if there is no element index.
     */
    int getElementIndex(const char* subname, const char** psubname = nullptr) const;

    /**
     *
     * @brief Get the element name from an index.
     *
     * This method will return the element name corresponding to the given
     * index.
     *
     * @param[in] idx The index of the element.
     * @param[out] ss The output stream to write the element name to.
     */
    void elementNameFromIndex(int idx, std::ostream& ss) const;

    /// Get the container object of this link.
    DocumentObject* getContainer();
    /// Get the container object of this link (const version).
    const DocumentObject* getContainer() const;

    /**
     * @brief Set the linked object.
     *
     * @param[in] index The index of the link property to set.
     * @param[in] obj The object to link to.
     * @param[in] subname The subname to link to.
     * @param[in] subs The sub-elements to link to.
     */
    void setLink(int index,
                 DocumentObject* obj,
                 const char* subname = nullptr,
                 const std::vector<std::string>& subs = std::vector<std::string>());

    /**
     * @brief Get the true linked object.
     *
     * This method returns the true linked object, which is the object that the
     * link points to.  The @p depth parameter is used as an indication of the
     * current level of recursion is as a safeguard against infinite recursion
     * caused by cyclic dependencies.
     *
     * @param recurse If true, it will recursively resolve the link until it reaches
     * the final linked object, or until it reaches the maximum recursion depth.
     * @param mat If non-null, it is used as the current transformation matrix on
     * input. On output it is used as the accumulated transformation up until
     * the final linked object.
     * @param depth This parameter indicates the level on which we are
     * resolving the link.
     * @param noElement If true, it will not return the linked object if it is
     * a link to an element.
     * @return Returns the true linked object. If the linked object is not found
     * or is invalid, it returns @c nullptr.
     *
     * @sa DocumentObject::getLinkedObject() which may return itself if it is not a link.
     *
     */
    DocumentObject* getTrueLinkedObject(bool recurse,
                                        Base::Matrix4D* mat = nullptr,
                                        int depth = 0,
                                        bool noElement = false) const;

    /// Check whether the link has a placement property.
    bool hasPlacement() const
    {
        return getLinkPlacementProperty() || getPlacementProperty();
    }

    /**
     * @brief Enable the cache for child labels.
     *
     * @param[in] enable If -1, it will enable the cache and only clear it. If
     * 0, it will clear the cache and disableit. If 1, it will enable the cache and fill it
     * with the current child elements.
     */
    void cacheChildLabel(int enable = -1) const;

    /**
     * @brief Setup copy on change behavior.
     *
     * This static method sets up the copy on change behavior for a given object.
     *
     * @param[in] obj The object to set up copy on change for.
     * @param[in] linked The linked object to copy from.
     * @param[in,out] copyOnChangeConns The vector to store the connections for copy on change.
     * @param[in] checkExisting If true, it will check the links properties
     * against the properties of the linked object.
     *
     * @return True if the setup was successful, false otherwise.
     */
    static bool
    setupCopyOnChange(App::DocumentObject* obj,
                      App::DocumentObject* linked,
                      std::vector<fastsignals::scoped_connection>* copyOnChangeConns,
                      bool checkExisting);

    /**
     * @brief Check if a property is marked as copy on change.
     *
     * @param[in] obj The object to check the property on.
     * @param[in] prop The property to check.
     * @return True if the property is marked as copy on change, false otherwise.
     */
    static bool isCopyOnChangeProperty(App::DocumentObject* obj, const Property& prop);

    /**
     * @brief Synchronize the copy on change object with the source object.
     *
     * This means updating the mutated copy in the copy-on-change group.
     */
    void syncCopyOnChange();

    /**
     * @brief Options used in setOnChangeCopyObject()
     *
     * Multiple options can be combined by bitwise or operator.
     */
    enum class OnChangeCopyOptions
    {
        None = 0, ///< No options set
        Exclude = 1, ///< Exclude an object to be copied when the configuration changes.
        ApplyAll = 2, ///< Apply the configuration to all links.
    };

    /**
     * @brief Include or exclude an object from the list of objects to copy on change.
     *
     * @param[in] obj: The object to include or exclude.
     * @param[in] options: control options of type OnChangeCopyOptions.
     */
    void setOnChangeCopyObject(App::DocumentObject* obj, OnChangeCopyOptions options);

    /**
     * @brief Get the list of objects that are set to be copied on change.
     *
     * @param[out] excludes: If not null, it will contain the objects that are
     * excluded from copy-on-change.
     * @param[in] src: The source of the copy on change link.  If `nullptr`, it
     * will use the `LinkCopyOnChangeSource` property to determine the source
     * or if not a copy-on-change link, the linked object.
     *
     * @return Objects that depend on the source of the copy-on-change link.
     */
    std::vector<App::DocumentObject*>
    getOnChangeCopyObjects(std::vector<App::DocumentObject*>* excludes = nullptr,
                           App::DocumentObject* src = nullptr);

    /**
     * @brief Check whether this link is configurable one.
     *
     * This essentially means that the linked object has copy-on-change properties.
     *
     * @return True if the link is configurable, false otherwise.
     */
    bool isLinkedToConfigurableObject() const;

    /**
     * @brief Monitor changes on the list of copy-on-change objects.
     *
     * This function has as input the list of dependencies of original
     * dependencies of the copy-on-change link.  It sets up connections to
     * monitor these original objects, to update the copy-on-change links.
     *
     * @param[in] objs The list of objects to monitor.
     */
    void monitorOnChangeCopyObjects(const std::vector<App::DocumentObject*>& objs);

    /// Check if the linked object is a copy on change
    bool isLinkMutated() const;

protected:
    void
    _handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName, const char* PropName);

    /// Parse the subname into mySubName and mySubElements
    void parseSubName() const;

    /**
     * @brief Update the link when a property changes.
     *
     * It fullfills a role similar to DocumentObject::onChanged().
     *
     * @param[in] parent The parent document object.
     * @param[in] prop The property that changed.
     */
    void update(App::DocumentObject* parent, const Property* prop);

    /**
     * @brief Check a property in case of copy-on-change.
     *
     * If a copy is a copy-on-change property in the parent, copy the property
     * from the source to the link (in the copy-on-change group).
     *
     * @param[in] parent The parent document object.
     * @param[in] prop The property to check.
     */
    void checkCopyOnChange(App::DocumentObject* parent, const App::Property& prop);

    /**
     * @brief Setup copy-on-change behavior for this link.
     *
     * Transform a regular link into a copy-on-change link.  This means that
     * the linked object is copied in the copy-on-change group and that the
     * linked object will point to this copy, while the copy-on-change source
     * will point to the original.
     *
     * @param[in] parent The parent document object.
     * @param[in] checkSource If true, it will check the and set the
     * copy-on-change source property.
     */
    void setupCopyOnChange(App::DocumentObject* parent, bool checkSource = false);

    /**
     * @brief Make a copy-of-change link from this link.
     *
     * This function retrieves the dependencies from the linked object and
     * copies them.  It will put these copies into the copy-on-change group and
     * the linked object will be the root of these list of dependencies, making
     * it equivalent to the original linked object.
     *
     * @return The new copy-on-change link object.
     */
    App::DocumentObject* makeCopyOnChange();

    /// Sync the link elements in this link.
    void syncElementList();

    /**
     * @brief Detach a linked element.
     *
     * Depending on earlier set options, the object may be deleted.
     *
     * @param[in] obj The object to detach.
     */
    void detachElement(App::DocumentObject* obj);

    /// Detach all linked elements.
    void detachElements();

    /**
     * @brief Check the geo element map for a linked object.
     *
     * This method checks if subnames are in accordance with the geo element
     * map.
     *
     * @param[in] obj The document object containing the link.
     * @param[in] linked The linked document object.
     * @param[in] pyObj The Python object corresponding to the linked object.
     * @param[in] postfix The postfix that should be taken into account regarding subelements.
     */
    void checkGeoElementMap(const App::DocumentObject* obj,
                            const App::DocumentObject* linked,
                            PyObject** pyObj,
                            const char* postfix) const;

    /// Update the connections for a group of link elements.
    void updateGroup();

    /**
     * @brief Slot called when a plain group changes.
     *
     * @param[in] obj The document object that changed.
     * @param[in] prop The property that changed.
     */
    void slotChangedPlainGroup(const App::DocumentObject& obj, const App::Property& prop);

protected:
    /// The properties for the link.
    std::vector<Property*> props;

    /// A set of elements to hide.
    std::unordered_set<const App::DocumentObject*> myHiddenElements;

    /// Cached subelements.
    mutable std::vector<std::string> mySubElements;

    /// Cached subname.
    mutable std::string mySubName;

    /// Connections to monitor plain group changes.
    std::unordered_map<const App::DocumentObject*, fastsignals::scoped_connection>
        plainGroupConns;

    /// Cache for label based subname lookup.
    mutable std::unordered_map<std::string, int> myLabelCache;
    /// Whether the label cache is enabled.
    mutable bool enableLabelCache {false};

    /// Whether the link has old style subelement.
    bool hasOldSubElement {false};

    /// Connections for copy on change behavior.
    std::vector<fastsignals::scoped_connection> copyOnChangeConns;

    /// Connections for the source objects for copy on change.
    std::vector<fastsignals::scoped_connection> copyOnChangeSrcConns;

    /// Whether the link has copy on change behavior.
    bool hasCopyOnChange {true};

    /// Whether we are checking properties to avoid recursion.
    mutable bool checkingProperty = false;

    /// Whether to pause copy on change updates.
    bool pauseCopyOnChange = false;

    /// Connection for monitoring changes on the copy on change source.
    fastsignals::scoped_connection connCopyOnChangeSource;
};

///////////////////////////////////////////////////////////////////////////

using LinkBaseExtensionPython = ExtensionPythonT<LinkBaseExtension>;

///////////////////////////////////////////////////////////////////////////


/**
 * @brief The link extension class.
 * @ingroup LinksGroup
 *
 * This class implements the link extension functionality and is the extension
 * that makes @ref App::Link "Link" a link.
 */
class AppExport LinkExtension: public LinkBaseExtension
{
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::LinkExtension);
    using inherited = LinkBaseExtension;

public:
    LinkExtension();
    ~LinkExtension() override = default;

    /**
     * @name Helpers for defining extended properties.
     * @brief Macros that help define properties that extend the properties of the linked object.
     *
     * Extended property definition:
     * `(Name, Type, Property_Type, Default, %Document, Property_Name,
     *  Derived_Property_Type, App_Property_Type, Group)`
     *
     * This helper simply reuses `Name` as `Property_Name`, `Property_Type` as
     * `Derived_Property_type`, `Prop_None` as `App_Propert_Type`.
     *
     * @note
     * Because PropertyView will merge linked object's properties into
     * ours, we set the default group name as ' Link' with a leading space to
     * try to make our group before others
     * {@
     */

#define LINK_ENAME(_param) BOOST_PP_TUPLE_ELEM(5, _param)
#define LINK_ETYPE(_param) BOOST_PP_TUPLE_ELEM(6, _param)
#define LINK_EPTYPE(_param) BOOST_PP_TUPLE_ELEM(7, _param)
#define LINK_EGROUP(_param) BOOST_PP_TUPLE_ELEM(8, _param)

#define _LINK_PROP_ADD(_add_property, _param)                                                      \
    _add_property(BOOST_PP_STRINGIZE(LINK_ENAME(_param)),                                          \
                                     LINK_ENAME(_param),                                           \
                                     (LINK_PDEF(_param)),                                          \
                                     LINK_EGROUP(_param),                                          \
                                     LINK_EPTYPE(_param),                                          \
                                     LINK_PDOC(_param));                                           \
    setProperty(LINK_PINDEX(_param), &LINK_ENAME(_param));

#define LINK_PROP_ADD(_1, _2, _param) _LINK_PROP_ADD(_ADD_PROPERTY_TYPE, _param);

#define LINK_PROP_ADD_EXTENSION(_1, _2, _param)                                                    \
    _LINK_PROP_ADD(_EXTENSION_ADD_PROPERTY_TYPE, _param);

#define LINK_PROPS_ADD(_seq) BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD, _, _seq)

#define LINK_PROPS_ADD_EXTENSION(_seq) BOOST_PP_SEQ_FOR_EACH(LINK_PROP_ADD_EXTENSION, _, _seq)

#define _LINK_PROP_SET(_1, _2, _param) setProperty(LINK_PINDEX(_param), &LINK_ENAME(_param));

#define LINK_PROPS_SET(_seq) BOOST_PP_SEQ_FOR_EACH(_LINK_PROP_SET, _, _seq)

    /// Helper for defining default extended parameter
#define _LINK_PARAM_EXT(_name, _type, _ptype, _def, _doc, ...)                                     \
    ((_name, _type, _ptype, _def, _doc, _name, _ptype, App::Prop_None, " Link"))

    /** Define default extended parameter
     * It simply reuses Name as Property_Name, Property_Type as
     * Derived_Property_Type, and App::Prop_None as App::PropertyType
     */
#define LINK_PARAM_EXT(_param) BOOST_PP_EXPAND(_LINK_PARAM_EXT LINK_PARAM_##_param())

    /// Helper for extended parameter with app property type
#define _LINK_PARAM_EXT_ATYPE(_name, _type, _ptype, _def, _doc, _atype)                            \
    ((_name, _type, _ptype, _def, _doc, _name, _ptype, _atype, " Link"))

    /// Define extended parameter with app property type
#define LINK_PARAM_EXT_ATYPE(_param, _atype)                                                       \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_ATYPE LINK_PARAM_##_param(_atype))

    /// Helper for extended parameter with derived property type
#define _LINK_PARAM_EXT_TYPE(_name, _type, _ptype, _def, _doc, _dtype)                             \
    ((_name, _type, _ptype, _def, _doc, _name, _dtype, App::Prop_None, " Link"))

    /// Define extended parameter with derived property type
#define LINK_PARAM_EXT_TYPE(_param, _dtype)                                                        \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_TYPE LINK_PARAM_##_param(_dtype))

    /// Helper for extended parameter with a different property name
#define _LINK_PARAM_EXT_NAME(_name, _type, _ptype, _def, _doc, _pname)                             \
    ((_name, _type, _ptype, _def, _doc, _pname, _ptype, App::Prop_None, " Link"))

    /// Define extended parameter with a different property name
#define LINK_PARAM_EXT_NAME(_param, _pname)                                                        \
    BOOST_PP_EXPAND(_LINK_PARAM_EXT_NAME LINK_PARAM_##_param(_pname))
    //@}

#define LINK_PARAMS_EXT                                                                            \
    LINK_PARAM_EXT(SCALE)                                                                          \
    LINK_PARAM_EXT_ATYPE(SCALE_VECTOR, App::Prop_Hidden)                                           \
    LINK_PARAM_EXT(SCALES)                                                                         \
    LINK_PARAM_EXT(VISIBILITIES)                                                                   \
    LINK_PARAM_EXT(PLACEMENTS)                                                                     \
    LINK_PARAM_EXT(ELEMENTS)

#define LINK_PROP_DEFINE(_1, _2, _param) LINK_ETYPE(_param) LINK_ENAME(_param);
#define LINK_PROPS_DEFINE(_seq) BOOST_PP_SEQ_FOR_EACH(LINK_PROP_DEFINE, _, _seq)

    // defines the actual properties
    LINK_PROPS_DEFINE(LINK_PARAMS_EXT)

    void onExtendedDocumentRestored() override
    {
        LINK_PROPS_SET(LINK_PARAMS_EXT);
        inherited::onExtendedDocumentRestored();
    }
};

///////////////////////////////////////////////////////////////////////////

using LinkExtensionPython = ExtensionPythonT<LinkExtension>;

///////////////////////////////////////////////////////////////////////////


/**
 * @brief The %Link class.
 * @ingroup LinksGroup
 *
 * Instances of this class represent links to other objects in the document or
 * even to objects in other documents.
 */
class AppExport Link: public App::DocumentObject, public App::LinkExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::Link);
    using inherited = App::DocumentObject;

public:
#define LINK_PARAMS_LINK                                                                           \
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)                                                \
    LINK_PARAM_EXT(CLAIM_CHILD)                                                                    \
    LINK_PARAM_EXT(TRANSFORM)                                                                      \
    LINK_PARAM_EXT(LINK_PLACEMENT)                                                                 \
    LINK_PARAM_EXT(PLACEMENT)                                                                      \
    LINK_PARAM_EXT(SHOW_ELEMENT)                                                                   \
    LINK_PARAM_EXT_TYPE(COUNT, App::PropertyIntegerConstraint)                                     \
    LINK_PARAM_EXT(LINK_EXECUTE)                                                                   \
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS, App::Prop_Hidden)                                       \
    LINK_PARAM_EXT(COPY_ON_CHANGE)                                                                 \
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)                                 \
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)                                                           \
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)

    LINK_PROPS_DEFINE(LINK_PARAMS_LINK)

    Link();

    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override
    {
        LINK_PROPS_SET(LINK_PARAMS_LINK);
        inherited::onDocumentRestored();
    }

    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* TypeName,
                                   const char* PropName) override
    {
        _handleChangedPropertyName(reader, TypeName, PropName);
    }

    bool canLinkProperties() const override;

    Base::Placement getPlacementOf(const std::string& sub, DocumentObject* targetObj = nullptr) override;

    bool isLink() const override;

    bool isLinkGroup() const override;
};

using LinkPython = App::FeaturePythonT<Link>;

///////////////////////////////////////////////////////////////////////////


/**
 * @brief A class that represents an element of a link.
 * @ingroup LinksGroup
 *
 * A link with an element count greater than 0 will contain multiple links to
 * the linked object, all with their own placement, scale, and visibility.
 * These links are instances of this class.  The link itself becomes a special
 * link pointing to the same linked object, but its element list will contain
 * references to the element links.
 */
class AppExport LinkElement: public App::DocumentObject, public App::LinkBaseExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkElement);
    using inherited = App::DocumentObject;

public:
#define LINK_PARAMS_ELEMENT                                                                        \
    LINK_PARAM_EXT(SCALE)                                                                          \
    LINK_PARAM_EXT_ATYPE(SCALE_VECTOR, App::Prop_Hidden)                                           \
    LINK_PARAM_EXT_TYPE(OBJECT, App::PropertyXLink)                                                \
    LINK_PARAM_EXT(TRANSFORM)                                                                      \
    LINK_PARAM_EXT(LINK_PLACEMENT)                                                                 \
    LINK_PARAM_EXT(PLACEMENT)                                                                      \
    LINK_PARAM_EXT(COPY_ON_CHANGE)                                                                 \
    LINK_PARAM_EXT_TYPE(COPY_ON_CHANGE_SOURCE, App::PropertyXLink)                                 \
    LINK_PARAM_EXT(COPY_ON_CHANGE_GROUP)                                                           \
    LINK_PARAM_EXT(COPY_ON_CHANGE_TOUCHED)


    /// Define the various properties for a link element.
    LINK_PROPS_DEFINE(LINK_PARAMS_ELEMENT)

    LinkElement();

    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override
    {
        LINK_PROPS_SET(LINK_PARAMS_ELEMENT);
        inherited::onDocumentRestored();
    }

    /// Check whether this link element can be deleted.
    bool canDelete() const;

    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* TypeName,
                                   const char* PropName) override
    {
        _handleChangedPropertyName(reader, TypeName, PropName);
    }

    bool isLink() const override;

    /// Get the parent link of this link element.
    App::Link* getLinkGroup() const;

    Base::Placement getPlacementOf(const std::string& sub, DocumentObject* targetObj = nullptr) override;
};

using LinkElementPython = App::FeaturePythonT<LinkElement>;

///////////////////////////////////////////////////////////////////////////


/**
 * @brief A class that represents a group of links.
 * @ingroup LinksGroup
 *
 * Other than "upgrading" a normal Link to having multiple @ref
 * App::LinkElement "LinkElements", a link group is a grouping for document
 * objects where the group itself has a separate placement or visibility of the
 * elements.
 *
 * A link group can contain the objects directly as children (called a simple
 * group), or it can contain links to the objects.  In the latter case, it is
 * possible to create a group with transform links which means that the
 * placement of the original objects affect the placement of the links as well.
 */
class AppExport LinkGroup: public App::DocumentObject, public App::LinkBaseExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(App::LinkGroup);
    using inherited = App::DocumentObject;

public:
#define LINK_PARAMS_GROUP                                                                          \
    LINK_PARAM_EXT(ELEMENTS)                                                                       \
    LINK_PARAM_EXT(PLACEMENT)                                                                      \
    LINK_PARAM_EXT(VISIBILITIES)                                                                   \
    LINK_PARAM_EXT(MODE)                                                                           \
    LINK_PARAM_EXT_ATYPE(COLORED_ELEMENTS, App::Prop_Hidden)

    /// Define the various properties of the link group.
    LINK_PROPS_DEFINE(LINK_PARAMS_GROUP)

    LinkGroup();

    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderLink";
    }

    void onDocumentRestored() override
    {
        LINK_PROPS_SET(LINK_PARAMS_GROUP);
        inherited::onDocumentRestored();
    }
};

using LinkGroupPython = App::FeaturePythonT<LinkGroup>;

}  // namespace App

ENABLE_BITMASK_OPERATORS(App::Link::OnChangeCopyOptions)

/*[[[cog
import LinkParams
LinkParams.declare()
]]]*/

// Auto generated code (App/params_utils.py:90)
namespace App {
/**
 * @brief Convenient class to obtain App::Link related parameters
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Link"
 *
 * This class is auto generated by App/LinkParams.py. Modify that file
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
 * You can add a new parameter by adding lines in App/LinkParams.py. Available
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

    // Auto generated code (App/params_utils.py:139)
    /// @name CopyOnChangeApplyToAll accessors
    /// @brief Accessors for parameter CopyOnChangeApplyToAll
    ///
    /// Stores the last user choice of whether to apply CopyOnChange setup to all link
    /// that links to the same configurable object
    /// @{
    static const bool & getCopyOnChangeApplyToAll();
    static const bool & defaultCopyOnChangeApplyToAll();
    static void removeCopyOnChangeApplyToAll();
    static void setCopyOnChangeApplyToAll(const bool &v);
    static const char *docCopyOnChangeApplyToAll();
    /// @}

// Auto generated code (App/params_utils.py:180)
}; // class LinkParams
} // namespace App
//[[[end]]]


#if defined(__clang__)
#pragma clang diagnostic pop
#endif
