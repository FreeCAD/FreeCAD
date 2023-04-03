// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2018-2022 Zheng, Lei (realthunder)                       *
 *   <realthunder.dev@gmail.com>                                            *
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

#ifndef DATA_ELEMENTMAP_H
#define DATA_ELEMENTMAP_H

#include "FCGlobal.h"

#include "MappedElement.h"
#include "StringHasher.h"

#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>


namespace Data
{

static constexpr const char* POSTFIX_TAG = ";:H";
static constexpr const char* POSTFIX_DECIMAL_TAG = ";:T";
static constexpr const char* POSTFIX_EXTERNAL_TAG = ";:X";
static constexpr const char* POSTFIX_CHILD = ";:C";
static constexpr const char* POSTFIX_INDEX = ";:I";
static constexpr const char* POSTFIX_UPPER = ";:U";
static constexpr const char* POSTFIX_LOWER = ";:L";
static constexpr const char* POSTFIX_MOD = ";:M";
static constexpr const char* POSTFIX_GEN = ";:G";
static constexpr const char* POSTFIX_MODGEN = ";:MG";
static constexpr const char* POSTFIX_DUPLICATE = ";D";

class ElementMap;
typedef std::shared_ptr<ElementMap> ElementMapPtr;

//FIXME
struct AppExport MappedChildElements
{
    IndexedName indexedName;
    int count;
    int offset;
    long tag;
    ElementMapPtr elementMap;
    QByteArray postfix;
    ElementIDRefs sids;

    static const std::string & prefix();
};


class ElementMap: public std::enable_shared_from_this<ElementMap>
{
public:
    ElementMap();

    void beforeSave(const ::App::StringHasherRef& hasher) const;

    const MappedNameRef* findMappedRef(const IndexedName& idx) const;
    MappedNameRef* findMappedRef(const IndexedName& idx);

    MappedNameRef& mappedRef(const IndexedName& idx);

    static void addPostfix(const QByteArray& postfix, std::map<QByteArray, int>& postfixMap,
                           std::vector<QByteArray>& postfixes);

    void collectChildMaps(std::map<const ElementMap*, int>& childMapSet,
                          std::vector<const ElementMap*>& childMaps,
                          std::map<QByteArray, int>& postfixMap,
                          std::vector<QByteArray>& postfixes) const;

    void save(std::ostream& s, int index, const std::map<const ElementMap*, int>& childMapSet,
              const std::map<QByteArray, int>& postfixMap) const;

    void save(std::ostream& s) const;

    ElementMapPtr restore(::App::StringHasherRef hasher, std::istream& s);

    ElementMapPtr restore(::App::StringHasherRef hasher, std::istream& s,
                          std::vector<ElementMapPtr>& childMaps,
                          const std::vector<std::string>& postfixes);

    MappedName addName(MappedName& name, const IndexedName& idx, const ElementIDRefs& sids,
                       bool overwrite, IndexedName* existing);

    bool erase(const MappedName& name);

    bool erase(const IndexedName& idx);

    IndexedName find(const MappedName& name, ElementIDRefs* sids = nullptr) const;

    MappedName find(const IndexedName& idx, ElementIDRefs* sids = nullptr) const;

    std::vector<std::pair<MappedName, ElementIDRefs>> findAll(const IndexedName& idx) const;

    // prefix searching is disabled, as TopoShape::getRelatedElement() is
    // deprecated in favor of GeoFeature::getRelatedElement(). Besides, there
    // is efficient way to support child element map if we were to implement
    // prefix search.
#if 0
    std::vector<MappedElement> findAllStartsWith(const char *prefix) const;
#endif

    unsigned long size() const;

    bool empty() const;

    bool hasChildElementMap() const;
    void hashChildMaps(ComplexGeoData& master);
    void addChildElements(ComplexGeoData& master, const std::vector<MappedChildElements>& children);

    std::vector<MappedChildElements> getChildElements() const;

    std::vector<MappedElement> getAll() const;

private:
    struct CStringComp
    {
    public:
        bool operator()(const char* str1, const char* str2) const
        {
            return std::strcmp(str1, str2) < 0;
        }
    };


    struct IndexedElements
    {
        std::deque<MappedNameRef> names;
        std::map<int, MappedChildElements> children;
    };

    std::map<const char*, IndexedElements, CStringComp> indexedNames;

    std::map<MappedName, IndexedName, std::less<MappedName>> mappedNames;


    struct ChildMapInfo
    {
        int index = 0;
        MappedChildElements* childMap = nullptr;
        std::map<ElementMap*, int> mapIndices;
    };

    QHash<QByteArray, ChildMapInfo> childElements;
    std::size_t childElementSize = 0;

    mutable unsigned _id = 0;
};


}// namespace Data

#endif// DATA_ELEMENTMAP_H
