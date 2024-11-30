// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
 *   Copyright (c) 2022 Zheng, Lei <realthunder.dev@gmail.com>              *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include "PreCompiled.h"  // NOLINT

#ifndef _PreComp_
#include <cstdlib>
#endif

#include <boost/regex.hpp>

#include "ComplexGeoData.h"
#include "ElementMap.h"
#include "ElementNamingUtils.h"

#include <Base/BoundBox.h>
#include <Base/Placement.h>
#include <Base/Reader.h>
#include <Base/Rotation.h>
#include <Base/Writer.h>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>


using namespace Data;

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment, Base::BaseClass)           // NOLINT
TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData, Base::Persistence)  // NOLINT

FC_LOG_LEVEL_INIT("ComplexGeoData", true, true)  // NOLINT

namespace bio = boost::iostreams;
using namespace Data;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

ComplexGeoData::ComplexGeoData() = default;

std::pair<std::string, unsigned long> ComplexGeoData::getTypeAndIndex(const char* Name)
{
    int index = 0;
    std::string element;
    boost::regex ex("^([^0-9]*)([0-9]*)$");
    boost::cmatch what;

    if (Name && boost::regex_match(Name, what, ex)) {
        element = what[1].str();
        index = std::atoi(what[2].str().c_str());
    }

    return std::make_pair(element, index);
}

Data::Segment* ComplexGeoData::getSubElementByName(const char* name) const
{
    auto type = getTypeAndIndex(name);
    return getSubElement(type.first.c_str(), type.second);
}

void ComplexGeoData::applyTransform(const Base::Matrix4D& rclTrf)
{
    setTransform(rclTrf * getTransform());
}

void ComplexGeoData::applyTranslation(const Base::Vector3d& mov)
{
    Base::Matrix4D mat;
    mat.move(mov);
    setTransform(mat * getTransform());
}

void ComplexGeoData::applyRotation(const Base::Rotation& rot)
{
    Base::Matrix4D mat;
    rot.getValue(mat);
    setTransform(mat * getTransform());
}

void ComplexGeoData::setPlacement(const Base::Placement& rclPlacement)
{
    setTransform(rclPlacement.toMatrix());
}

Base::Placement ComplexGeoData::getPlacement() const
{
    Base::Matrix4D mat = getTransform();

    return {Base::Vector3d(mat[0][3], mat[1][3], mat[2][3]), Base::Rotation(mat)};
}

double ComplexGeoData::getAccuracy() const
{
    return 0.0;
}

void ComplexGeoData::getLinesFromSubElement(const Segment* segment,
                                            std::vector<Base::Vector3d>& Points,
                                            std::vector<Line>& lines) const
{
    (void)segment;
    (void)Points;
    (void)lines;
}

void ComplexGeoData::getFacesFromSubElement(const Segment* segment,
                                            std::vector<Base::Vector3d>& Points,
                                            std::vector<Base::Vector3d>& PointNormals,
                                            std::vector<Facet>& faces) const
{
    (void)segment;
    (void)Points;
    (void)PointNormals;
    (void)faces;
}

Base::Vector3d ComplexGeoData::getPointFromLineIntersection(const Base::Vector3f& base,
                                                            const Base::Vector3f& dir) const
{
    (void)base;
    (void)dir;
    return Base::Vector3d();
}

void ComplexGeoData::getPoints(std::vector<Base::Vector3d>& Points,
                               std::vector<Base::Vector3d>& Normals,
                               double Accuracy,
                               uint16_t flags) const
{
    (void)Points;
    (void)Normals;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getLines(std::vector<Base::Vector3d>& Points,
                              std::vector<Line>& lines,
                              double Accuracy,
                              uint16_t flags) const
{
    (void)Points;
    (void)lines;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getFaces(std::vector<Base::Vector3d>& Points,
                              std::vector<Facet>& faces,
                              double Accuracy,
                              uint16_t flags) const
{
    (void)Points;
    (void)faces;
    (void)Accuracy;
    (void)flags;
}

bool ComplexGeoData::getCenterOfGravity(Base::Vector3d& unused) const
{
    (void)unused;
    return false;
}

const std::string& ComplexGeoData::elementMapPrefix()
{
    static std::string prefix(ELEMENT_MAP_PREFIX);
    return prefix;
}

std::string ComplexGeoData::getElementMapVersion() const
{
    return "4";
}

bool ComplexGeoData::checkElementMapVersion(const char* ver) const
{
    return !boost::equals(ver, "3") && !boost::equals(ver, "4") && !boost::starts_with(ver, "3.");
}

size_t ComplexGeoData::getElementMapSize(bool flush) const
{
    if (flush) {
        flushElementMap();
#ifdef _FC_MEM_TRACE
        FC_MSG("memory size " << (_MemSize / 1024 / 1024) << "MB, " << (_MemMaxSize / 1024 / 1024));
        for (auto& unit : _MemUnits) {
            FC_MSG("unit " << unit.first << ": " << unit.second.count << ", "
                           << unit.second.maxcount);
        }
#endif
    }
    return _elementMap ? _elementMap->size() : 0;
}

MappedName ComplexGeoData::getMappedName(const IndexedName& element,
                                         bool allowUnmapped,
                                         ElementIDRefs* sid) const
{
    if (!element) {
        return {};
    }
    flushElementMap();
    if (!_elementMap) {
        if (allowUnmapped) {
            return MappedName(element);
        }
        return {};
    }

    MappedName name = _elementMap->find(element, sid);
    if (allowUnmapped && !name) {
        return MappedName(element);
    }
    return name;
}

IndexedName ComplexGeoData::getIndexedName(const MappedName& name, ElementIDRefs* sid) const
{
    flushElementMap();
    if (!name) {
        return IndexedName();
    }
    if (!_elementMap) {
        std::string str;
        return {name.appendToBuffer(str), getElementTypes()};
    }
    return _elementMap->find(name, sid);
}

Data::MappedElement
ComplexGeoData::getElementName(const char* name, ElementIDRefs* sid, bool copy) const
{
    IndexedName element(name, getElementTypes());
    if (element) {
        return {getMappedName(element, false, sid), element};
    }

    const char* mapped = isMappedElement(name);
    if (mapped) {
        name = mapped;
    }

    MappedElement result;
    // Strip out the trailing '.XXXX' if any
    const char* dot = strchr(name, '.');
    if (dot) {
        result.name = MappedName(name, static_cast<int>(dot - name));
    }
    else if (copy) {
        result.name = name;
    }
    else {
        result.name = MappedName(name);
    }
    result.index = getIndexedName(result.name, sid);
    return result;
}

std::vector<std::pair<MappedName, ElementIDRefs>>
ComplexGeoData::getElementMappedNames(const IndexedName& element, bool needUnmapped) const
{
    flushElementMap();
    if (_elementMap) {
        auto res = _elementMap->findAll(element);
        if (!res.empty()) {
            return res;
        }
    }

    if (!needUnmapped) {
        return {};
    }
    return {std::make_pair(MappedName(element), ElementIDRefs())};
}

ElementMapPtr ComplexGeoData::resetElementMap(ElementMapPtr elementMap)
{
    _elementMap.swap(elementMap);
    // We expect that if the ComplexGeoData ( TopoShape ) has a hasher, then its elementMap will
    // have the same one.  Make sure that happens.
    if (_elementMap && !_elementMap->hasher) {
        _elementMap->hasher = Hasher;
    }
    return elementMap;
}

std::vector<MappedElement> ComplexGeoData::getElementMap() const
{
    flushElementMap();
    if (!_elementMap) {
        return {};
    }
    return _elementMap->getAll();
}

ElementMapPtr ComplexGeoData::elementMap(bool flush) const
{
    if (flush) {
        flushElementMap();
    }
    return _elementMap;
}

ElementMapPtr ComplexGeoData::ensureElementMap(bool flush)
{
    if (!_elementMap) {
        resetElementMap(std::make_shared<Data::ElementMap>());
    }
    return elementMap(flush);
}

void ComplexGeoData::flushElementMap() const
{}

void ComplexGeoData::setElementMap(const std::vector<MappedElement>& map)
{
    _elementMap = std::make_shared<Data::ElementMap>();  // Get rid of the old one, if any, but make
                                                         // sure the memory exists for the new data.
    for (auto& element : map) {
        _elementMap->setElementName(element.index, element.name, Tag);
    }
}

char ComplexGeoData::elementType(const Data::MappedName& name) const
{
    if (!name) {
        return 0;
    }
    auto indexedName = getIndexedName(name);
    if (indexedName) {
        return elementType(indexedName);
    }
    char element_type = 0;
    if (name.findTagInElementName(nullptr, nullptr, nullptr, &element_type) < 0) {
        return elementType(name.toIndexedName());
    }
    return element_type;
}

char ComplexGeoData::elementType(const Data::IndexedName& element) const
{
    if (!element) {
        return 0;
    }
    for (auto& type : getElementTypes()) {
        if (boost::equals(element.getType(), type)) {
            return type[0];
        }
    }
    return 0;
}

// The elementType function can take a char *, in which case it tries a sequence of checks to
// see what it got.
// 1) Check to see if it is an indexedName, and if so directly look up the type
// 2) If not:
//    a) Remove any element map prefix that is present
//    b) See if the name contains a dot:
//        i)  If yes, create a MappedName from the part before the dot, and set the type to the
//            part after the dot
//        ii) If no, create a MappedName from the whole name
//    c) Try to get the elementType based on the MappedName. Return it if found
// 3) Check to make sure the discovered type is in the list of types, and return its first
//    character if so.
char ComplexGeoData::elementType(const char* name) const
{
    if (!name) {
        return 0;
    }

    const char* type = nullptr;
    IndexedName element(name, getElementTypes());
    if (element) {
        type = element.getType();
    }
    else {
        const char* mapped = isMappedElement(name);
        if (mapped) {
            name = mapped;
        }

        MappedName mappedName;
        const char* dot = strchr(name, '.');
        if (dot) {
            mappedName = MappedName(name, static_cast<int>(dot - name));
            type = dot + 1;
        }
        else {
            mappedName = MappedName::fromRawData(name);
        }
        char res = elementType(mappedName);
        if (res != 0) {
            return res;
        }
    }

    if (type && (type[0] != 0)) {
        for (auto& elementTypes : getElementTypes()) {
            if (boost::starts_with(type, elementTypes)) {
                return type[0];
            }
        }
    }
    return 0;
}

void ComplexGeoData::setPersistenceFileName(const char* filename) const
{
    if (!filename) {
        filename = "";
    }
    _persistenceName = filename;
}

void ComplexGeoData::Save(Base::Writer& writer) const
{

    if (getElementMapSize() == 0U) {
        writer.Stream() << writer.ind() << "<ElementMap/>\n";
        return;
    }

    // Store some dummy map entry to trigger recompute in older version.
    writer.Stream() << writer.ind() << R"(<ElementMap new="1" count="1">)"
                    << R"(<Element key="Dummy" value="Dummy"/>)"
                    << "</ElementMap>\n";

    // New layout of element map, so we use new xml tag, ElementMap2
    writer.Stream() << writer.ind() << "<ElementMap2";

    if (!_persistenceName.empty()) {
        writer.Stream() << " file=\"" << writer.addFile((_persistenceName + ".txt").c_str(), this)
                        << "\"/>\n";
        return;
    }
    writer.Stream() << " count=\"" << _elementMap->size() << "\">\n";
    _elementMap->save(writer.beginCharStream(Base::CharStreamFormat::Raw) << '\n');
    writer.endCharStream() << '\n';
    writer.Stream() << writer.ind() << "</ElementMap2>\n";
}

void ComplexGeoData::Restore(Base::XMLReader& reader)
{
    resetElementMap();

    reader.readElement("ElementMap");
    bool newTag = false;
    if (reader.hasAttribute("new") && reader.getAttributeAsInteger("new") > 0) {
        reader.readEndElement("ElementMap");
        reader.readElement("ElementMap2");
        newTag = true;
    }

    const char* file = "";
    if (reader.hasAttribute("file")) {
        file = reader.getAttribute("file");
    }
    if (*file != 0) {
        reader.addFile(file, this);
        return;
    }

    std::size_t count = 0;
    if (reader.hasAttribute("count")) {
        count = reader.getAttributeAsUnsigned("count");
    }
    if (count == 0) {
        return;
    }

    if (newTag) {
        resetElementMap(std::make_shared<ElementMap>());
        _elementMap =
            _elementMap->restore(Hasher, reader.beginCharStream(Base::CharStreamFormat::Raw));
        reader.endCharStream();
        reader.readEndElement("ElementMap2");
        return;
    }

    if (reader.FileVersion > 1) {
        restoreStream(reader.beginCharStream(Base::CharStreamFormat::Raw), count);
        reader.endCharStream();
        return;
    }
    readElements(reader, count);
    reader.readEndElement("ElementMap");
}

void ComplexGeoData::readElements(Base::XMLReader& reader, size_t count)
{
    size_t invalid_count = 0;
    bool warned = false;

    const auto& types = getElementTypes();

    for (size_t i = 0; i < count; ++i) {
        reader.readElement("Element");
        ElementIDRefs sids;
        if (reader.hasAttribute("sid")) {
            if (!Hasher) {
                if (!warned) {
                    warned = true;
                    FC_ERR("missing hasher");  // NOLINT
                }
            }
            else {
                const char* attr = reader.getAttribute("sid");
                bio::stream<bio::array_source> iss(attr, std::strlen(attr));
                long id {};
                while ((iss >> id)) {
                    if (id == 0) {
                        continue;
                    }
                    auto sid = Hasher->getID(id);
                    if (!sid) {
                        ++invalid_count;
                    }
                    else {
                        sids.push_back(sid);
                    }
                    char sep {};
                    iss >> sep;
                }
            }
        }
        ensureElementMap()->setElementName(IndexedName(reader.getAttribute("value"), types),
                                           MappedName(reader.getAttribute("key")),
                                           Tag,
                                           &sids);
    }
    if (invalid_count != 0) {
        FC_ERR("Found " << invalid_count << " invalid string id");  // NOLINT
    }
}

void ComplexGeoData::restoreStream(std::istream& stream, std::size_t count)
{
    resetElementMap();

    size_t invalid_count = 0;
    std::string key;
    std::string value;
    std::string sid;
    bool warned = false;

    const auto& types = getElementTypes();
    try {
        for (size_t i = 0; i < count; ++i) {
            ElementIDRefs sids;
            std::size_t sCount = 0;
            if (!(stream >> value >> key >> sCount)) {
                // NOLINTNEXTLINE
                FC_THROWM(Base::RuntimeError, "Failed to restore element map " << _persistenceName);
            }
            sids.reserve(static_cast<int>(sCount));
            for (std::size_t j = 0; j < sCount; ++j) {
                long id = 0;
                if (!(stream >> id)) {
                    // NOLINTNEXTLINE
                    FC_THROWM(Base::RuntimeError,
                              "Failed to restore element map " << _persistenceName);
                }
                if (Hasher) {
                    auto hasherSID = Hasher->getID(id);
                    if (!hasherSID) {
                        ++invalid_count;
                    }
                    else {
                        sids.push_back(hasherSID);
                    }
                }
            }
            if (sCount != 0 && !Hasher) {
                sids.clear();
                if (!warned) {
                    warned = true;
                    FC_ERR("missing hasher");  // NOLINT
                }
            }
            _elementMap->setElementName(IndexedName(value.c_str(), types),
                                        MappedName(key),
                                        Tag,
                                        &sids);
        }
    }
    catch (Base::Exception& e) {
        e.ReportException();
        _restoreFailed = true;
        _elementMap.reset();
    }
    if (invalid_count != 0) {
        FC_ERR("Found " << invalid_count << " invalid string id");  // NOLINT
    }
}

void ComplexGeoData::SaveDocFile(Base::Writer& writer) const
{
    flushElementMap();
    if (_elementMap) {
        writer.Stream() << "BeginElementMap v1\n";
        _elementMap->save(writer.Stream());
    }
}

void ComplexGeoData::RestoreDocFile(Base::Reader& reader)
{
    std::string marker;
    std::string ver;
    reader >> marker;
    if (boost::equals(marker, "BeginElementMap")) {
        resetElementMap();
        reader >> ver;
        if (ver != "v1") {
            FC_WARN("Unknown element map format");  // NOLINT
        }
        else {
            resetElementMap(std::make_shared<ElementMap>());
            _elementMap = _elementMap->restore(Hasher, reader);
            return;
        }
    }
    std::size_t count = atoi(marker.c_str());
    restoreStream(reader, count);
}

unsigned int ComplexGeoData::getMemSize() const
{
    flushElementMap();
    if (_elementMap) {
        static const int multiplier {10};
        return _elementMap->size() * multiplier;
    }
    return 0;
}

std::vector<IndexedName> ComplexGeoData::getHigherElements(const char*, bool) const
{
    return {};
}

void ComplexGeoData::setMappedChildElements(
    const std::vector<Data::ElementMap::MappedChildElements>& children)
{
    // DO NOT reset element map if there is one. Because we allow mixing child
    // mapping and normal mapping
    if (!_elementMap) {
        resetElementMap(std::make_shared<Data::ElementMap>());
    }
    _elementMap->addChildElements(Tag, children);
}

std::vector<Data::ElementMap::MappedChildElements> ComplexGeoData::getMappedChildElements() const
{
    if (!_elementMap) {
        return {};
    }
    return _elementMap->getChildElements();
}

void ComplexGeoData::beforeSave() const
{
    flushElementMap();
    if (this->_elementMap) {
        this->_elementMap->beforeSave(Hasher);
    }
}

void ComplexGeoData::hashChildMaps()
{
    flushElementMap();
    if (_elementMap) {
        _elementMap->hashChildMaps(Tag);
    }
}

bool ComplexGeoData::hasChildElementMap() const
{
    flushElementMap();
    return _elementMap && _elementMap->hasChildElementMap();
}

void ComplexGeoData::dumpElementMap(std::ostream& stream) const
{
    auto map = getElementMap();
    std::sort(map.begin(), map.end());
    for (auto& element : map) {
        stream << element.index << " : " << element.name << std::endl;
    }
}

const std::string ComplexGeoData::dumpElementMap() const
{
    std::stringstream ss;
    dumpElementMap(ss);
    return ss.str();
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
