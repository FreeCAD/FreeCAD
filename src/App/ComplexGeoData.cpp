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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
#endif

#include <boost/regex.hpp>

#include "ComplexGeoData.h"
#include "ElementMap.h"
#include "ElementNamingUtils.h"

#include <Base/BoundBox.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>


using namespace Data;

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment , Base::BaseClass)


TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData , Base::Persistence)

FC_LOG_LEVEL_INIT("ComplexGeoData", true,true)

namespace bio = boost::iostreams;
using namespace Data;


ComplexGeoData::ComplexGeoData()
    :Tag(0)
{
}

ComplexGeoData::~ComplexGeoData() = default;

Data::Segment* ComplexGeoData::getSubElementByName(const char* name) const
{
    int index = 0;
    std::string element;
    boost::regex ex("^([^0-9]*)([0-9]*)$");
    boost::cmatch what;

    if (boost::regex_match(name, what, ex)) {
        element = what[1].str();
        index = std::atoi(what[2].str().c_str());
    }

    return getSubElement(element.c_str(), static_cast<unsigned long>(index));
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

    return Base::Placement(Base::Vector3d(mat[0][3],
                                          mat[1][3],
                                          mat[2][3]),
                           Base::Rotation(mat));
}

double ComplexGeoData::getAccuracy() const
{
    return 0.0;
}

void ComplexGeoData::getLinesFromSubElement(const Segment*,
                                            std::vector<Base::Vector3d> &Points,
                                            std::vector<Line> &lines) const
{
    (void)Points;
    (void)lines;
}

void ComplexGeoData::getFacesFromSubElement(const Segment*,
                                            std::vector<Base::Vector3d> &Points,
                                            std::vector<Base::Vector3d> &PointNormals,
                                            std::vector<Facet> &faces) const
{
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

void ComplexGeoData::getPoints(std::vector<Base::Vector3d> &Points,
                               std::vector<Base::Vector3d> &Normals,
                               double Accuracy, uint16_t flags) const
{
    (void)Points;
    (void)Normals;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getLines(std::vector<Base::Vector3d> &Points,
                              std::vector<Line> &lines,
                              double Accuracy, uint16_t flags) const
{
    (void)Points;
    (void)lines;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getFaces(std::vector<Base::Vector3d> &Points,
                              std::vector<Facet> &faces,
                              double Accuracy, uint16_t flags) const
{
    (void)Points;
    (void)faces;
    (void)Accuracy;
    (void)flags;
}

bool ComplexGeoData::getCenterOfGravity(Base::Vector3d&) const
{
    return false;
}

size_t ComplexGeoData::getElementMapSize(bool flush) const {
    if (flush) {
        flushElementMap();
#ifdef _FC_MEM_TRACE
        FC_MSG("memory size " << (_MemSize/1024/1024) << "MB, " << (_MemMaxSize/1024/1024));
        for (auto &unit : _MemUnits)
            FC_MSG("unit " << unit.first << ": " << unit.second.count << ", " << unit.second.maxcount);
#endif
    }
    return _elementMap ? _elementMap->size():0;
}

MappedName ComplexGeoData::getMappedName(const IndexedName & element,
                                         bool allowUnmapped,
                                         ElementIDRefs *sid) const
{
    if (!element)
        return MappedName();
    flushElementMap();
    if(!_elementMap) {
        if (allowUnmapped)
            return MappedName(element);
        return MappedName();
    }

    MappedName name = _elementMap->find(element, sid);
    if (allowUnmapped && !name)
        return MappedName(element);
    return name;
}

IndexedName ComplexGeoData::getIndexedName(const MappedName & name,
                                           ElementIDRefs *sid) const
{
    flushElementMap();
    if (!name)
        return IndexedName();
    if (!_elementMap) {
        std::string s;
        return IndexedName(name.appendToBuffer(s), getElementTypes());
    }
    return _elementMap->find(name, sid);
}

Data::MappedElement
ComplexGeoData::getElementName(const char *name,
                               ElementIDRefs *sid,
                               bool copy) const
{
    IndexedName element(name, getElementTypes());
    if (element)
        return MappedElement(getMappedName(element, false, sid), element);

    const char * mapped = isMappedElement(name);
    if (mapped)
        name = mapped;

    MappedElement res;
    // Strip out the trailing '.XXXX' if any
    const char *dot = strchr(name,'.');
    if(dot)
        res.name = MappedName(name, dot-name);
    else if (copy)
        res.name = name;
    else
        res.name = MappedName(name);
    res.index = getIndexedName(res.name, sid);
    return res;
}

std::vector<std::pair<MappedName, ElementIDRefs> >
ComplexGeoData::getElementMappedNames(const IndexedName & element, bool needUnmapped) const {
    flushElementMap();
    if(_elementMap) {
        auto res = _elementMap->findAll(element);
        if (!res.empty())
            return res;
    }

    if (!needUnmapped)
        return {};
    return {std::make_pair(MappedName(element), ElementIDRefs())};
}

std::vector<Data::MappedElement>
ComplexGeoData::getElementNamesWithPrefix(const char *prefix) const {
#if 0
    std::vector<Data::MappedElement> names;
    flushElementMap();
    if(!prefix || !prefix[0] || !_elementMap)
        return names;
    const auto &p = elementMapPrefix();
    if(boost::starts_with(prefix,p))
        prefix += p.size();
    names = _elementMap->findAllStartsWith(prefix);
    return names;
#else
    (void)prefix;
    return {};
#endif
}

std::vector<MappedElement> ComplexGeoData::getElementMap() const {
    flushElementMap();
    if(!_elementMap)
        return {};
    return _elementMap->getAll();
}

ElementMapPtr ComplexGeoData::elementMap(bool flush) const
{
    if (flush)
        flushElementMap();
    return _elementMap;
}

void ComplexGeoData::flushElementMap() const
{
}

void ComplexGeoData::setElementMap(const std::vector<MappedElement> &map) {
    resetElementMap();
    for(auto &v : map)
        _elementMap->setElementName(v.index, v.name, Tag);
}

char ComplexGeoData::elementType(const Data::MappedName &name) const
{
    if(!name)
        return 0;
    auto indexedName = getIndexedName(name);
    if (indexedName)
        return elementType(indexedName);
    char element_type=0;
    if (name.findTagInElementName(0,0,0,&element_type) < 0)
        return elementType(name.toIndexedName());
    return element_type;
}

char ComplexGeoData::elementType(const Data::IndexedName &element) const
{
    if(!element)
        return 0;
    for(auto &type : getElementTypes()) {
        if(boost::equals(element.getType(), type))
            return type[0];
    }
    return 0;
}

char ComplexGeoData::elementType(const char *name) const {
    if(!name)
        return 0;

    const char *type = nullptr;
    IndexedName element(name, getElementTypes());
    if (element)
        type = element.getType();
    else {
        const char * mapped = isMappedElement(name);
        if (mapped)
            name = mapped;

        MappedName n;
        const char *dot = strchr(name,'.');
        if(dot) {
            n = MappedName(name, dot-name);
            type = dot+1;
        }
        else
            n = MappedName::fromRawData(name);
        char res = elementType(n);
        if (res)
            return res;
    }

    if(type && type[0]) {
        for(auto &t : getElementTypes()) {
            if(boost::starts_with(type, t))
                return type[0];
        }
    }
    return 0;
}

MappedName ComplexGeoData::renameDuplicateElement(int index,
                                                  const IndexedName & element,
                                                  const IndexedName & element2,
                                                  const MappedName & name,
                                                  ElementIDRefs &sids)
{
    std::ostringstream ss;
    ss << ELEMENT_MAP_PREFIX << 'D' << std::hex << index;
    MappedName renamed(name);
    this->elementMap()->encodeElementName(element.getType()[0],renamed,ss,&sids,Tag);
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        FC_WARN("duplicate element mapping '" << name << " -> " << renamed << ' '
                                              << element << '/' << element2);
    return renamed;
}
