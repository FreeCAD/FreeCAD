// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD Project Association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include <Base/Color.h>

#include "Material.h"
#include "PropertyLinks.h"

namespace App
{

class PropertyLinkSubColorMap;
class PropertyLinkSubMaterialMap;
struct PropertyLinkSubColorMapTraits;
struct PropertyLinkSubMaterialMapTraits;

/**
 * @brief Base class for topology-aware subelement value maps.
 *
 * The class stores the document-object/subelement-reference side of the map and
 * implements the `PropertyLinkBase` integration shared by all value types:
 * dependency scope, label reference registration, topology element remapping,
 * link replacement, and backlink maintenance. Concrete subclasses add typed
 * value storage and persistence.
 *
 * References are intentionally multi-object capable. A map may contain
 * `(object, subelement)` entries for one object, for several objects, or for a
 * higher-level linked subname path.
 */
class AppExport PropertyLinkSubValueMapBase: public PropertyLinkBase
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * @brief Stored reference for one map entry.
     */
    using Reference = PropertyLinkSubReferenceStore::Reference;

    PropertyLinkSubValueMapBase();
    ~PropertyLinkSubValueMapBase() override;

    void afterRestore() override;
    void onContainerRestored() override;

    /**
     * @brief Return the number of map entries.
     */
    int getSize() const;

    /**
     * @brief Return the linked document objects in entry order.
     */
    std::vector<App::DocumentObject*> getObjects() const;

    /**
     * @brief Return the stored subelement names in entry order.
     */
    std::vector<std::string> getSubValues() const;

    /**
     * @brief Return subelement names using old or new style topology names.
     *
     * @param newStyle If true, prefer mapped/new style names. If false, prefer
     * old style names.
     */
    std::vector<std::string> getSubValues(bool newStyle) const;

    /**
     * @brief Return the topology shadow references in entry order.
     */
    std::vector<ShadowSub> getShadowSubs() const;

    /**
     * @brief Return all references in entry order.
     *
     * @param newStyle If true, each returned subelement name prefers the mapped
     * topology name.
     */
    std::vector<Reference> getReferences(bool newStyle = true) const;

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

protected:
    /**
     * @brief Return the document object that owns this property's link state.
     */
    App::DocumentObject* getLinkOwner() const;

    /**
     * @brief Return the document used to resolve object names during restore.
     */
    App::Document* getOwnerDocument() const;

    /**
     * @brief Check whether a candidate linked object is valid for this map.
     */
    void verifyObject(App::DocumentObject* obj, App::DocumentObject* owner) const;

    /**
     * @brief Maintain backlinks from linked objects to the owning document object.
     */
    void maintainBackLinks(const std::vector<App::DocumentObject*>& objects);

    /**
     * @brief Replace the stored references after typed values have been updated.
     */
    void setReferences(std::vector<App::DocumentObject*>&& objects,
                       std::vector<std::string>&& subNames,
                       std::vector<ShadowSub>&& shadowSubs = {});

    /**
     * @brief Write the reference XML elements common to all typed maps.
     */
    void saveReferences(Base::Writer& writer, const char* elementName) const;

    /**
     * @brief Restore the reference XML elements common to all typed maps.
     */
    void restoreReferences(Base::XMLReader& reader,
                           const char* elementName,
                           int count,
                           std::vector<std::size_t>& restoredValueIndices);

    /**
     * @brief Return indices that will be written to a document side file.
     */
    std::vector<std::size_t> getPersistedEntryIndices() const;

    /**
     * @brief Stored link references in value order.
     *
     * The shared store keeps linked objects, subelement names, and topology
     * shadows together so reference updates cannot desynchronize parallel
     * containers.
     */
    PropertyLinkSubReferenceStore _references;
    std::vector<int> _mapped;
};

/**
 * @brief Typed topology-aware map from subelement references to values.
 *
 * @tparam ValueT Stored C++ value type.
 * @tparam TraitsT Value conversion and serialization policy.
 * @tparam DerivedT Registered concrete FreeCAD property type.
 */
template<class ValueT, class TraitsT, class DerivedT>
class AppExport PropertyLinkSubValueMapT: public PropertyLinkSubValueMapBase
{
public:
    using Value = ValueT;

    /**
     * @brief Full typed entry returned by `getEntries()`.
     */
    struct Entry
    {
        App::DocumentObject* object {nullptr}; /**< Linked document object. */
        std::string subName;                   /**< Subelement name. */
        Value value {};                        /**< Value assigned to the subelement. */
    };

    PropertyLinkSubValueMapT();
    ~PropertyLinkSubValueMapT() override;

    /**
     * @brief Clear all entries.
     */
    void clear();

    /**
     * @brief Clear all entries.
     *
     * This overload matches the default-initialization convention used by
     * FreeCAD's static property registration macros.
     */
    void setValue();

    /**
     * @brief Set one map entry, replacing any existing entry with the same key.
     */
    void setValue(App::DocumentObject* object, const char* subName, const Value& value);

    /**
     * @brief Replace all map entries.
     *
     * Duplicate `(object, subName)` keys are canonicalized with last-write-wins
     * semantics.
     */
    void setValues(std::vector<App::DocumentObject*> objects,
                   std::vector<std::string> subNames,
                   std::vector<Value> values,
                   std::vector<ShadowSub>&& shadowSubs = {});

    /**
     * @brief Return all stored values in entry order.
     */
    const std::vector<Value>& getValues() const
    {
        return _values;
    }

    /**
     * @brief Return the value for one `(object, subName)` key.
     *
     * @param object Linked document object.
     * @param subName Subelement name to query.
     * @param value Receives the stored value when the method returns true.
     * @param newStyle If true, compare against mapped/new style subelement
     * names. If false, compare against old style names.
     */
    bool getValue(App::DocumentObject* object,
                  const char* subName,
                  Value& value,
                  bool newStyle = true) const;

    /**
     * @brief Return all entries in entry order.
     */
    std::vector<Entry> getEntries(bool newStyle = true) const;

    /**
     * @brief Return entries belonging to one object keyed by subelement name.
     */
    std::map<std::string, Value>
    getValuesForObject(App::DocumentObject* object, bool newStyle = true) const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    Property*
    CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const override;

    Property* CopyOnLabelChange(App::DocumentObject* obj,
                                const std::string& ref,
                                const char* newLabel) const override;

    Property* CopyOnLinkReplace(const App::DocumentObject* parent,
                                App::DocumentObject* oldObj,
                                App::DocumentObject* newObj) const override;

    void breakLink(App::DocumentObject* obj, bool clear) override;
    bool adjustLink(const std::set<App::DocumentObject*>& inList) override;

    unsigned int getMemSize() const override;
    bool isSame(const Property& other) const override;

protected:
    void setEntries(std::vector<App::DocumentObject*> objects,
                    std::vector<std::string> subNames,
                    std::vector<Value> values,
                    std::vector<ShadowSub>&& shadowSubs,
                    bool canonicalize);

private:
    std::vector<Value> _values;
    std::vector<std::size_t> _restoreValueIndices;
    int _valueFormatVersion {0};
};

/**
 * @brief Topology-aware map from subelement references to colors.
 */
class AppExport PropertyLinkSubColorMap
    : public PropertyLinkSubValueMapT<Base::Color,
                                      PropertyLinkSubColorMapTraits,
                                      PropertyLinkSubColorMap>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
};

/**
 * @brief Hidden topology-aware map from subelement references to colors.
 */
class AppExport PropertyLinkSubColorMapHidden: public PropertyLinkSubColorMap
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubColorMapHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

/**
 * @brief Topology-aware map from subelement references to materials.
 */
class AppExport PropertyLinkSubMaterialMap
    : public PropertyLinkSubValueMapT<App::Material,
                                      PropertyLinkSubMaterialMapTraits,
                                      PropertyLinkSubMaterialMap>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
};

/**
 * @brief Hidden topology-aware map from subelement references to materials.
 */
class AppExport PropertyLinkSubMaterialMapHidden: public PropertyLinkSubMaterialMap
{
    TYPESYSTEM_HEADER();

public:
    PropertyLinkSubMaterialMapHidden()
    {
        _pcScope = LinkScope::Hidden;
    }
};

}  // namespace App
