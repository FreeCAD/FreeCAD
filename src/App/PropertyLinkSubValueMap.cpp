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

#include "PropertyLinkSubValueMap.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <utility>

#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "ComplexGeoData.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObserver.h"
#include "ElementNamingUtils.h"
#include "GeoFeature.h"
#include "IndexedName.h"
#include "MaterialPy.h"
#include "ObjectIdentifier.h"
#include "PropertySerialization.h"
#include "PropertyStandard.h"

using namespace App;

namespace
{

constexpr int MapFormatVersion = 1;
constexpr const char* LinkElementName = "Link";
constexpr const char* ShadowedAttribute = "shadowed";
constexpr const char* ShadowAttribute = "shadow";
constexpr const char* MappedAttribute = "mapped";
constexpr const char* ValueFileAttribute = "file";
constexpr const char* MapVersionAttribute = "version";
constexpr const char* ValueVersionAttribute = "valueVersion";

struct GeometryCacheElement
{
    std::string name;
    std::size_t replacementOffset {};
};

const std::string& activeReferenceSubName(
    const App::PropertyLinkSubReferenceStore::Reference& reference)
{
    if (!reference.shadow.newName.empty()) {
        return reference.shadow.newName;
    }
    if (!reference.shadow.oldName.empty()) {
        return reference.shadow.oldName;
    }
    return reference.subName;
}

// The Python facade accepts both full multi-object entries and the shorthand
// `(object, {subname: value, ...})`. Keep that parsing local to avoid making
// these property-specific assignment rules look like general App API.
App::DocumentObject* getDocumentObject(PyObject* value, const char* errorMessage)
{
    if (!PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        throw Base::TypeError(errorMessage);
    }
    return static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr();
}

std::string getSubName(PyObject* value)
{
    PropertyString stringProp;
    stringProp.setPyObject(value);
    return stringProp.getValue();
}

template<class TraitsT>
void appendPyEntry(std::vector<App::DocumentObject*>& objects,
                   std::vector<std::string>& subNames,
                   std::vector<typename TraitsT::Value>& values,
                   App::DocumentObject* object,
                   PyObject* pySubName,
                   PyObject* pyValue)
{
    objects.push_back(object);
    subNames.push_back(getSubName(pySubName));
    values.push_back(TraitsT::fromPyObject(pyValue));
}

template<class TraitsT>
void appendPyObjectEntries(std::vector<App::DocumentObject*>& objects,
                           std::vector<std::string>& subNames,
                           std::vector<typename TraitsT::Value>& values,
                           PyObject* pyEntries,
                           const char* errorMessage)
{
    Py::Sequence entries(pyEntries);
    for (Py::Sequence::size_type i = 0; i < entries.size(); ++i) {
        Py::Object entry = entries[i];
        if (!PyTuple_Check(entry.ptr()) && !PyList_Check(entry.ptr())) {
            throw Base::TypeError(errorMessage);
        }

        Py::Sequence entrySequence(entry);
        if (entrySequence.size() != 3) {
            throw Base::ValueError("map entries must contain object, subname, and value");
        }

        auto* object = getDocumentObject(entrySequence[0].ptr(), errorMessage);
        appendPyEntry<TraitsT>(
            objects, subNames, values, object, entrySequence[1].ptr(), entrySequence[2].ptr());
    }
}

template<class TraitsT>
void appendPySingleObjectEntries(std::vector<App::DocumentObject*>& objects,
                                 std::vector<std::string>& subNames,
                                 std::vector<typename TraitsT::Value>& values,
                                 App::DocumentObject* object,
                                 PyObject* pyMapping,
                                 const char* errorMessage)
{
    if (PyDict_Check(pyMapping)) {
        PyObject* key {};
        PyObject* item {};
        Py_ssize_t pos {};
        while (PyDict_Next(pyMapping, &pos, &key, &item)) {
            appendPyEntry<TraitsT>(objects, subNames, values, object, key, item);
        }
        return;
    }

    if (!PyTuple_Check(pyMapping) && !PyList_Check(pyMapping)) {
        throw Base::TypeError(errorMessage);
    }

    Py::Sequence entries(pyMapping);
    for (Py::Sequence::size_type i = 0; i < entries.size(); ++i) {
        Py::Object entry = entries[i];
        if (!PyTuple_Check(entry.ptr()) && !PyList_Check(entry.ptr())) {
            throw Base::TypeError(errorMessage);
        }

        Py::Sequence entrySequence(entry);
        if (entrySequence.size() != 2) {
            throw Base::ValueError("single-object map entries must contain subname and value");
        }
        appendPyEntry<TraitsT>(
            objects, subNames, values, object, entrySequence[0].ptr(), entrySequence[1].ptr());
    }
}

GeometryCacheElement indexedElementNameForGeometryCache(
    const App::PropertyLinkSubReferenceStore::Reference& reference)
{
    const std::string& subName = activeReferenceSubName(reference);
    const char* element = Data::findElementName(subName.c_str());
    if (!element || !element[0] || Data::hasMissingElement(element)) {
        return {};
    }

    const auto replacementOffset = static_cast<std::size_t>(element - subName.c_str());
    if (!Data::isMappedElement(element)) {
        return {element, replacementOffset};
    }

    const char* suffix = std::strrchr(element, '.');
    if (!suffix || !suffix[1]) {
        return {};
    }
    ++suffix;

    const Data::IndexedName index(suffix);
    if (!index) {
        return {};
    }

    return {suffix, replacementOffset};
}

std::string indexedFallbackSubName(
    const App::PropertyLinkSubReferenceStore::Reference& reference)
{
    const std::string& subName = activeReferenceSubName(reference);
    const char* element = Data::findElementName(subName.c_str());
    if (!element || !element[0] || Data::hasMissingElement(element)
        || !Data::isMappedElement(element)) {
        return {};
    }

    const char* suffix = std::strrchr(element, '.');
    if (!suffix || !suffix[1]) {
        return {};
    }
    ++suffix;

    const Data::IndexedName index(suffix);
    if (!index) {
        return {};
    }

    std::string result(subName, 0, static_cast<std::size_t>(element - subName.c_str()));
    result += suffix;
    return result;
}

std::string subNameFromCachedElement(
    const App::PropertyLinkSubReferenceStore::Reference& reference,
    const GeometryCacheElement& cachedElement,
    const std::string& replacementElement)
{
    const std::string& subName = activeReferenceSubName(reference);
    if (cachedElement.replacementOffset > subName.size()) {
        return {};
    }

    std::string result(subName, 0, cachedElement.replacementOffset);
    result += replacementElement;
    return result;
}

bool isMissingReference(const std::string& subName, const App::PropertyLinkBase::ShadowSub& shadow)
{
    return Data::hasMissingElement(subName.c_str())
        || Data::hasMissingElement(shadow.oldName.c_str())
        || Data::hasMissingElement(shadow.newName.c_str());
}

std::string findCachedSubName(App::DocumentObject* feature,
                              const App::PropertyLinkSubReferenceStore::Reference& reference)
{
    // An indexed subelement can still exist after recompute while naming different geometry.
    // Prefer the feature's pre-change geometry snapshot when this map references that feature
    // directly; fall back to the generic topo-name updater when the cache has no match.
    if (!feature || feature != reference.object) {
        return {};
    }

    auto* geo = freecad_cast<App::GeoFeature*>(feature);
    if (!geo) {
        return {};
    }

    const GeometryCacheElement cachedElement = indexedElementNameForGeometryCache(reference);
    if (cachedElement.name.empty()) {
        return {};
    }

    auto names = geo->searchElementCache(cachedElement.name, Data::SearchOption::CheckGeometry);
    if (names.empty()) {
        names = geo->searchElementCache(cachedElement.name, Data::SearchOptions());
    }
    if (names.empty()) {
        return {};
    }

    return subNameFromCachedElement(reference, cachedElement, names.front());
}

}  // namespace

namespace App
{

struct PropertyLinkSubColorMapTraits
{
    using Value = Base::Color;

    static const char* xmlName()
    {
        return "LinkSubColorMap";
    }

    static int valueFormatVersion()
    {
        return 1;
    }

    static Py::Tuple toPyObject(const Base::Color& value)
    {
        Py::Tuple rgba(4);
        rgba[0] = Py::Float(value.r);
        rgba[1] = Py::Float(value.g);
        rgba[2] = Py::Float(value.b);
        rgba[3] = Py::Float(value.a);
        return rgba;
    }

    static Base::Color fromPyObject(PyObject* value)
    {
        PropertyColor color;
        color.setPyObject(value);
        return color.getValue();
    }

    static void saveValues(Base::OutputStream& stream,
                           const std::vector<Base::Color>& values,
                           const std::vector<std::size_t>& indices)
    {
        stream << static_cast<uint32_t>(indices.size());
        for (auto index : indices) {
            stream << values[index].getPackedValue();
        }
    }

    static std::vector<Base::Color>
    restoreValues(Base::InputStream& stream, int restoredValueFormat)
    {
        if (restoredValueFormat != valueFormatVersion()) {
            throw Base::RuntimeError("Unsupported color map value format");
        }

        uint32_t count {};
        stream >> count;

        std::vector<Base::Color> values(count);
        uint32_t packed {};
        for (auto& value : values) {
            stream >> packed;
            value.setPackedValue(packed);
        }
        return values;
    }

    static unsigned int valueMemSize(const std::vector<Base::Color>& values)
    {
        return static_cast<unsigned int>(values.size() * sizeof(Base::Color));
    }
};

struct PropertyLinkSubMaterialMapTraits
{
    using Value = App::Material;

    static const char* xmlName()
    {
        return "LinkSubMaterialMap";
    }

    static int valueFormatVersion()
    {
        return 3;
    }

    static Py::Object toPyObject(const App::Material& value)
    {
        return Py::asObject(new MaterialPy(new Material(value)));
    }

    static App::Material fromPyObject(PyObject* value)
    {
        if (!PyObject_TypeCheck(value, &(MaterialPy::Type))) {
            std::string error = "type must be 'Material', not ";
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }
        return *static_cast<MaterialPy*>(value)->getMaterialPtr();
    }

    static void saveValues(Base::OutputStream& stream,
                           const std::vector<App::Material>& values,
                           const std::vector<std::size_t>& indices)
    {
        stream << static_cast<uint32_t>(indices.size());
        for (auto index : indices) {
            const auto& material = values[index];
            stream << material.ambientColor.getPackedValue();
            stream << material.diffuseColor.getPackedValue();
            stream << material.specularColor.getPackedValue();
            stream << material.emissiveColor.getPackedValue();
            stream << material.shininess;
            stream << material.transparency;
        }

        for (auto index : indices) {
            const auto& material = values[index];
            PropertySerialization::writeBinaryString(stream, material.image);
            PropertySerialization::writeBinaryString(stream, material.imagePath);
            PropertySerialization::writeBinaryString(stream, material.uuid);
        }
    }

    static std::vector<App::Material>
    restoreValues(Base::InputStream& stream, int restoredValueFormat)
    {
        if (restoredValueFormat != valueFormatVersion()) {
            throw Base::RuntimeError("Unsupported material map value format");
        }

        uint32_t count {};
        stream >> count;

        std::vector<App::Material> values(count);
        uint32_t packed {};
        float value {};
        for (auto& material : values) {
            stream >> packed;
            material.ambientColor.setPackedValue(packed);
            stream >> packed;
            material.diffuseColor.setPackedValue(packed);
            stream >> packed;
            material.specularColor.setPackedValue(packed);
            stream >> packed;
            material.emissiveColor.setPackedValue(packed);
            stream >> value;
            material.shininess = value;
            stream >> value;
            material.transparency = value;
        }

        for (auto& material : values) {
            PropertySerialization::readBinaryString(stream, material.image);
            PropertySerialization::readBinaryString(stream, material.imagePath);
            PropertySerialization::readBinaryString(stream, material.uuid);
        }

        return values;
    }

    static unsigned int valueMemSize(const std::vector<App::Material>& values)
    {
        return static_cast<unsigned int>(values.size() * sizeof(App::Material));
    }
};

}  // namespace App

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLinkSubValueMapBase, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubColorMap, App::PropertyLinkSubValueMapBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubColorMapHidden, App::PropertyLinkSubColorMap)
TYPESYSTEM_SOURCE(App::PropertyLinkSubMaterialMap, App::PropertyLinkSubValueMapBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubMaterialMapHidden, App::PropertyLinkSubMaterialMap)

PropertyLinkSubValueMapBase::PropertyLinkSubValueMapBase() = default;

PropertyLinkSubValueMapBase::~PropertyLinkSubValueMapBase()
{
    // Dynamic properties can be deleted while their owner remains alive.  Keep
    // the owning object's backlink set in sync in that case.
    maintainBackLinks({});
}

void PropertyLinkSubValueMapBase::afterRestore()
{
    if (!testFlag(LinkRestoreLabel)) {
        return;
    }

    setFlag(LinkRestoreLabel, false);
    for (auto& reference : _references) {
        restoreLabelReference(reference.object, reference.subName, &reference.shadow);
    }
}

void PropertyLinkSubValueMapBase::onContainerRestored()
{
    unregisterElementReference();
    for (auto& reference : _references) {
        _registerElementReference(reference.object, reference.subName, reference.shadow);
    }
}

int PropertyLinkSubValueMapBase::getSize() const
{
    return static_cast<int>(_references.size());
}

std::vector<App::DocumentObject*> PropertyLinkSubValueMapBase::getObjects() const
{
    return _references.objects();
}

std::vector<std::string> PropertyLinkSubValueMapBase::getSubValues() const
{
    return _references.subNames();
}

std::vector<std::string> PropertyLinkSubValueMapBase::getSubValues(bool newStyle) const
{
    return _references.subNames(newStyle);
}

std::vector<PropertyLinkBase::ShadowSub> PropertyLinkSubValueMapBase::getShadowSubs() const
{
    return _references.shadows();
}

std::vector<PropertyLinkSubValueMapBase::Reference>
PropertyLinkSubValueMapBase::getReferences(bool newStyle) const
{
    return _references.references(newStyle);
}

App::DocumentObject* PropertyLinkSubValueMapBase::getLinkOwner() const
{
    auto* container = getContainer();
    if (auto* owner = freecad_cast<App::DocumentObject*>(container)) {
        return owner;
    }
    return container ? container->getPropertyLinkOwner() : nullptr;
}

App::Document* PropertyLinkSubValueMapBase::getOwnerDocument() const
{
    auto* owner = getLinkOwner();
    return owner ? owner->getDocument() : nullptr;
}

void PropertyLinkSubValueMapBase::verifyObject(App::DocumentObject* obj,
                                               App::DocumentObject* owner) const
{
    if (!obj) {
        throw Base::ValueError("PropertyLinkSubValueMap does not accept null document objects");
    }
    if (!obj->isAttachedToDocument()) {
        throw Base::ValueError("PropertyLinkSubValueMap: invalid document object");
    }
    if (!testFlag(LinkAllowExternal) && owner && owner->getDocument() != obj->getDocument()) {
        throw Base::ValueError("PropertyLinkSubValueMap does not support external object");
    }
}

void PropertyLinkSubValueMapBase::maintainBackLinks(
    const std::vector<App::DocumentObject*>& objects)
{
    auto* owner = getLinkOwner();
    if (!owner || owner->testStatus(ObjectStatus::Destroy) || _pcScope == LinkScope::Hidden) {
        return;
    }

    for (auto& reference : _references) {
        if (reference.object) {
            reference.object->_removeBackLink(owner);
        }
    }

    for (auto* obj : objects) {
        if (obj) {
            obj->_addBackLink(owner);
        }
    }
}

void PropertyLinkSubValueMapBase::setReferences(std::vector<App::DocumentObject*>&& objects,
                                                std::vector<std::string>&& subNames,
                                                std::vector<ShadowSub>&& shadowSubs)
{
    const bool hasShadowSubs = shadowSubs.size() == objects.size();
    _references.set(std::move(objects), std::move(subNames), std::move(shadowSubs));

    if (hasShadowSubs) {
        onContainerRestored();
    }
    else {
        updateElementReference(nullptr);
    }

    checkLabelReferences(getSubValues());
}

void PropertyLinkSubValueMapBase::saveReferences(Base::Writer& writer,
                                                 const char* elementName) const
{
    auto* owner = getLinkOwner();
    const bool exporting = owner && owner->isExporting();

    for (auto index : getPersistedEntryIndices()) {
        const auto& reference = _references[index];
        auto* obj = reference.object;
        const auto& shadow = reference.shadow;
        const auto& sub = shadow.oldName.empty() ? reference.subName : shadow.oldName;

        // Store the original reference plus optional topology-shadow state. A
        // shadowed entry keeps the currently mapped subname, while mapped marks
        // exported references that should adopt their remapped name after load.
        writer.Stream() << writer.ind() << "<" << elementName << " obj=\""
                        << obj->getExportName() << "\" sub=\"";
        if (exporting) {
            std::string exportName;
            writer.Stream() << encodeAttribute(exportSubName(exportName, obj, sub.c_str()));
            if (!shadow.oldName.empty() && reference.subName == shadow.newName) {
                writer.Stream() << "\" " << MappedAttribute << "=\"1";
            }
        }
        else {
            writer.Stream() << encodeAttribute(sub);
            if (!reference.subName.empty()) {
                if (sub != reference.subName) {
                    writer.Stream() << "\" " << ShadowedAttribute << "=\""
                                    << encodeAttribute(reference.subName);
                }
                else if (!shadow.newName.empty()) {
                    writer.Stream() << "\" " << ShadowAttribute << "=\""
                                    << encodeAttribute(shadow.newName);
                }
            }
        }
        writer.Stream() << "\"/>\n";
    }
}

void PropertyLinkSubValueMapBase::restoreReferences(
    Base::XMLReader& reader,
    const char* elementName,
    int count,
    std::vector<std::size_t>& restoredValueIndices)
{
    std::vector<Reference> references;
    std::vector<App::DocumentObject*> backlinkObjects;
    std::vector<int> mapped;
    // XML stores references, while typed values live in the side file. If a
    // referenced object is missing during restore, skip that reference and keep
    // the original XML position so RestoreDocFile() can replay only surviving
    // value payloads.
    references.reserve(count);
    backlinkObjects.reserve(count);
    restoredValueIndices.clear();
    restoredValueIndices.reserve(count);

    auto* document = getOwnerDocument();
    bool restoreLabel = false;

    for (int i = 0; i < count; ++i) {
        reader.readElement(LinkElementName);
        std::string name = reader.getName(reader.getAttribute<const char*>("obj"));
        auto* child = document ? document->getObject(name.c_str()) : nullptr;
        if (child) {
            Reference reference;
            reference.object = child;
            reference.shadow.oldName =
                importSubName(reader, reader.getAttribute<const char*>("sub"), restoreLabel);
            if (reader.hasAttribute(ShadowedAttribute)) {
                reference.shadow.newName =
                    importSubName(reader, reader.getAttribute<const char*>(ShadowedAttribute), restoreLabel);
                reference.subName = reference.shadow.newName;
            }
            else {
                reference.subName = reference.shadow.oldName;
                if (reader.hasAttribute(ShadowAttribute)) {
                    reference.shadow.newName =
                        importSubName(reader, reader.getAttribute<const char*>(ShadowAttribute), restoreLabel);
                }
            }
            if (reader.hasAttribute(MappedAttribute)) {
                mapped.push_back(static_cast<int>(references.size()));
            }
            restoredValueIndices.push_back(static_cast<std::size_t>(i));
            backlinkObjects.push_back(child);
            references.push_back(std::move(reference));
        }
        else if (reader.isVerbose()) {
            Base::Console().warning("Lost link to '%s' while loading, maybe "
                                    "an object was not loaded correctly\n",
                                    name.c_str());
        }
    }

    reader.readEndElement(elementName);

    setFlag(LinkRestoreLabel, restoreLabel);
    maintainBackLinks(backlinkObjects);
    _references.set(std::move(references));
    onContainerRestored();
    checkLabelReferences(getSubValues());
    _mapped = std::move(mapped);
}

std::vector<std::size_t> PropertyLinkSubValueMapBase::getPersistedEntryIndices() const
{
    return _references.attachedEntryIndices();
}

void PropertyLinkSubValueMapBase::updateElementReference(DocumentObject* feature,
                                                         bool reverse,
                                                         bool notify)
{
    if (!feature) {
        _references.clearShadows();
        unregisterElementReference();
    }

    auto* owner = getLinkOwner();
    if (owner && owner->isRestoring()) {
        return;
    }

    bool touched = false;
    for (auto& reference : _references) {
        auto storeRecoveredSubName = [&](const std::string& subName) {
            if (subName == reference.subName && reference.shadow.oldName.empty()
                && reference.shadow.newName.empty()) {
                return false;
            }
            if (notify && !touched) {
                aboutToSetValue();
            }
            reference.subName = subName;
            reference.shadow = {};
            touched = true;
            return true;
        };

        if (!reverse) {
            const std::string cachedSubName = findCachedSubName(feature, reference);
            if (!cachedSubName.empty()) {
                auto subName = cachedSubName;
                ShadowSub shadow;
                if (_updateElementReference(feature,
                                            reference.object,
                                            subName,
                                            shadow,
                                            reverse,
                                            notify && !touched)) {
                    reference.subName = std::move(subName);
                    reference.shadow = std::move(shadow);
                    touched = true;
                    continue;
                }
                if (storeRecoveredSubName(cachedSubName)) {
                    continue;
                }
            }
        }

        // Stale generated names from older stored maps can become missing even
        // though they still carry a usable indexed `.FaceN` tail. Let normal
        // TNP remapping try first; only fall back to that indexed tail after
        // the reference is explicitly reported as missing.
        const std::string fallbackSubName = reverse ? std::string() : indexedFallbackSubName(reference);
        if (_updateElementReference(feature,
                                    reference.object,
                                    reference.subName,
                                    reference.shadow,
                                    reverse,
                                    notify && !touched,
                                    fallbackSubName.empty())) {
            if (!fallbackSubName.empty()
                && isMissingReference(reference.subName, reference.shadow)
                && storeRecoveredSubName(fallbackSubName)) {
                continue;
            }
            touched = true;
        }
    }

    if (!touched) {
        return;
    }

    std::vector<int> mapped;
    mapped.reserve(_mapped.size());
    for (int index : _mapped) {
        if (index < static_cast<int>(_references.size())) {
            // `_mapped` entries have received topology-shadow updates but have
            // not yet been materialized into the stored subname. Once the new
            // name is known, make it the visible reference and drop the entry
            // from the pending mapped list.
            if (!_references[index].shadow.newName.empty()) {
                _references[index].subName = _references[index].shadow.newName;
            }
            else {
                mapped.push_back(index);
            }
        }
    }
    _mapped.swap(mapped);

    if (owner && feature) {
        owner->onUpdateElementReference(this);
    }
    if (notify) {
        hasSetValue();
    }
}

bool PropertyLinkSubValueMapBase::referenceChanged() const
{
    return !_mapped.empty();
}

void PropertyLinkSubValueMapBase::getLinks(std::vector<App::DocumentObject*>& objs,
                                           bool all,
                                           std::vector<std::string>* subs,
                                           bool newStyle) const
{
    if (!all && _pcScope == LinkScope::Hidden) {
        return;
    }

    objs.reserve(objs.size() + _references.size());
    for (const auto& reference : _references) {
        if (reference.object && reference.object->isAttachedToDocument()) {
            objs.push_back(reference.object);
        }
    }

    if (subs) {
        auto subNames = getSubValues(newStyle);
        subs->reserve(subs->size() + subNames.size());
        std::move(subNames.begin(), subNames.end(), std::back_inserter(*subs));
    }
}

void PropertyLinkSubValueMapBase::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                             App::DocumentObject* obj,
                                             const char* subname,
                                             bool all) const
{
    if (!obj || (!all && _pcScope == LinkScope::Hidden)) {
        return;
    }

    App::SubObjectT objT(obj, subname);
    auto subObject = objT.getSubObject();
    auto subElement = objT.getOldElementName();

    for (std::size_t i = 0; i < _references.size(); ++i) {
        const auto& reference = _references[i];
        if (reference.object != obj) {
            continue;
        }

        if (!subname || reference.subName == subname) {
            identifiers.emplace_back(*this, static_cast<int>(i));
            continue;
        }

        if (!subObject) {
            continue;
        }

        App::SubObjectT stored(obj, reference.subName.c_str());
        if (stored.getSubObject() == subObject && stored.getOldElementName() == subElement) {
            identifiers.emplace_back(*this);
            continue;
        }

        const auto& shadow = reference.shadow;
        App::SubObjectT shadowed(
            obj,
            shadow.newName.empty() ? shadow.oldName.c_str() : shadow.newName.c_str()
        );
        if (shadowed.getSubObject() == subObject && shadowed.getOldElementName() == subElement) {
            identifiers.emplace_back(*this);
        }
    }
}

template<class ValueT, class TraitsT, class DerivedT>
PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::PropertyLinkSubValueMapT()
    : _valueFormatVersion(TraitsT::valueFormatVersion())
{
}

template<class ValueT, class TraitsT, class DerivedT>
PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::~PropertyLinkSubValueMapT() = default;

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::clear()
{
    setEntries({}, {}, {}, {}, false);
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::setValue()
{
    clear();
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::setValue(App::DocumentObject* object,
                                                                   const char* subName,
                                                                   const Value& value)
{
    std::vector<App::DocumentObject*> objects = getObjects();
    std::vector<std::string> subNames = getSubValues();
    std::vector<Value> values = _values;
    std::vector<ShadowSub> shadowSubs = getShadowSubs();

    const std::string sub = subName ? subName : "";
    for (std::size_t i = 0; i < objects.size(); ++i) {
        if (objects[i] == object && subNames[i] == sub) {
            objects.erase(objects.begin() + static_cast<std::ptrdiff_t>(i));
            subNames.erase(subNames.begin() + static_cast<std::ptrdiff_t>(i));
            values.erase(values.begin() + static_cast<std::ptrdiff_t>(i));
            shadowSubs.erase(shadowSubs.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }

    objects.push_back(object);
    subNames.push_back(sub);
    values.push_back(value);
    shadowSubs.emplace_back();
    setEntries(std::move(objects), std::move(subNames), std::move(values), std::move(shadowSubs), false);
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::setValues(
    std::vector<App::DocumentObject*> objects,
    std::vector<std::string> subNames,
    std::vector<Value> values,
    std::vector<ShadowSub>&& shadowSubs)
{
    setEntries(std::move(objects), std::move(subNames), std::move(values), std::move(shadowSubs), true);
}

template<class ValueT, class TraitsT, class DerivedT>
bool PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::getValue(App::DocumentObject* object,
                                                                  const char* subName,
                                                                  Value& value,
                                                                  bool newStyle) const
{
    const std::string sub = subName ? subName : "";
    auto subNames = getSubValues(newStyle);
    for (std::size_t i = 0; i < _references.size(); ++i) {
        if (_references[i].object == object && subNames[i] == sub) {
            value = _values[i];
            return true;
        }
    }
    return false;
}

template<class ValueT, class TraitsT, class DerivedT>
std::vector<typename PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::Entry>
PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::getEntries(bool newStyle) const
{
    std::vector<Entry> entries;
    entries.reserve(_references.size());

    auto subNames = getSubValues(newStyle);
    for (std::size_t i = 0; i < _references.size(); ++i) {
        entries.push_back({_references[i].object, std::move(subNames[i]), _values[i]});
    }

    return entries;
}

template<class ValueT, class TraitsT, class DerivedT>
std::map<std::string, ValueT>
PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::getValuesForObject(App::DocumentObject* object,
                                                                        bool newStyle) const
{
    std::map<std::string, Value> values;
    auto subNames = getSubValues(newStyle);
    for (std::size_t i = 0; i < _references.size(); ++i) {
        if (_references[i].object == object) {
            values[subNames[i]] = _values[i];
        }
    }
    return values;
}

template<class ValueT, class TraitsT, class DerivedT>
PyObject* PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::getPyObject()
{
    Py::List list(_references.size());
    auto subNames = getSubValues(true);

    for (std::size_t i = 0; i < _references.size(); ++i) {
        Py::Tuple item(3);
        auto* object = _references[i].object;
        item[0] = object ? Py::asObject(object->getPyObject()) : Py::None();
        item[1] = Py::String(subNames[i]);
        item[2] = TraitsT::toPyObject(_values[i]);
        list.setItem(static_cast<int>(i), item);
    }

    return Py::new_reference_to(list);
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::setPyObject(PyObject* value)
{
    static const char* errorMessage =
        "expected None, [(object, subname, value), ...], "
        "or (object, {subname: value, ...})";

    if (value == Py_None) {
        clear();
        return;
    }
    if (!PyTuple_Check(value) && !PyList_Check(value)) {
        throw Base::TypeError(errorMessage);
    }

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    std::vector<Value> values;

    Py::Sequence sequence(value);
    if (sequence.size() == 0) {
        clear();
        return;
    }

    if (sequence.size() == 2 && PyObject_TypeCheck(sequence[0].ptr(), &(DocumentObjectPy::Type))) {
        auto* object = static_cast<DocumentObjectPy*>(sequence[0].ptr())->getDocumentObjectPtr();
        Py::Object mapping = sequence[1];
        appendPySingleObjectEntries<TraitsT>(objects, subNames, values, object, mapping.ptr(), errorMessage);
        setValues(std::move(objects), std::move(subNames), std::move(values));
        return;
    }

    appendPyObjectEntries<TraitsT>(objects, subNames, values, value, errorMessage);
    setValues(std::move(objects), std::move(subNames), std::move(values));
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::Save(Base::Writer& writer) const
{
    if (writer.isForceXML()) {
        // Values are stored in a side file, like PropertyColorList and
        // PropertyMaterialList. In force-XML mode there is no value payload to
        // pair with these references, so omit the property entirely.
        return;
    }

    const auto indices = getPersistedEntryIndices();
    writer.Stream() << writer.ind() << "<" << TraitsT::xmlName() << " count=\""
                    << indices.size() << "\" " << ValueFileAttribute << "=\""
                    << (indices.empty() ? "" : writer.addFile(getName(), this))
                    << "\" " << MapVersionAttribute << "=\"" << MapFormatVersion << "\" "
                    << ValueVersionAttribute << "=\"" << TraitsT::valueFormatVersion() << "\">"
                    << "\n";
    writer.incInd();
    saveReferences(writer, LinkElementName);
    writer.decInd();
    writer.Stream() << writer.ind() << "</" << TraitsT::xmlName() << ">\n";
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::Restore(Base::XMLReader& reader)
{
    reader.readElement(TraitsT::xmlName());
    const int count = reader.getAttribute<long>("count");
    const int mapFormatVersion = reader.hasAttribute(MapVersionAttribute)
        ? static_cast<int>(reader.getAttribute<long>(MapVersionAttribute))
        : MapFormatVersion;
    if (mapFormatVersion > MapFormatVersion) {
        throw Base::RuntimeError("Unsupported link sub value map format");
    }

    _valueFormatVersion = reader.hasAttribute(ValueVersionAttribute)
        ? static_cast<int>(reader.getAttribute<long>(ValueVersionAttribute))
        : TraitsT::valueFormatVersion();

    std::string file;
    if (reader.hasAttribute(ValueFileAttribute)) {
        file = reader.getAttribute<const char*>(ValueFileAttribute);
    }

    restoreReferences(reader, TraitsT::xmlName(), count, _restoreValueIndices);
    _values.assign(_references.size(), Value {});

    if (!file.empty()) {
        reader.addFile(file.c_str(), this);
    }
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream stream(writer.Stream());
    TraitsT::saveValues(stream, _values, getPersistedEntryIndices());
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream stream(reader);
    auto restoredValues = TraitsT::restoreValues(stream, _valueFormatVersion);

    std::vector<Value> values;
    values.reserve(_restoreValueIndices.size());
    // References whose target object was lost during XML restore are skipped,
    // so replay only the side-file values that still have a surviving entry.
    for (auto index : _restoreValueIndices) {
        if (index < restoredValues.size()) {
            values.push_back(restoredValues[index]);
        }
    }

    if (values.size() != _references.size()) {
        values.resize(_references.size());
    }

    aboutToSetValue();
    _values = std::move(values);
    hasSetValue();
    _restoreValueIndices.clear();
}

template<class ValueT, class TraitsT, class DerivedT>
Property* PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::Copy() const
{
    auto* copy = new DerivedT();
    copy->_references = _references;
    copy->_mapped = _mapped;
    copy->_values = _values;
    return copy;
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::Paste(const Property& from)
{
    if (!from.isDerivedFrom<DerivedT>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }

    const auto& map = static_cast<const PropertyLinkSubValueMapT&>(from);
    setEntries(map.getObjects(),
               map.getSubValues(),
               std::vector<Value>(map._values),
               map.getShadowSubs(),
               false);
}

template<class ValueT, class TraitsT, class DerivedT>
Property* PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::CopyOnImportExternal(
    const std::map<std::string, std::string>& nameMap) const
{
    auto* owner = getLinkOwner();
    if (!owner || !owner->getDocument()) {
        return nullptr;
    }

    bool changed = false;
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    std::vector<Value> values;
    objects.reserve(_references.size());
    subNames.reserve(_references.size());
    values.reserve(_values.size());

    for (std::size_t i = 0; i < _references.size(); ++i) {
        const auto& reference = _references[i];
        auto* object = tryImport(owner->getDocument(), reference.object, nameMap);
        auto subName =
            tryImportSubName(reference.object, reference.subName.c_str(), owner->getDocument(), nameMap);
        if (!subName.empty()) {
            changed = true;
        }
        else {
            subName = reference.subName;
        }
        changed = changed || object != reference.object;
        objects.push_back(object);
        subNames.push_back(std::move(subName));
        values.push_back(_values[i]);
    }

    if (!changed) {
        return nullptr;
    }

    auto* copy = new DerivedT();
    copy->setEntries(std::move(objects), std::move(subNames), std::move(values), {}, true);
    return copy;
}

template<class ValueT, class TraitsT, class DerivedT>
Property* PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::CopyOnLabelChange(
    App::DocumentObject* obj,
    const std::string& ref,
    const char* newLabel) const
{
    auto* owner = getLinkOwner();
    if (!owner || !owner->getDocument()) {
        return nullptr;
    }

    bool changed = false;
    std::vector<std::string> subNames;
    subNames.reserve(_references.size());
    for (const auto& reference : _references) {
        auto subName =
            updateLabelReference(reference.object, reference.subName.c_str(), obj, ref, newLabel);
        if (!subName.empty()) {
            changed = true;
            subNames.push_back(std::move(subName));
        }
        else {
            subNames.push_back(reference.subName);
        }
    }

    if (!changed) {
        return nullptr;
    }

    auto* copy = new DerivedT();
    copy->setEntries(getObjects(),
                     std::move(subNames),
                     std::vector<Value>(_values),
                     {},
                     true);
    return copy;
}

template<class ValueT, class TraitsT, class DerivedT>
Property* PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::CopyOnLinkReplace(
    const App::DocumentObject* parent,
    App::DocumentObject* oldObj,
    App::DocumentObject* newObj) const
{
    bool changed = false;
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    std::vector<Value> values;
    objects.reserve(_references.size());
    subNames.reserve(_references.size());
    values.reserve(_values.size());

    for (std::size_t i = 0; i < _references.size(); ++i) {
        auto* object = _references[i].object;
        auto subName = _references[i].subName;
        auto replacement =
            tryReplaceLink(getContainer(), object, parent, oldObj, newObj, subName.c_str());
        if (replacement.first) {
            object = replacement.first;
            subName = std::move(replacement.second);
            changed = true;
        }
        objects.push_back(object);
        subNames.push_back(std::move(subName));
        values.push_back(_values[i]);
    }

    if (!changed) {
        return nullptr;
    }

    auto* copy = new DerivedT();
    copy->setEntries(std::move(objects), std::move(subNames), std::move(values), {}, true);
    return copy;
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::breakLink(App::DocumentObject* obj,
                                                                    bool clearLinks)
{
    if (clearLinks && getLinkOwner() == obj) {
        this->clear();
        return;
    }

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    std::vector<Value> values;
    objects.reserve(_references.size());
    subNames.reserve(_references.size());
    values.reserve(_values.size());

    for (std::size_t i = 0; i < _references.size(); ++i) {
        if (_references[i].object == obj) {
            continue;
        }
        objects.push_back(_references[i].object);
        subNames.push_back(_references[i].subName);
        values.push_back(_values[i]);
    }

    if (objects.size() != _references.size()) {
        setEntries(std::move(objects), std::move(subNames), std::move(values), {}, false);
    }
}

template<class ValueT, class TraitsT, class DerivedT>
bool PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::adjustLink(
    const std::set<App::DocumentObject*>& inList)
{
    if (_pcScope == LinkScope::Hidden) {
        return false;
    }

    auto objects = getObjects();
    auto subNames = getSubValues();
    bool touched = false;

    for (std::size_t i = 0; i < subNames.size(); ++i) {
        auto*& object = objects[i];
        auto& subName = subNames[i];
        if (!object || !object->isAttachedToDocument() || !inList.contains(object)) {
            continue;
        }

        touched = true;
        auto pos = subName.find('.');
        for (; pos != std::string::npos; pos = subName.find('.', pos + 1)) {
            auto* subObject = object->getSubObject(subName.substr(0, pos + 1).c_str());
            if (!subObject || subObject->getDocument() != object->getDocument()) {
                pos = std::string::npos;
                break;
            }
            if (!inList.contains(subObject)) {
                object = subObject;
                subName = subName.substr(pos + 1);
                break;
            }
        }
        if (pos == std::string::npos) {
            return false;
        }
    }

    if (touched) {
        setEntries(std::move(objects), std::move(subNames), std::vector<Value>(_values), {}, true);
    }
    return touched;
}

template<class ValueT, class TraitsT, class DerivedT>
unsigned int PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::getMemSize() const
{
    unsigned int size = _references.getMemSize();
    size += TraitsT::valueMemSize(_values);
    return size;
}

template<class ValueT, class TraitsT, class DerivedT>
bool PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::isSame(const Property& other) const
{
    if (&other == this) {
        return true;
    }
    if (getTypeId() != other.getTypeId()) {
        return false;
    }

    const auto& map = static_cast<const PropertyLinkSubValueMapT&>(other);
    if (getScope() != map.getScope() || _references.size() != map._references.size()
        || _values != map._values) {
        return false;
    }

    for (std::size_t i = 0; i < _references.size(); ++i) {
        if (_references[i].object != map._references[i].object
            || _references[i].subName != map._references[i].subName) {
            return false;
        }
    }

    return true;
}

template<class ValueT, class TraitsT, class DerivedT>
void PropertyLinkSubValueMapT<ValueT, TraitsT, DerivedT>::setEntries(
    std::vector<App::DocumentObject*> objects,
    std::vector<std::string> subNames,
    std::vector<Value> values,
    std::vector<ShadowSub>&& shadowSubs,
    bool canonicalize)
{
    if (objects.size() != subNames.size() || objects.size() != values.size()) {
        throw Base::ValueError("PropertyLinkSubValueMap: object, subelement, and value counts differ");
    }

    auto* owner = getLinkOwner();
    for (auto* object : objects) {
        verifyObject(object, owner);
    }

    if (shadowSubs.size() != objects.size()) {
        shadowSubs.clear();
        shadowSubs.resize(objects.size());
    }

    if (canonicalize) {
        // Duplicate keys follow Python dict-style last-write-wins semantics,
        // while preserving the relative order of surviving entries.
        std::map<std::pair<App::DocumentObject*, std::string>, std::size_t> lastIndexByKey;
        for (std::size_t i = 0; i < objects.size(); ++i) {
            lastIndexByKey[{objects[i], subNames[i]}] = i;
        }

        std::vector<App::DocumentObject*> canonicalObjects;
        std::vector<std::string> canonicalSubNames;
        std::vector<Value> canonicalValues;
        std::vector<ShadowSub> canonicalShadowSubs;
        canonicalObjects.reserve(lastIndexByKey.size());
        canonicalSubNames.reserve(lastIndexByKey.size());
        canonicalValues.reserve(lastIndexByKey.size());
        canonicalShadowSubs.reserve(lastIndexByKey.size());

        for (std::size_t i = 0; i < objects.size(); ++i) {
            const auto key = std::make_pair(objects[i], subNames[i]);
            const auto last = lastIndexByKey.find(key);
            if (last == lastIndexByKey.end() || last->second != i) {
                continue;
            }

            canonicalObjects.push_back(objects[i]);
            canonicalSubNames.push_back(std::move(subNames[i]));
            canonicalValues.push_back(std::move(values[i]));
            canonicalShadowSubs.push_back(std::move(shadowSubs[i]));
        }

        objects = std::move(canonicalObjects);
        subNames = std::move(canonicalSubNames);
        values = std::move(canonicalValues);
        shadowSubs = std::move(canonicalShadowSubs);
    }

    maintainBackLinks(objects);

    aboutToSetValue();
    _values = std::move(values);
    setReferences(std::move(objects), std::move(subNames), std::move(shadowSubs));
    hasSetValue();
}

namespace App
{

template class PropertyLinkSubValueMapT<Base::Color,
                                        PropertyLinkSubColorMapTraits,
                                        PropertyLinkSubColorMap>;
template class PropertyLinkSubValueMapT<App::Material,
                                        PropertyLinkSubMaterialMapTraits,
                                        PropertyLinkSubMaterialMap>;

}  // namespace App
