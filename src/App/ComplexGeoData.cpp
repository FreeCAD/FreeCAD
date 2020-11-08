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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/io/ios_state.hpp>
#include <Base/Writer.h>
#include <Base/Reader.h>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <App/DynamicProperty.h>
#include "ComplexGeoData.h"
#include "MappedElement.h"

FC_LOG_LEVEL_INIT("ComplexGeoData", true,true);

namespace bio = boost::iostreams;
using namespace Data;

#ifdef _FC_MEM_TRACE

static int64_t _MemSize;
static int64_t _MemMaxSize;

struct MemUnit {
    int count;
    int maxcount;
};
static std::map<int, MemUnit> _MemUnits;

template<typename T>
struct MemoryMapAllocator : std::allocator<T> {
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::size_type size_type;
    template<typename U> struct rebind { typedef MemoryMapAllocator<U> other; };

    MemoryMapAllocator() {}

    template<typename U>
    MemoryMapAllocator(const MemoryMapAllocator<U>& u) : std::allocator<T>(u) {}

    pointer allocate(size_type size, std::allocator<void>::const_pointer = 0) {
        void* p = std::malloc(size * sizeof(T));
        if(p == 0)
            throw std::bad_alloc();
        _MemSize += size * sizeof(T);
        if (_MemSize > _MemMaxSize)
            _MemMaxSize = _MemSize;
        auto &unit = _MemUnits[sizeof(T)];
        if (++unit.count > unit.maxcount)
            unit.maxcount = unit.count;
        return static_cast<pointer>(p);
    }
    void deallocate(pointer p, size_type size) {
        _MemSize -= size * sizeof(T);
        --_MemUnits[sizeof(T)].count;
        std::free(p);
    }
};

#endif

namespace Data {

struct MappedNameRef
{
    MappedName name;
    ElementIDRefs sids;
    std::unique_ptr<MappedNameRef> next;

    MappedNameRef() {}

    MappedNameRef(const MappedName &name, const ElementIDRefs & sids = ElementIDRefs())
        :name(name), sids(sids)
    {
    }

    MappedNameRef(const MappedNameRef & other)
        :name(other.name), sids(other.sids)
    {
    }

    MappedNameRef(MappedNameRef && other)
        :name(std::move(other.name))
        ,sids(std::move(other.sids))
        ,next(std::move(other.next))
    {}

    MappedNameRef & operator=(MappedNameRef && other)
    {
        name = std::move(other.name);
        sids = std::move(other.sids);
        next = std::move(other.next);
        return *this;
    }

    explicit operator bool() const 
    { 
        return !name.empty();
    }

    void append(const MappedName &name, const ElementIDRefs sids = ElementIDRefs())
    {
        if (!name)
            return;
        if(!this->name) {
            this->name = name;
            this->sids = sids;
            compact();
            return;
        }
        std::unique_ptr<MappedNameRef> n(new MappedNameRef(name, sids));
        n->compact();
        if (!this->next)
            this->next = std::move(n);
        else {
            this->next.swap(n);
            this->next->next = std::move(n);
        }
    }

    void compact()
    {
        if (sids.size() > 1) {
            std::sort(sids.begin(), sids.end());
            sids.erase(std::unique(sids.begin(), sids.end()), sids.end());
        }
    }

    bool erase(const MappedName &name)
    {
        if (this->name == name) {
            this->name.clear();
            this->sids.clear();
            if (this->next) {
                this->name = std::move(this->next->name);
                this->sids = std::move(this->next->sids);
                std::unique_ptr<MappedNameRef> tmp;
                tmp.swap(this->next);
                this->next = std::move(tmp->next);
            }
            return  true;
        }

        for (std::unique_ptr<MappedNameRef> *p = &this->next; *p; p = &(*p)->next) {
            if ((*p)->name == name) {
                std::unique_ptr<MappedNameRef> tmp;
                tmp.swap(*p);
                *p = std::move(tmp->next);
                return true;
            }
        }
        return false;
    }

    void clear()
    {
        this->name.clear();
        this->sids.clear();
        this->next.reset();
    }
};

struct IndexedElements
{
    std::deque<MappedNameRef> names;
    std::map<int, MappedChildElements> children;
};

struct CStringComp
{
    bool operator()(const char *a, const char *b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

inline std::ostream & operator << (std::ostream &s, const QByteArray &bytes)
{
    s.write(bytes.constData(), bytes.size());
    return s;
}

class ElementMap {
public:
    ElementMap()
    {
    }

    void mark() const {
        for (auto & v : this->indexedNames) {
            for (const MappedNameRef & ref : v.second.names) {
                for (const MappedNameRef *r=&ref; r; r=r->next.get()) {
                    for (const App::StringIDRef & s : r->sids)
                        s.mark();
                }
            }
            for (auto & vv : v.second.children) {
                if (vv.second.elementMap)
                    vv.second.elementMap->mark();
            }
        }
    }

    const MappedNameRef * findMappedRef(const IndexedName & idx) const
    {
        auto iter = this->indexedNames.find(idx.getType());
        if (iter == this->indexedNames.end())
            return nullptr;
        auto & indices = iter->second;
        if (idx.getIndex() >= (int)indices.names.size())
            return nullptr;
        return &indices.names[idx.getIndex()];
    }

    MappedNameRef * findMappedRef(const IndexedName & idx)
    {
        auto iter = this->indexedNames.find(idx.getType());
        if (iter == this->indexedNames.end())
            return nullptr;
        auto & indices = iter->second;
        if (idx.getIndex() >= (int)indices.names.size())
            return nullptr;
        return &indices.names[idx.getIndex()];
    }

    MappedNameRef & mappedRef(const IndexedName & idx)
    {
        assert(idx);
        auto & indices = this->indexedNames[idx.getType()];
        if (idx.getIndex() >= (int)indices.names.size())
            indices.names.resize(idx.getIndex()+1);
        return indices.names[idx.getIndex()];
    }

    static void addPostfix(const QByteArray & postfix,
                          std::map<QByteArray, int> &postfixMap,
                          std::vector<QByteArray> &postfixes)
    {
        auto res = postfixMap.insert(std::make_pair(postfix, 0));
        if (res.second) {
            postfixes.push_back(postfix);
            res.first->second = (int)postfixes.size();
        }
    }

    void collectChildMaps(std::map<const ElementMap*, int> &childMapSet,
                          std::vector<const ElementMap*> &childMaps,
                          std::map<QByteArray, int> &postfixMap,
                          std::vector<QByteArray> &postfixes) const
    {
        auto res = childMapSet.insert(std::make_pair(this, 0));
        if (!res.second)
            return;

        for (auto & v : this->indexedNames) {
            addPostfix(QByteArray::fromRawData(v.first, qstrlen(v.first)), postfixMap, postfixes);

            for (auto & vv : v.second.children) {
                auto & child = vv.second;
                if (child.elementMap)
                    child.elementMap->collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);
            }
        }

        for (auto & v : this->mappedNames)
            addPostfix(v.first.constPostfix(), postfixMap, postfixes);

        childMaps.push_back(this);
        res.first->second = (int)childMaps.size();
    }

    void save(std::ostream &s,
              int index,
              const std::map<const ElementMap*,int> &childMapSet,
              const std::map<QByteArray, int> &postfixMap) const
    {
        s << "ElementMap " << index << ' ' << this->indexedNames.size() << '\n';

        for (auto & v : this->indexedNames) {
            s << v.first;
            if (v.second.children.size()) {
                s << " ChildCount " << v.second.children.size() << '\n';
                for (auto & vv : v.second.children) {
                    auto & child = vv.second;
                    int mapIndex = 0;
                    if (child.elementMap) {
                        auto it = childMapSet.find(child.elementMap.get());
                        if (it == childMapSet.end() || it->second == 0)
                            FC_ERR("Invalid child element map");
                        else
                            mapIndex = it->second;
                    }
                    s << child.indexedName.getIndex() << ' '
                      << child.offset << ' '
                      << child.count << ' '
                      << child.tag << ' '
                      << child.postfix << ' '
                      << mapIndex << '\n';
                }
            } else {
                s << " NameCount " << v.second.names.size() << '\n';
                boost::io::ios_flags_saver ifs(s);
                s << std::hex;
                for (auto & ref : v.second.names) {
                    for (auto r = &ref; r; r=r->next.get()) {
                        if (!r->name)
                            break;

                        App::StringID::IndexID prefixid;
                        prefixid.id = 0;
                        IndexedName idx(r->name.dataBytes());
                        bool printName = true;
                        if (idx) {
                            auto key = QByteArray::fromRawData(idx.getType(), qstrlen(idx.getType()));
                            auto it = postfixMap.find(key);
                            if (it != postfixMap.end()) {
                                s << ':' << it->second << '.' << idx.getIndex();
                                printName = false;
                            }
                        } else  {
                            prefixid = App::StringID::fromString(r->name.dataBytes());
                            if (prefixid.id) {
                                for (auto & sid : r->sids) {
                                    if (sid.value() == prefixid.id) {
                                        s << '$' << r->name.dataBytes();
                                        printName = false;
                                        break;
                                    }
                                }
                                if (printName)
                                    prefixid.id = 0;
                            }
                        }
                        if (printName)
                            s << ';' << r->name.dataBytes();

                        const QByteArray & postfix = r->name.postfixBytes();
                        if (postfix.isEmpty())
                            s << ".0";
                        else {
                            auto it = postfixMap.find(postfix);
                            assert(it != postfixMap.end());
                            s << '.' << it->second;
                        }
                        for (auto & sid : r->sids) {
                            if (sid.value() != prefixid.id)
                                s << '.' << sid.value();
                        }

                        s << ' ';
                    }
                    s << "0\n";
                }
            }
        }
    }

    void save(std::ostream &s) const {
        std::map<const ElementMap*, int> childMapSet;
        std::vector<const ElementMap*> childMaps;
        std::map<QByteArray, int> postfixMap;
        std::vector<QByteArray> postfixes;

        collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);

        s << "PostfixCount " << postfixes.size() << '\n';
        for (auto & p : postfixes)
            s << p << '\n';
        int index = 0;
        s << "MapCount " << childMaps.size() << '\n';
        for (auto & elementMap : childMaps)
            elementMap->save(s, ++index, childMapSet, postfixMap);
    }

    void restore(App::StringHasherRef hasher, std::istream &s)
    {
        const char * msg = "Invalid element map";
        int count = 0;
        std::string tmp;
        if (! (s >> tmp >> count) || tmp != "PostfixCount")
            FC_THROWM(Base::RuntimeError, msg);

        std::vector<std::string> postfixes;
        postfixes.reserve(count);
        for (int i=0; i < count; ++i) {
            postfixes.emplace_back();
            s >> postfixes.back();
        }

        std::vector<ElementMapPtr> childMaps;
        count = 0;
        if (! (s >> tmp >> count) || tmp != "MapCount" || count==0)
            FC_THROWM(Base::RuntimeError, msg);
        if (count > 1)
            count = count;
        childMaps.reserve(count);
        for (int i=0; i<count-1; ++i) {
            childMaps.push_back(std::make_shared<ElementMap>());
            childMaps.back()->restore(hasher, s, childMaps, postfixes);
        }
        restore(hasher, s, childMaps, postfixes);
    }

    void restore(App::StringHasherRef hasher,
                 std::istream &s,
                 std::vector<ElementMapPtr> &childMaps,
                 const std::vector<std::string> &postfixes)
    {
        const char * msg = "Invalid element map";
        std::string tmp;
        std::string tmp2;
        int index;
        int typeCount = 0;
        if (! (s >> tmp >> index >> typeCount) || tmp != "ElementMap")
            FC_THROWM(Base::RuntimeError, msg);

        const char *hasherWarn = nullptr;
        const char *hasherIDWarn = nullptr;
        const char *postfixWarn = nullptr;
        std::vector<std::string> tokens;

        for (int i=0; i<typeCount; ++i) {
            int count;
            if (! (s >> tmp >> tmp2 >> count))
                FC_THROWM(Base::RuntimeError, msg);
            IndexedName idx(tmp.c_str(), 1);
            auto & indices = this->indexedNames[idx.getType()];
            if (tmp2 == "ChildCount") {
                for (int j=0; j<count; ++j) {
                    int cindex;
                    int offset;
                    int count;
                    long tag;
                    int mapIndex;
                    if (! (s >> cindex >> offset >> count >> tag >> tmp >> mapIndex))
                        FC_THROWM(Base::RuntimeError, "Invalid element child");
                    if (cindex < 0)
                        FC_THROWM(Base::RuntimeError, "Invalid element child index");
                    if (offset < 0)
                        FC_THROWM(Base::RuntimeError, "Invalid element child offset");
                    if (mapIndex >= index || mapIndex < 0 || mapIndex > (int)childMaps.size())
                        FC_THROWM(Base::RuntimeError, "Invalid element child map index");
                    auto & child = indices.children[cindex+offset+count];
                    child.indexedName = IndexedName::fromConst(idx.getType(), cindex);
                    child.offset = offset;
                    child.count = count;
                    child.tag = tag;
                    if (mapIndex > 0)
                        child.elementMap = childMaps[mapIndex-1];
                    else
                        child.elementMap = nullptr;
                    child.postfix = tmp.c_str();
                    this->childElements.insert(child.postfix, &child);
                    this->childElementSize += child.count;
                }
            } else if (tmp2 == "NameCount") {
                indices.names.resize(count);
                for (int j=0; j<count; ++j) {
                    idx.setIndex(j);
                    auto * ref = & indices.names[j];
                    int k = 0;
                    while (1) {
                        if (! (s >> tmp))
                            FC_THROWM(Base::RuntimeError, "Failed to read element name");
                        if (tmp == "0")
                            break;
                        if (k++ != 0) {
                            ref->next.reset(new MappedNameRef);
                            ref = ref->next.get();
                        }
                        tokens.clear();
                        boost::split(tokens, tmp, boost::is_any_of("."));
                        if (tokens.size() < 2)
                            FC_THROWM(Base::RuntimeError, "Invalid element entry");

                        int offset = 1;
                        App::StringID::IndexID prefixid;
                        prefixid.id = 0;

                        switch(tokens[0][0]) {
                        case ':': {
                            if (tokens.size() < 3)
                                FC_THROWM(Base::RuntimeError, "Invalid element entry");
                            ++offset;
                            long n = strtol(tokens[0].c_str()+1, nullptr, 16);
                            if (n <= 0 || n > (int)postfixes.size())
                                FC_THROWM(Base::RuntimeError, "Invalid element name index");
                            long m = strtol(tokens[1].c_str(), nullptr, 16);
                            ref->name = MappedName(IndexedName::fromConst(postfixes[n-1].c_str(), m));
                            break;
                        }
                        case '$':
                            ref->name = MappedName(tokens[0].c_str()+1);
                            prefixid = App::StringID::fromString(ref->name.dataBytes());
                            break;
                        case ';':
                            ref->name = MappedName(tokens[0].c_str()+1);
                            break;
                        default:
                            FC_THROWM(Base::RuntimeError, "Invalid element name marker");
                        }

                        if (tokens[offset] != "0") {
                            long n = strtol(tokens[offset].c_str(), nullptr, 16);
                            if (n <= 0 || n > (int)postfixes.size())
                                postfixWarn = "Invalid element postfix index";
                            else
                                ref->name += postfixes[n-1];
                        }

                        this->mappedNames.emplace(ref->name, idx);

                        if (!hasher) {
                            if (offset + 1 < (int)tokens.size())
                                hasherWarn = "No hasher";
                            continue;
                        }

                        ref->sids.reserve(tokens.size()-offset-1 + prefixid.id?1:0);
                        if (prefixid.id) {
                            auto sid = hasher->getID(prefixid.id);
                            if (!sid)
                                hasherIDWarn = "Missing element name prefix id";
                            else
                                ref->sids.push_back(sid);
                        }
                        for (int l=offset+1; l<(int)tokens.size(); ++l) {
                            long id = strtol(tokens[l].c_str(), nullptr, 16);
                            auto sid = hasher->getID(id);
                            if (!sid)
                                hasherIDWarn = "Invalid element name string id";
                            else
                                ref->sids.push_back(sid);
                        }
                    }
                }
            }
            else 
                FC_THROWM(Base::RuntimeError, msg);
        }
        if (hasherWarn)
            FC_WARN(hasherWarn);
        if (hasherIDWarn)
            FC_WARN(hasherIDWarn);
        if (postfixWarn)
            FC_WARN(postfixWarn);
    }

    MappedName addName(MappedName & name,
                       const IndexedName & idx,
                       const ElementIDRefs &sids,
                       bool overwrite,
                       IndexedName * existing)
    {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            if (name.find("#") >= 0
                    && ComplexGeoData::findTagInElementName(name) < 0)
            {
                FC_ERR("missing tag postfix");
            }
        }
        do {
            auto ret = mappedNames.insert(std::make_pair(name, idx));
            if (ret.second) {
                ret.first->first.compact();
                mappedRef(idx).append(ret.first->first, sids);
                FC_TRACE(idx << " -> " << name);
                return ret.first->first;
            }
            if(ret.first->second == idx) {
                FC_TRACE("duplicate " << idx << " -> " << name);
                return ret.first->first;
            }
            if(!overwrite) {
                if (existing)
                    *existing = ret.first->second;
                return MappedName();
            }

            erase(ret.first->first);
        } while (true);
    }

    bool erase(const MappedName &name)
    {
        auto it = this->mappedNames.find(name);
        if (it == this->mappedNames.end())
            return false;
        MappedNameRef * ref = findMappedRef(it->second);
        if (!ref)
            return false;
        ref->erase(name);
        this->mappedNames.erase(it);
        return true;
    }

    bool erase(const IndexedName &idx)
    {
        auto iter = this->indexedNames.find(idx.getType());
        if (iter == this->indexedNames.end())
            return false;
        auto & indices = iter->second;
        if (idx.getIndex() >= (int)indices.names.size())
            return false;
        auto & ref = indices.names[idx.getIndex()];
        for (auto *r = &ref; r; r = r->next.get())
            this->mappedNames.erase(r->name);
        this->indexedNames.erase(iter);
        return true;
    }

    IndexedName find(const MappedName &name, ElementIDRefs * sids = nullptr) const
    {
        auto it = mappedNames.find(name);
        if (it == mappedNames.end()) {
            if (childElements.isEmpty())
                return IndexedName();

            int len = 0;
            if (ComplexGeoData::findTagInElementName(name,nullptr,&len) < 0)
                return IndexedName();
            QByteArray key = name.toRawBytes(len);
            auto it = this->childElements.find(key);
            if (it == this->childElements.end())
                return IndexedName();

            const auto & child = *it.value();
            IndexedName res;

            MappedName childName = MappedName::fromRawData(name, 0, len);
            if (child.elementMap)
                res = child.elementMap->find(childName, sids);
            else
                res = childName.toIndexedName();

            if (res && boost::equals(res.getType(), child.indexedName.getType())
                    && child.indexedName.getIndex() <= res.getIndex()
                    && child.indexedName.getIndex() + child.count > res.getIndex())
            {
                res.setIndex(res.getIndex() + it.value()->offset);
                return res;
            }

            return IndexedName();
        }

        if (sids) {
            const MappedNameRef * ref = findMappedRef(it->second);
            for (; ref; ref = ref->next.get()) {
                if (ref->name == name) {
                    if (!sids->size())
                        *sids = ref->sids;
                    else
                        *sids += ref->sids;
                    break;
                }
            }
        }
        return it->second;
    }

    MappedName find(const IndexedName &idx, ElementIDRefs * sids = nullptr) const
    {
        if (!idx)
            return MappedName();

        auto iter = this->indexedNames.find(idx.getType());
        if (iter == this->indexedNames.end())
            return MappedName();

        auto & indices = iter->second;
        if (idx.getIndex() < (int)indices.names.size()) {
            const MappedNameRef & ref = indices.names[idx.getIndex()];
            if (ref.name) {
                if (sids) {
                    if (!sids->size())
                        *sids = ref.sids;
                    else
                        *sids += ref.sids;
                }
                return ref.name;
            }
        }

        auto it = indices.children.upper_bound(idx.getIndex());
        if (it != indices.children.end()
                && it->second.indexedName.getIndex()+it->second.offset <= idx.getIndex())
        {
            auto & child = it->second;
            MappedName name;
            IndexedName childIdx(idx.getType(), idx.getIndex() - child.offset);
            if (child.elementMap)
                name = child.elementMap->find(childIdx, sids);
             else
                name = MappedName(childIdx);
            if (name) {
                name += child.postfix;
                return name;
            }
        }
        return MappedName();
    }

    std::vector<std::pair<MappedName, ElementIDRefs> >
    findAll(const IndexedName &idx) const
    {
        std::vector<std::pair<MappedName, ElementIDRefs> > res;
        if (!idx)
            return res;

        auto iter = this->indexedNames.find(idx.getType());
        if (iter == this->indexedNames.end())
            return res;

        auto & indices = iter->second;
        if (idx.getIndex() < (int)indices.names.size()) {
            const MappedNameRef & ref = indices.names[idx.getIndex()];
            int count = 0;
            for (auto r = &ref; r; r = r->next.get()) {
                if (r->name)
                    ++count;
            }
            if (count) {
                res.reserve(count);
                for (auto r = &ref; r; r = r->next.get())
                    res.emplace_back(r->name, r->sids);
                return res;
            }
        }

        auto it = indices.children.upper_bound(idx.getIndex());
        if (it != indices.children.end()
                && it->second.indexedName.getIndex()+it->second.offset <= idx.getIndex())
        {
            auto & child = it->second;
            IndexedName childIdx(idx.getType(), idx.getIndex() - child.offset);
            if (child.elementMap) {
                res = child.elementMap->findAll(childIdx);
                for (auto &v : res)
                    v.first += child.postfix;
            } else 
                res.emplace_back(MappedName(childIdx) + child.postfix, ElementIDRefs());
        }

        return res;
    }

    // prefix searching is disabled, as TopoShape::getRelatedElement() is
    // deprecated in favor of GeoFeature::getRelatedElement(). Besides, there
    // is efficient way to support child element map if we were to implement
    // prefix search.
#if 0
    std::vector<MappedElement>
    findAllStartsWith(const char *prefix) const
    {
        std::vector<MappedElement> res;
        MappedName mapped(prefix);
        for(auto it=mappedNames.lower_bound(mapped);it!=mappedNames.end();++it) {
            if(it->first.startsWith(prefix))
                res.emplace_back(it->first, it->second);
        }
        return res;
    }
#endif

    unsigned long size() const
    {
        return mappedNames.size() + childElementSize;
    }

    bool empty() const
    {
        return mappedNames.empty() && childElementSize == 0;
    }

    struct ChildKey {
        const MappedChildElements & item;
        ChildKey(const MappedChildElements & item)
            : item(item)
        {}

        bool operator < (const ChildKey &other) const
        {
            if (item.indexedName[0] < other.item.indexedName[0])
                return true;
            if (item.indexedName[0] > other.item.indexedName[0])
                return false;
            if (item.tag < other.item.tag)
                return true;
            if (item.tag > other.item.tag)
                return false;
            return item.postfix < other.item.postfix;
        }
    };

    struct ChildValue {
        int index = 0;
        std::map<ElementMap*, int> mapIndices;
    };

    void setChildElements(ComplexGeoData & master, const std::vector<MappedChildElements> &children)
    {
        assert(empty());

        std::map<ChildKey, ChildValue> indexMap;

        std::ostringstream ss;
        ss << std::hex;
        for (auto & child : children) {
            if (!child.indexedName || !child.count) {
                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                    FC_ERR("invalid mapped child element");
                continue;
            }

            ss.str("");

            // Perform some disambiguation in case the same shape is mapped
            // multiple times, e.g. draft array.
            auto & entry = indexMap[child];
            int mapIndex = entry.mapIndices[child.elementMap.get()]++;
            ++entry.index;
            if (entry.index != 1) {
                if (child.elementMap && mapIndex == 0) {
                    // This child has duplicated 'tag' and 'postfix', but it
                    // has its own element map. We'll expand this map now.

                    IndexedName childIdx(child.indexedName);
                    IndexedName idx(childIdx.getType(), childIdx.getIndex()+child.offset);
                    for (int i=0; i<child.count; ++i, ++childIdx, ++idx) {
                        ElementIDRefs sids;
                        MappedName name = child.elementMap->find(childIdx, &sids);
                        if (!name) {
                            if (!child.tag || child.tag == master.Tag) {
                                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                                    FC_WARN("unmapped element");
                                continue;
                            }
                            name = MappedName(childIdx);
                        }
                        ss.str("");
                        master.encodeElementName(idx[0], name, ss, &sids,
                                child.postfix.constData(), child.tag);
                        master.setElementName(idx, name, &sids);
                    }
                    continue;
                }

                // NOTE: We are not using ComplexGeoData::indexPostfix() to not
                // confuse other code that actually uses this postfix for indexing
                // purposes. Here, we just need some postfix for disambiguation. We
                // don't need to extract the index.
                ss.str("");
                ss << ComplexGeoData::elementMapPrefix() << ":C" << entry.index-1;
            }

            auto & indices = this->indexedNames[child.indexedName.getType()];
            auto res = indices.children.emplace(
                    child.indexedName.getIndex() + child.offset + child.count, child);
            if (!res.second) {
                FC_ERR("duplicate mapped child element");
                continue;
            }

            MappedName tmp;
            master.encodeElementName(child.indexedName[0],
                    tmp, ss, nullptr, child.postfix.constData(), child.tag, true);

            auto & insertedChild = res.first->second;
            insertedChild.postfix = tmp.toBytes();

            if (this->childElements.contains(insertedChild.postfix))
                FC_ERR("duplicate mapped child element postfix: "
                        << insertedChild.postfix);
            else
                this->childElements.insert(insertedChild.postfix, &insertedChild);

            childElementSize += insertedChild.count;
        }
    }

    std::vector<MappedChildElements> getChildElements() const
    {
        std::vector<MappedChildElements> res;
        res.reserve(this->childElements.size());
        for (auto & v : this->childElements)
            res.push_back(*v);
        return res;
    }

    std::vector<MappedElement> getAll() const {
        std::vector<MappedElement> ret;
        ret.reserve(size());
        for (auto &v : this->mappedNames)
            ret.emplace_back(v.first, v.second);
        for (auto &v : this->childElements) {
            auto & child = *v;
            IndexedName idx(child.indexedName);
            idx.setIndex(idx.getIndex() + child.offset);
            IndexedName childIdx(child.indexedName);
            for (int i=0; i<child.count; ++i, ++idx, ++childIdx) {
                MappedName name;
                if (child.elementMap)
                    name = child.elementMap->find(childIdx);
                else 
                    name = MappedName(childIdx);
                if (name) {
                    name += child.postfix;
                    ret.emplace_back(name, idx);
                }
            }
        }
        return ret;
    }

private:
    std::map<const char *, IndexedElements, CStringComp> indexedNames;

    std::map<MappedName
             ,IndexedName
             ,std::less<MappedName>
#ifdef _FC_MEM_TRACE
             ,MemoryMapAllocator<std::pair<MappedName, IndexedName> >
#endif
            > mappedNames;

    QHash<QByteArray, MappedChildElements *> childElements;

    std::size_t childElementSize = 0;
};

}

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment , Base::BaseClass)


TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData , Base::Persistence)


ComplexGeoData::ComplexGeoData(void)
    :Tag(0)
{
}

ComplexGeoData::~ComplexGeoData(void)
{
}

Data::Segment* ComplexGeoData::getSubElementByName(const char* name) const
{
    int index = 0;
    std::string element(name);
    std::string::size_type pos = element.find_first_of("0123456789");
    if (pos != std::string::npos) {
        index = std::atoi(element.substr(pos).c_str());
        element = element.substr(0,pos);
    }

    return getSubElement(element.c_str(),index);
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

void ComplexGeoData::getLinesFromSubelement(const Segment*,
                                            std::vector<Base::Vector3d> &Points,
                                            std::vector<Line> &lines) const
{
    (void)Points;
    (void)lines;
}

void ComplexGeoData::getFacesFromSubelement(const Segment*,
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
                               float Accuracy, uint16_t flags) const
{
    (void)Points;
    (void)Normals;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getLines(std::vector<Base::Vector3d> &Points,
                              std::vector<Line> &lines,
                              float Accuracy, uint16_t flags) const
{
    (void)Points;
    (void)lines;
    (void)Accuracy;
    (void)flags;
}

void ComplexGeoData::getFaces(std::vector<Base::Vector3d> &Points,
                              std::vector<Facet> &faces,
                              float Accuracy, uint16_t flags) const
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

const std::string &ComplexGeoData::elementMapPrefix() {
    static std::string prefix(";");
    return prefix;
}

const char *ComplexGeoData::isMappedElement(const char *name) {
    if(name && boost::starts_with(name,elementMapPrefix()))
        return name+elementMapPrefix().size();
    return 0;
}

std::string ComplexGeoData::getElementMapVersion() const {
    return "4";
}

bool ComplexGeoData::checkElementMapVersion(const char * ver) const
{
    return !boost::equals(ver, "3")
        && !boost::equals(ver, "4")
        && !boost::starts_with(ver, "3.");
}

std::string ComplexGeoData::newElementName(const char *name) {
    if(!name) return std::string();
    const char *dot = strrchr(name,'.');
    if(!dot || dot==name) return name;
    const char *c = dot-1;
    for(;c!=name;--c) {
        if(*c == '.') {
            ++c;
            break;
        }
    }
    if(isMappedElement(c))
        return std::string(name,dot-name);
    return name;
}

std::string ComplexGeoData::oldElementName(const char *name) {
    if(!name) return std::string();
    const char *dot = strrchr(name,'.');
    if(!dot || dot==name) return name;
    const char *c = dot-1;
    for(;c!=name;--c) {
        if(*c == '.') {
            ++c;
            break;
        }
    }
    if(isMappedElement(c))
        return std::string(name,c-name)+(dot+1);
    return name;
}

std::string ComplexGeoData::noElementName(const char *name) {
    if(!name) return std::string();
    auto element = findElementName(name);
    if(element)
        return std::string(name,element-name);
    return name;
}

const char *ComplexGeoData::findElementName(const char *subname) {
    if(!subname || !subname[0] || isMappedElement(subname))
        return subname;
    const char *dot = strrchr(subname,'.');
    if(!dot)
        return subname;
    const char *element = dot+1;
    if(dot==subname || isMappedElement(element))
        return element;
    for(--dot;dot!=subname;--dot) {
        if(*dot == '.') {
            ++dot;
            break;
        }
    }
    if(isMappedElement(dot))
        return dot;
    return element;
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
    return _ElementMap?_ElementMap->size():0;
}

MappedName ComplexGeoData::getMappedName(const IndexedName & element,
                                         bool allowUnmapped,
                                         ElementIDRefs *sid) const 
{
    if (!element)
        return MappedName();
    flushElementMap();
    if(!_ElementMap) {
        if (allowUnmapped)
            return MappedName(element);
        return MappedName();
    }

    MappedName name = _ElementMap->find(element, sid);
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
    if (!_ElementMap) {
        std::string s;
        return IndexedName(name.toString(s), getElementTypes());
    }
    return _ElementMap->find(name, sid);
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
        res.name = MappedName::fromRawData(name);
    res.index = getIndexedName(res.name, sid);
    return res;
}

std::vector<std::pair<MappedName, ElementIDRefs> >
ComplexGeoData::getElementMappedNames(const IndexedName & element, bool needUnmapped) const {
    flushElementMap();
    if(_ElementMap) {
        auto res = _ElementMap->findAll(element);
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
    if(!prefix || !prefix[0] || !_ElementMap)
        return names;
    const auto &p = elementMapPrefix();
    if(boost::starts_with(prefix,p))
        prefix += p.size();
    names = _ElementMap->findAllStartsWith(prefix);
    return names;
#else
    (void)prefix;
    return {};
#endif
}

std::vector<MappedElement> ComplexGeoData::getElementMap() const {
    flushElementMap();
    if(!_ElementMap)
        return {};
    return _ElementMap->getAll();
}

ElementMapPtr ComplexGeoData::elementMap(bool flush) const
{
    if (flush)
        flushElementMap();
    return _ElementMap;
}

void ComplexGeoData::flushElementMap() const
{
}

void ComplexGeoData::setElementMap(const std::vector<MappedElement> &map) {
    resetElementMap();
    for(auto &v : map)
        setElementName(v.index, v.name);
}

MappedName ComplexGeoData::hashElementName(
        const MappedName & name, ElementIDRefs &sids) const
{
    if(!this->Hasher || !name)
        return name;
    if (name.find(elementMapPrefix()) < 0)
        return name;
    App::StringIDRef sid = this->Hasher->getID(name, sids);
    sids.clear();
    sids.push_back(sid);
    return MappedName(sid.toString());
}

MappedName ComplexGeoData::dehashElementName(const MappedName & name) const {
    if(name.empty())
        return name;
    if(!Hasher)
        return name;
    auto id = App::StringID::fromString(name.toRawBytes());
    if(!id)
        return name;
    auto sid = Hasher->getID(id);
    if(!sid) {
        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE))
            FC_WARN("failed to find hash id " << id);
        else
            FC_LOG("failed to find hash id " << id);
        return name;
    }
    if(sid.isHashed()) {
        FC_LOG("cannot dehash id " << id);
        return name;
    }
    MappedName ret(sid);
    FC_TRACE("dehash " << name << " -> " << ret);
    return ret;
}

MappedName ComplexGeoData::setElementName(const IndexedName & element,
                                          const MappedName & name, 
                                          const ElementIDRefs *sid,
                                          bool overwrite)
{
    if(!element)
        throw Base::ValueError("Invalid input");
    if(!name)  {
        if(_ElementMap)
            _ElementMap->erase(element);
        return MappedName();
    }

    for(int i=0, count=name.size(); i<count; ++i) {
        char c = name[i];
        if(c == '.' || std::isspace((int)c))
            FC_THROWM(Base::RuntimeError,"Illegal character in mapped name: " << name);
    }
    for(const char *s=element.getType();*s;++s) {
        char c = *s;
        if(c == '.' || std::isspace((int)c))
            FC_THROWM(Base::RuntimeError,"Illegal character in element name: " << element);
    }

    if(!_ElementMap)
        resetElementMap(std::make_shared<ElementMap>());

    ElementIDRefs _sid;
    if (!sid)
        sid = &_sid;

    std::ostringstream ss;
    Data::MappedName n(name);
    for(int i=0;;) {
        IndexedName existing;
        MappedName res = _ElementMap->addName(n, element, *sid, overwrite, &existing);
        if (res)
            return res;
        if (++i == 100) {
            FC_ERR("unresolved duplicate element mapping '" << name 
                    <<' ' << element << '/' << existing);
            return name;
        }
        if(sid != &_sid)
            _sid = *sid;
        n = renameDuplicateElement(i,element,existing,name,_sid);
        if (!n)
            return name;
        sid = &_sid;
    }
    
}

char ComplexGeoData::elementType(const Data::MappedName &name) const
{
    if(!name)
        return 0;
    char element_type=0;
    findTagInElementName(name,0,0,0,&element_type);
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
    ss << elementMapPrefix() << 'D' << std::hex << index;
    MappedName renamed(name);
    encodeElementName(element.getType()[0],renamed,ss,&sids);
    FC_TRACE("duplicate element mapping '" << name << " -> " << renamed << ' ' 
            << element << '/' << element2);
    return renamed;
}

const std::string &ComplexGeoData::tagPostfix() {
    static std::string postfix(elementMapPrefix() + ":H");
    return postfix;
}

const std::string &ComplexGeoData::decTagPostfix() {
    static std::string postfix(elementMapPrefix() + ":T");
    return postfix;
}

const std::string &ComplexGeoData::indexPostfix() {
    static std::string postfix(elementMapPrefix() + ":I");
    return postfix;
}

const std::string &ComplexGeoData::missingPrefix() {
    static std::string prefix("?");
    return prefix;
}

bool ComplexGeoData::hasMissingElement(const char *subname) {
    if(!subname)
        return false;
    auto dot = strrchr(subname,'.');
    if(dot)
        subname = dot+1;
    return boost::starts_with(subname,missingPrefix());
}

int ComplexGeoData::findTagInElementName(const MappedName & name, 
                                         long *tag,
                                         int *len,
                                         std::string *postfix,
                                         char *type,
                                         bool negative) 
{
    bool hex = true;
    int pos = name.rfind(tagPostfix());
    if(pos < 0) {
        pos = name.rfind(decTagPostfix());
        if (pos < 0)
            return -1;
        hex = false;
    }
    int offset = pos + (int)tagPostfix().size();
    long _tag = 0;
    int _len = 0;
    char sep = 0;
    char sep2 = 0;
    char tp = 0;
    char eof = 0;

    int size;
    const char *s = name.toConstString(offset, size);

    bool isNegative = (s[0] == '-');
    if (isNegative) {
        ++s;
        --size;
    }
    bio::stream<bio::array_source> iss(s, size);
    if (!hex)
        iss >> _tag >> sep;
    else {
        iss >> std::hex;
        // _tag field can be skipped, if it is 0
        if (s[0] == ',' || s[0] == ':')
            iss >> sep;
        else
            iss >> _tag >> sep;
    }

    if (isNegative)
        _tag = -_tag;

    if (sep == ':') {
        // ':' is followed by _len field.
        //
        // For decTagPostfix(), this is the length of the string before the
        // entire postfix (A postfix may contain multiple segments usually
        // separated by elementPrefix().
        //
        // For newer tagPostfix(), this counts the number of characters that proceeds
        // this tag postfix segment that forms the entire postfix.
        //
        // For example, Face2;FUS;M2;H:6,F, the length field :6 counts the string ;FUS;M2,
        // The entire postfix is thus ;FUS;M2;H:6,F
        //
        // The reason of this change is so that the postfix can stay the same
        // regardless of the prefix, which is can increase memory efficiency.
        //
        iss >> _len >> sep2 >> tp >> eof;

        // The next separator to look for is either ':' for older tag postfix, or ','
        if (!hex && sep2 == ':')
            sep2 = ',';
    }
    else if (hex && sep == ',') {
        // ',' is followed by a single character that indicates the element type.
        iss >> tp >> eof;
        sep = ':';
        sep2 = ',';
    }

    if (_len < 0 || sep != ':' || sep2 != ',' || tp == 0 || eof != 0)
        return -1;

    if (hex) {
        if (pos-_len <= 0)
           return -1; 
        _len = pos - _len;
    }
    if(type)
        *type = tp;
    if(tag) {
        if (_tag == 0)
            return findTagInElementName(
                        MappedName(name, 0, _len), tag, len, postfix, type, negative);
        if(_tag>0 || negative)
            *tag = _tag;
        else
            *tag = -_tag;
    }
    if(len)
        *len = _len;
    if(postfix)
        name.toString(*postfix, pos);
    return pos;
}

// try to hash element name while preserving the source tag
void ComplexGeoData::encodeElementName(char element_type,
                                       MappedName &name,
                                       std::ostringstream &ss,
                                       ElementIDRefs *sids,
                                       const char* postfix,
                                       long tag,
                                       bool forceTag) const
{
    if(postfix && postfix[0]) {
        if (!boost::starts_with(postfix, elementMapPrefix()))
            ss << elementMapPrefix();
        ss << postfix;
    }
    long inputTag = 0;
    if (!forceTag && !ss.tellp()) {
        if(!tag || tag==Tag)
            return;
        findTagInElementName(name,&inputTag,nullptr,nullptr,nullptr,true);
        if(inputTag == tag)
            return;
    }
    else if (forceTag || !tag || tag==Tag) {
        int pos = findTagInElementName(name,&inputTag,nullptr,nullptr,nullptr,true);
        if(inputTag) {
            tag = inputTag;
            // About to encode the same tag used last time. This usually means
            // the owner object is doing multi step modeling. Let's not
            // recursively encode the same tag too many time. It will be a
            // waste of memory, because the intermediate shapes has no
            // corresponding objects, so no real value for history tracing.
            //
            // On the other hand, we still need to distingush the original name
            // from the input object from the element name of the intermediate
            // shapes. So we limit ourselves to encode only one extra level
            // using the same tag. In order to do that, we need to dehash the
            // previous level name, and check for its tag.
            Data::MappedName n(name, 0, pos);
            Data::MappedName prev = dehashElementName(n);
            long prevTag = 0;
            findTagInElementName(prev,&prevTag,nullptr,nullptr,nullptr,true);
            if (prevTag == inputTag || prevTag == -inputTag)
                name = n;
        }
    }
    if(sids && Hasher)
        name = hashElementName(name, *sids);
    if(forceTag || tag) {
        assert(element_type);
        int pos = ss.tellp();
        boost::io::ios_flags_saver ifs(ss);
        ss << tagPostfix() << std::hex;
        if (tag < 0)
            ss << '-' << -tag;
        else if (tag)
            ss << tag;
        assert(pos >= 0);
        if (pos != 0)
            ss << ':' << pos;
        ss << ',' << element_type;
    }
    name += ss.str();
}

long ComplexGeoData::getElementHistory(const char *name, 
                                       MappedName *original,
                                       std::vector<MappedName> *history) const 
{
    MappedElement mapped = getElementName(name);
    if (!mapped.name)
        return 0;
    return getElementHistory(name, original, history);
}

long ComplexGeoData::getElementHistory(const MappedName & name, 
                                       MappedName *original,
                                       std::vector<MappedName> *history) const 
{
    long tag = 0;
    int len = 0;
    int pos = findTagInElementName(name,&tag,&len,nullptr,nullptr,true);
    if(pos < 0) {
        if(original)
            *original = name;
        return tag;
    }
    if(!original && !history)
        return tag;

    MappedName tmp;
    MappedName &ret = original?*original:tmp;
    bool first = true;
    while(1) {
        if(!len || len>pos) {
            FC_WARN("invalid name length " << name);
            return 0;
        }
        if(first) {
            first = false;
            size_t offset = 0;
            if(name.startsWith(elementMapPrefix()))
                offset = elementMapPrefix().size();
            ret = MappedName(name, offset, len);
        }else
            ret = MappedName(ret, 0, len);
        ret = dehashElementName(ret);
        long tag2 = 0;
        pos = findTagInElementName(ret,&tag2,&len,nullptr,nullptr,true);
        if(pos < 0 || (tag2!=tag && tag!=Tag && -tag!=Tag))
            return tag;
        tag = tag2;
        if(history)
            history->push_back(ret);
    }
}

void ComplexGeoData::setPersistenceFileName(const char *filename) const {
    if(!filename)
        filename = "";
    _PersistenceName = filename;
}

void ComplexGeoData::Save(Base::Writer &writer) const {
    writer.Stream() << writer.ind() << "<ElementMap new=\"1\"/>\n";
    // New layout of element map, so we use new xml tag, ElementMap2
    writer.Stream() << writer.ind() << "<ElementMap2";
    flushElementMap();
    if(!_ElementMap || _ElementMap->empty()) {
        writer.Stream() << "/>\n";
        return;
    }
    if(_PersistenceName.size()) {
        writer.Stream() << " file=\"" 
            << writer.addFile(_PersistenceName+".txt",this) 
            << "\"/>\n";
        return;
    }
    writer.Stream() << " count=\"" << _ElementMap->size() << "\">\n";
    _ElementMap->save(writer.beginCharStream(false) << '\n');
    writer.endCharStream() << '\n';
    writer.Stream() << writer.ind() << "</ElementMap2>\n" ;
}

void ComplexGeoData::Restore(Base::XMLReader &reader) {
    resetElementMap();

    reader.readElement("ElementMap");
    bool newtag = false;
    if (reader.getAttributeAsInteger("new","0") > 0) {
        reader.readElement("ElementMap2");
        newtag = true;
    }

    const char *file = reader.getAttribute("file","");
    if(*file) {
        reader.addFile(file,this);
        return;
    }
    
    std::size_t count = reader.getAttributeAsUnsigned("count","");
    if(!count)
        return;

    if (newtag) {
        resetElementMap(std::make_shared<ElementMap>());
        _ElementMap->restore(Hasher, reader.beginCharStream(false));
        reader.endCharStream();
        reader.readEndElement("ElementMap2");
        return;
    }

    if(reader.FileVersion>1) {
        restoreStream(reader.beginCharStream(false), count);
        reader.endCharStream();
        return;
    }

    size_t invalid_count = 0;
    bool warned = false;

    const auto & types = getElementTypes();

    for(size_t i=0;i<count;++i) {
        reader.readElement("Element");
        ElementIDRefs sids;
        if(reader.hasAttribute("sid")) {
            if(!Hasher) {
                if(!warned) {
                    warned = true;
                    FC_ERR("missing hasher");
                }
            } else {
                const char *attr = reader.getAttribute("sid");
                bio::stream<bio::array_source> iss(attr, std::strlen(attr));
                long id;
                while((iss >> id)) {
                    if (id == 0)
                        continue;
                    auto sid = Hasher->getID(id);
                    if(!sid) 
                        ++invalid_count;
                    else
                        sids.push_back(sid);
                    char sep;
                    iss >> sep;
                }
            }
        }
        setElementName(IndexedName(reader.getAttribute("value"), types),
                    MappedName(reader.getAttribute("key")),
                    &sids);
    }
    if(invalid_count)
        FC_ERR("Found " << invalid_count << " invalid string id");
    reader.readEndElement("ElementMap");
}

void ComplexGeoData::restoreStream(std::istream &s, std::size_t count) { 
    resetElementMap();

    size_t invalid_count = 0;
    std::string key,value,sid;
    bool warned = false;

    const auto & types = getElementTypes();

    for(size_t i=0;i<count;++i) {
        ElementIDRefs sids;
        std::size_t scount;
        if(!(s >> value >> key >> scount))
            throw Base::RuntimeError("Failed to restore element map");
        sids.reserve(scount);
        for(std::size_t j=0;j<scount;++j) {
            long id;
            if(!(s >> id))
                throw Base::RuntimeError("Failed to restore element map");
            if (Hasher) {
                auto sid = Hasher->getID(id);
                if(!sid) 
                    ++invalid_count;
                else
                    sids.push_back(sid);
            }
        }
        if(scount && !Hasher) {
            if(!warned) {
                warned = true;
                FC_ERR("missing hasher");
            }
        }
        setElementName(IndexedName(value.c_str(), types), MappedName(key), &sids);
    }
    if(invalid_count)
        FC_ERR("Found " << invalid_count << " invalid string id");
}

void ComplexGeoData::SaveDocFile(Base::Writer &writer) const {
    flushElementMap();
    if (_ElementMap) {
        writer.Stream() << "ElementMapStart v1\n";
        _ElementMap->save(writer.Stream());
        writer.Stream() << "ElementMapEnd\n";
    }
}

void ComplexGeoData::RestoreDocFile(Base::Reader &reader) {
    std::string marker, ver;
    reader >> marker;
    if (boost::equals(marker, "ElementMapStart")) {
        resetElementMap();
        reader >> ver;
        if (ver != "v1")
            FC_WARN("Unknown element map format");
        else {
            resetElementMap(std::make_shared<ElementMap>());
            _ElementMap->restore(Hasher, reader);
            return;
        }
    }
    std::size_t count = atoi(marker.c_str());
    restoreStream(reader,count);
}

unsigned int ComplexGeoData::getMemSize(void) const {
    flushElementMap();
    if(_ElementMap)
        return _ElementMap->size()*10;
    return 0;
}

std::vector<IndexedName> ComplexGeoData::getHigherElements(const char *, bool) const
{
    return {};
}

void ComplexGeoData::traceElement(const MappedName &name, TraceCallback cb) const
{
    long tag = this->Tag, encodedTag = 0;
    int len = 0;
    auto pos = findTagInElementName(name,&tag,&len,nullptr,nullptr,true);
    if(cb(name, len, encodedTag, tag) || pos < 0)
        return;

    tag = encodedTag;
    MappedName tmp;
    bool first = true;
    while(1) {
        if(!len || len>pos)
            return;
        if(first) {
            first = false;
            size_t offset = 0;
            if(name.startsWith(elementMapPrefix()))
                offset = elementMapPrefix().size();
            tmp = MappedName(name, offset, len);
        }else
            tmp = MappedName(tmp, 0, len);
        tmp = dehashElementName(tmp);
        encodedTag = 0;
        pos = findTagInElementName(tmp,&encodedTag,&len,nullptr,nullptr,true);
        if(cb(tmp, len, encodedTag, tag) || pos < 0)
            return;
        tag = encodedTag;
    }
}

void ComplexGeoData::setMappedChildElements(const std::vector<MappedChildElements> & children)
{
    resetElementMap(std::make_shared<ElementMap>());
    _ElementMap->setChildElements(*this, children);
}

std::vector<MappedChildElements> ComplexGeoData::getMappedChildElements() const
{
    if (!_ElementMap)
        return {};
    return _ElementMap->getChildElements();
}

void ComplexGeoData::beforeSave() const
{
    flushElementMap();
    if (this->_ElementMap)
        this->_ElementMap->mark();
}
