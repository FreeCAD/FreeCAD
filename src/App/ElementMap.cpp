
#include "ElementMap.h"
#include "PostfixStringReferences.h"

#include "App/Application.h"
#include "Base/Console.h"

// #include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <unordered_map>


FC_LOG_LEVEL_INIT("ElementMap", true, 2);

namespace Data
{


// Because the existence of hierarchical element maps, for the same document
// we may store an element map more than once in multiple objects. And because
// we may want to support partial loading, we choose to tolerate such redundancy
// for now.
//
// In order to not waste memory space when the file is loaded, we use the
// following two maps to assign a one-time id for each unique element map.  The
// id will be saved together with the element map.
//
// When restoring, we'll read back the id and lookup for an existing element map
// with the same id, and skip loading the current map if one is found.
//
// TODO: Note that the same redundancy can be found when saving OCC shapes,
// because we currently save shapes for each object separately. After restoring,
// any shape sharing is lost. But again, we do want to keep separate shape files
// because of partial loading. The same technique used here can be applied to
// restore shape sharing.
static std::unordered_map<const ElementMap*, unsigned> _ElementMapToId;
static std::unordered_map<unsigned, ElementMapPtr> _IdToElementMap;


ElementMap::ElementMap()
{
    static bool inited;
    if (!inited) {
        inited = true;
        ::App::GetApplication().signalStartSaveDocument.connect(
            [](const ::App::Document&, const std::string&) {
                _ElementMapToId.clear();
            });
        ::App::GetApplication().signalFinishSaveDocument.connect(
            [](const ::App::Document&, const std::string&) {
                _ElementMapToId.clear();
            });
        ::App::GetApplication().signalStartRestoreDocument.connect([](const ::App::Document&) {
            _IdToElementMap.clear();
        });
        ::App::GetApplication().signalFinishRestoreDocument.connect([](const ::App::Document&) {
            _IdToElementMap.clear();
        });
    }
}

void ElementMap::beforeSave(const ::App::StringHasherRef& hasher) const
{
    unsigned& id = _ElementMapToId[this];
    if (!id)
        id = _ElementMapToId.size();
    this->_id = id;

    for (auto& v : this->indexedNames) {
        for (const MappedNameRef& ref : v.second.names) {
            for (const MappedNameRef* r = &ref; r; r = r->next.get()) {
                for (const ::App::StringIDRef& sid : r->sids) {
                    if (sid.isFromSameHasher(hasher))
                        sid.mark();
                }
            }
        }
        for (auto& vv : v.second.children) {
            if (vv.second.elementMap)
                vv.second.elementMap->beforeSave(hasher);
            for (auto& sid : vv.second.sids) {
                if (sid.isFromSameHasher(hasher))
                    sid.mark();
            }
        }
    }
}

void ElementMap::save(std::ostream& s, int index,
                      const std::map<const ElementMap*, int>& childMapSet,
                      const std::map<QByteArray, int>& postfixMap) const
{
    s << "\nElementMap " << index << ' ' << this->_id << ' ' << this->indexedNames.size() << '\n';

    for (auto& v : this->indexedNames) {
        s << '\n' << v.first << '\n';

        s << "\nChildCount " << v.second.children.size() << '\n';
        for (auto& vv : v.second.children) {
            auto& child = vv.second;
            int mapIndex = 0;
            if (child.elementMap) {
                auto it = childMapSet.find(child.elementMap.get());
                if (it == childMapSet.end() || it->second == 0)
                    FC_ERR("Invalid child element map");
                else
                    mapIndex = it->second;
            }
            s << child.indexedName.getIndex() << ' ' << child.offset << ' ' << child.count << ' '
              << child.tag << ' ' << mapIndex << ' ';
            s.write(child.postfix.constData(), child.postfix.size());
            s << ' ' << '0';
            for (auto& sid : child.sids) {
                if (sid.isMarked())
                    s << '.' << sid.value();
            }
            s << '\n';
        }

        s << "\nNameCount " << v.second.names.size() << '\n';
        if (v.second.names.empty())
            continue;

        boost::io::ios_flags_saver ifs(s);
        s << std::hex;

        for (auto& ref : v.second.names) {
            for (auto r = &ref; r; r = r->next.get()) {
                if (!r->name)
                    break;

                ::App::StringID::IndexID prefixid;
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
                }
                else {
                    prefixid = ::App::StringID::fromString(r->name.dataBytes());
                    if (prefixid.id) {
                        for (auto& sid : r->sids) {
                            if (sid.isMarked() && sid.value() == prefixid.id) {
                                s << '$';
                                s.write(r->name.dataBytes().constData(),
                                        r->name.dataBytes().size());
                                printName = false;
                                break;
                            }
                        }
                        if (printName)
                            prefixid.id = 0;
                    }
                }
                if (printName) {
                    s << ';';
                    s.write(r->name.dataBytes().constData(), r->name.dataBytes().size());
                }

                const QByteArray& postfix = r->name.postfixBytes();
                if (postfix.isEmpty())
                    s << ".0";
                else {
                    auto it = postfixMap.find(postfix);
                    assert(it != postfixMap.end());
                    s << '.' << it->second;
                }
                for (auto& sid : r->sids) {
                    if (sid.isMarked() && sid.value() != prefixid.id)
                        s << '.' << sid.value();
                }

                s << ' ';
            }
            s << "0\n";
        }
    }
    s << "\nEndMap\n";
}

void ElementMap::save(std::ostream& s) const
{
    std::map<const ElementMap*, int> childMapSet;
    std::vector<const ElementMap*> childMaps;
    std::map<QByteArray, int> postfixMap;
    std::vector<QByteArray> postfixes;

    collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);

    s << this->_id << " PostfixCount " << postfixes.size() << '\n';
    for (auto& p : postfixes) {
        s.write(p.constData(), p.size());
        s << '\n';
    }
    int index = 0;
    s << "\nMapCount " << childMaps.size() << '\n';
    for (auto& elementMap : childMaps)
        elementMap->save(s, ++index, childMapSet, postfixMap);
}

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasher, std::istream& s)
{
    const char* msg = "Invalid element map";

    unsigned id;
    int count = 0;
    std::string tmp;
    if (!(s >> id >> tmp >> count) || tmp != "PostfixCount")
        FC_THROWM(Base::RuntimeError, msg);

    auto& map = _IdToElementMap[id];
    if (map)
        return map;

    std::vector<std::string> postfixes;
    postfixes.reserve(count);
    for (int i = 0; i < count; ++i) {
        postfixes.emplace_back();
        s >> postfixes.back();
    }

    std::vector<ElementMapPtr> childMaps;
    count = 0;
    if (!(s >> tmp >> count) || tmp != "MapCount" || count == 0)
        FC_THROWM(Base::RuntimeError, msg);
    childMaps.reserve(count - 1);
    for (int i = 0; i < count - 1; ++i) {
        childMaps.push_back(
            std::make_shared<ElementMap>()->restore(hasher, s, childMaps, postfixes));
    }

    return restore(hasher, s, childMaps, postfixes);
}

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasher, std::istream& s,
                                  std::vector<ElementMapPtr>& childMaps,
                                  const std::vector<std::string>& postfixes)
{
    const char* msg = "Invalid element map";
    std::string tmp;
    int index = 0;
    int typeCount = 0;
    unsigned id = 0;
    if (!(s >> tmp >> index >> id >> typeCount) || tmp != "ElementMap")
        FC_THROWM(Base::RuntimeError, msg);

    auto& map = _IdToElementMap[id];
    if (map) {
        do {
            if (!std::getline(s, tmp))
                FC_THROWM(Base::RuntimeError, "unexpected end of child element map");
        } while (tmp != "EndMap");
        return map;
    }
    map = shared_from_this();

    const char* hasherWarn = nullptr;
    const char* hasherIDWarn = nullptr;
    const char* postfixWarn = nullptr;
    const char* childSIDWarn = nullptr;
    std::vector<std::string> tokens;

    for (int i = 0; i < typeCount; ++i) {
        int count;
        if (!(s >> tmp))
            FC_THROWM(Base::RuntimeError, "missing element type");
        IndexedName idx(tmp.c_str(), 1);

        if (!(s >> tmp >> count) || tmp != "ChildCount")
            FC_THROWM(Base::RuntimeError, "missing element child count");

        auto& indices = this->indexedNames[idx.getType()];
        for (int j = 0; j < count; ++j) {
            int cindex;
            int offset;
            int count;
            long tag;
            int mapIndex;
            if (!(s >> cindex >> offset >> count >> tag >> mapIndex >> tmp))
                FC_THROWM(Base::RuntimeError, "Invalid element child");
            if (cindex < 0)
                FC_THROWM(Base::RuntimeError, "Invalid element child index");
            if (offset < 0)
                FC_THROWM(Base::RuntimeError, "Invalid element child offset");
            if (mapIndex >= index || mapIndex < 0 || mapIndex > (int)childMaps.size())
                FC_THROWM(Base::RuntimeError, "Invalid element child map index");
            auto& child = indices.children[cindex + offset + count];
            child.indexedName = IndexedName::fromConst(idx.getType(), cindex);
            child.offset = offset;
            child.count = count;
            child.tag = tag;
            if (mapIndex > 0)
                child.elementMap = childMaps[mapIndex - 1];
            else
                child.elementMap = nullptr;
            child.postfix = tmp.c_str();
            this->childElements[child.postfix].childMap = &child;
            this->childElementSize += child.count;

            if (!(s >> tmp))
                FC_THROWM(Base::RuntimeError, "Invalid element child string id");

            tokens.clear();
            boost::split(tokens, tmp, boost::is_any_of("."));
            if (tokens.size() > 1) {
                child.sids.reserve(tokens.size() - 1);
                for (unsigned k = 1; k < tokens.size(); ++k) {
                    // The element child string ID is saved as decimal
                    // instead of hex by accident. To simplify maintenance
                    // of backward compatibility, it is not corrected, and
                    // just restored as decimal here.
                    //
                    // long n = strtol(tokens[k].c_str(), nullptr, 16);
                    long n = strtol(tokens[k].c_str(), nullptr, 10);
                    auto sid = hasher->getID(n);
                    if (!sid)
                        childSIDWarn = "Missing element child string id";
                    else
                        child.sids.push_back(sid);
                }
            }
        }

        if (!(s >> tmp >> count) || tmp != "NameCount")
            FC_THROWM(Base::RuntimeError, "missing element name count");

        boost::io::ios_flags_saver ifs(s);
        s >> std::hex;

        indices.names.resize(count);
        for (int j = 0; j < count; ++j) {
            idx.setIndex(j);
            auto* ref = &indices.names[j];
            int k = 0;
            while (1) {
                if (!(s >> tmp))
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
                ::App::StringID::IndexID prefixid;
                prefixid.id = 0;

                switch (tokens[0][0]) {
                    case ':': {
                        if (tokens.size() < 3)
                            FC_THROWM(Base::RuntimeError, "Invalid element entry");
                        ++offset;
                        long n = strtol(tokens[0].c_str() + 1, nullptr, 16);
                        if (n <= 0 || n > (int)postfixes.size())
                            FC_THROWM(Base::RuntimeError, "Invalid element name index");
                        long m = strtol(tokens[1].c_str(), nullptr, 16);
                        ref->name = MappedName(IndexedName::fromConst(postfixes[n - 1].c_str(), m));
                        break;
                    }
                    case '$':
                        ref->name = MappedName(tokens[0].c_str() + 1);
                        prefixid = ::App::StringID::fromString(ref->name.dataBytes());
                        break;
                    case ';':
                        ref->name = MappedName(tokens[0].c_str() + 1);
                        break;
                    default:
                        FC_THROWM(Base::RuntimeError, "Invalid element name marker");
                }

                if (tokens[offset] != "0") {
                    long n = strtol(tokens[offset].c_str(), nullptr, 16);
                    if (n <= 0 || n > (int)postfixes.size())
                        postfixWarn = "Invalid element postfix index";
                    else
                        ref->name += postfixes[n - 1];
                }

                this->mappedNames.emplace(ref->name, idx);

                if (!hasher) {
                    if (offset + 1 < (int)tokens.size())
                        hasherWarn = "No hasher";
                    continue;
                }

                ref->sids.reserve(tokens.size() - offset - 1 + prefixid.id ? 1 : 0);
                if (prefixid.id) {
                    auto sid = hasher->getID(prefixid.id);
                    if (!sid)
                        hasherIDWarn = "Missing element name prefix id";
                    else
                        ref->sids.push_back(sid);
                }
                for (int l = offset + 1; l < (int)tokens.size(); ++l) {
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
    if (hasherWarn)
        FC_WARN(hasherWarn);
    if (hasherIDWarn)
        FC_WARN(hasherIDWarn);
    if (postfixWarn)
        FC_WARN(postfixWarn);
    if (childSIDWarn)
        FC_WARN(childSIDWarn);

    if (!(s >> tmp) || tmp != "EndMap")
        FC_THROWM(Base::RuntimeError, "unexpected end of child element map");

    return shared_from_this();
}

MappedName ElementMap::addName(MappedName& name, const IndexedName& idx, const ElementIDRefs& sids,
                               bool overwrite, IndexedName* existing)
{
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        if (name.find("#") >= 0 && name.findTagInElementName() < 0) {
            FC_ERR("missing tag postfix " << name);
        }
    }
    do {
        if (overwrite)
            erase(idx);
        auto ret = mappedNames.insert(std::make_pair(name, idx));
        if (ret.second) {              // element just inserted did not exist yet in the map
            ret.first->first.compact();// FIXME see MappedName.cpp
            mappedRef(idx).append(ret.first->first, sids);
            FC_TRACE(idx << " -> " << name);
            return ret.first->first;
        }
        if (ret.first->second == idx) {
            FC_TRACE("duplicate " << idx << " -> " << name);
            return ret.first->first;
        }
        if (!overwrite) {
            if (existing)
                *existing = ret.first->second;
            return MappedName();
        }

        erase(ret.first->first);
    } while (true);
}

void ElementMap::addPostfix(const QByteArray& postfix, std::map<QByteArray, int>& postfixMap,
                            std::vector<QByteArray>& postfixes)
{
    if (postfix.isEmpty())
        return;
    auto res = postfixMap.insert(std::make_pair(postfix, 0));
    if (res.second) {
        postfixes.push_back(postfix);
        res.first->second = (int)postfixes.size();
    }
}

MappedName ElementMap::setElementName(const IndexedName& element, const MappedName& name,
                                      ElementMapPtr& elementMap, long masterTag,
                                      const ElementIDRefs* sid, bool overwrite)
{
    if (!element)
        throw Base::ValueError("Invalid input");
    if (!name) {
        if (elementMap)
            elementMap->erase(element);
        return MappedName();
    }

    for (int i = 0, count = name.size(); i < count; ++i) {
        char c = name[i];
        if (c == '.' || std::isspace((int)c))
            FC_THROWM(Base::RuntimeError, "Illegal character in mapped name: " << name);
    }
    for (const char* s = element.getType(); *s; ++s) {
        char c = *s;
        if (c == '.' || std::isspace((int)c))
            FC_THROWM(Base::RuntimeError, "Illegal character in element name: " << element);
    }
    if (!elementMap)
        elementMap = std::make_shared<ElementMap>();

    ElementIDRefs _sid;
    if (!sid)
        sid = &_sid;

    std::ostringstream ss;
    Data::MappedName n(name);
    for (int i = 0;;) {
        IndexedName existing;
        MappedName res = elementMap->addName(n, element, *sid, overwrite, &existing);
        if (res)
            return res;
        if (++i == 100) {
            FC_ERR("unresolved duplicate element mapping '" << name << ' ' << element << '/'
                                                            << existing);
            return name;
        }
        if (sid != &_sid)
            _sid = *sid;
        n = renameDuplicateElement(i, element, existing, name, _sid, masterTag);
        if (!n)
            return name;
        sid = &_sid;
    }
}

// try to hash element name while preserving the source tag
void ElementMap::encodeElementName(char element_type, MappedName& name, std::ostringstream& ss,
                                   ElementIDRefs* sids, long masterTag, const char* postfix,
                                   long tag, bool forceTag) const
{
    if (postfix && postfix[0]) {
        if (!boost::starts_with(postfix, ELEMENT_MAP_PREFIX))
            ss << ELEMENT_MAP_PREFIX;
        ss << postfix;
    }
    long inputTag = 0;
    if (!forceTag && !ss.tellp()) {
        if (!tag || tag == masterTag)
            return;
        name.findTagInElementName(&inputTag, nullptr, nullptr, nullptr, true);
        if (inputTag == tag)
            return;
    }
    else if (!tag || (!forceTag && tag == masterTag)) {
        int pos = name.findTagInElementName(&inputTag, nullptr, nullptr, nullptr, true);
        if (inputTag) {
            tag = inputTag;
            // About to encode the same tag used last time. This usually means
            // the owner object is doing multi step modeling. Let's not
            // recursively encode the same tag too many time. It will be a
            // waste of memory, because the intermediate shapes has no
            // corresponding objects, so no real value for history tracing.
            //
            // On the other hand, we still need to distinguish the original name
            // from the input object from the element name of the intermediate
            // shapes. So we limit ourselves to encode only one extra level
            // using the same tag. In order to do that, we need to dehash the
            // previous level name, and check for its tag.
            Data::MappedName n(name, 0, pos);
            Data::MappedName prev = dehashElementName(n);
            long prevTag = 0;
            prev.findTagInElementName(&prevTag, nullptr, nullptr, nullptr, true);
            if (prevTag == inputTag || prevTag == -inputTag)
                name = n;
        }
    }

    if (sids && this->hasher) {
        name = hashElementName(name, *sids);
        if (!forceTag && !tag && ss.tellp())
            forceTag = true;
    }
    if (forceTag || tag) {
        assert(element_type);
        int pos = ss.tellp();
        boost::io::ios_flags_saver ifs(ss);
        ss << POSTFIX_TAG << std::hex;
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

MappedName ElementMap::hashElementName(const MappedName& name, ElementIDRefs& sids) const
{
    if (!this->hasher || !name)
        return name;
    if (name.find(ELEMENT_MAP_PREFIX) < 0)
        return name;
    App::StringIDRef sid = this->hasher->getID(name, sids);
    const auto& related = sid.relatedIDs();
    if (related == sids) {
        sids.clear();
        sids.push_back(sid);
    }
    else {
        ElementIDRefs tmp;
        tmp.push_back(sid);
        for (auto& s : sids) {
            if (related.indexOf(s) < 0)
                tmp.push_back(s);
        }
        sids = tmp;
    }
    return MappedName(sid.toString());
}

MappedName ElementMap::dehashElementName(const MappedName& name) const
{
    if (name.empty())
        return name;
    if (!this->hasher)
        return name;
    auto id = App::StringID::fromString(name.toRawBytes());
    if (!id)
        return name;
    auto sid = this->hasher->getID(id);
    if (!sid) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE))
            FC_WARN("failed to find hash id " << id);
        else
            FC_LOG("failed to find hash id " << id);
        return name;
    }
    if (sid.isHashed()) {
        FC_LOG("cannot dehash id " << id);
        return name;
    }
    MappedName ret(
        sid.toString());// FIXME .toString() was missing in original function. is this correct?
    FC_TRACE("dehash " << name << " -> " << ret);
    return ret;
}

MappedName ElementMap::renameDuplicateElement(int index, const IndexedName& element,
                                              const IndexedName& element2, const MappedName& name,
                                              ElementIDRefs& sids, long masterTag)
{
    std::ostringstream ss;
    ss << ELEMENT_MAP_PREFIX << 'D' << std::hex << index;
    MappedName renamed(name);
    encodeElementName(element.getType()[0], renamed, ss, &sids, masterTag);
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        FC_WARN("duplicate element mapping '" << name << " -> " << renamed << ' ' << element << '/'
                                              << element2);
    return renamed;
}

bool ElementMap::erase(const MappedName& name)
{
    auto it = this->mappedNames.find(name);
    if (it == this->mappedNames.end())
        return false;
    MappedNameRef* ref = findMappedRef(it->second);
    if (!ref)
        return false;
    ref->erase(name);
    this->mappedNames.erase(it);
    return true;
}

bool ElementMap::erase(const IndexedName& idx)
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end())
        return false;
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size())
        return false;
    auto& ref = indices.names[idx.getIndex()];
    for (auto* r = &ref; r; r = r->next.get())
        this->mappedNames.erase(r->name);
    ref.clear();
    return true;
}

unsigned long ElementMap::size() const
{
    return mappedNames.size() + childElementSize;
}

bool ElementMap::empty() const
{
    return mappedNames.empty() && childElementSize == 0;
}

IndexedName ElementMap::find(const MappedName& name, ElementIDRefs* sids) const
{
    auto it = mappedNames.find(name);
    if (it == mappedNames.end()) {
        if (childElements.isEmpty())
            return IndexedName();

        int len = 0;
        if (name.findTagInElementName(nullptr, &len, nullptr, nullptr, false, false) < 0)
            return IndexedName();
        QByteArray key = name.toRawBytes(len);
        auto it = this->childElements.find(key);
        if (it == this->childElements.end())
            return IndexedName();

        const auto& child = *it.value().childMap;
        IndexedName res;

        MappedName childName = MappedName::fromRawData(name, 0, len);
        if (child.elementMap)
            res = child.elementMap->find(childName, sids);
        else
            res = childName.toIndexedName();

        if (res && boost::equals(res.getType(), child.indexedName.getType())
            && child.indexedName.getIndex() <= res.getIndex()
            && child.indexedName.getIndex() + child.count > res.getIndex()) {
            res.setIndex(res.getIndex() + it.value().childMap->offset);
            return res;
        }

        return IndexedName();
    }

    if (sids) {
        const MappedNameRef* ref = findMappedRef(it->second);
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

MappedName ElementMap::find(const IndexedName& idx, ElementIDRefs* sids) const
{
    if (!idx)
        return MappedName();

    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end())
        return MappedName();

    auto& indices = iter->second;
    if (idx.getIndex() < (int)indices.names.size()) {
        const MappedNameRef& ref = indices.names[idx.getIndex()];
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
        && it->second.indexedName.getIndex() + it->second.offset <= idx.getIndex()) {
        auto& child = it->second;
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

std::vector<std::pair<MappedName, ElementIDRefs>> ElementMap::findAll(const IndexedName& idx) const
{
    std::vector<std::pair<MappedName, ElementIDRefs>> res;
    if (!idx)
        return res;

    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end())
        return res;

    auto& indices = iter->second;
    if (idx.getIndex() < (int)indices.names.size()) {
        const MappedNameRef& ref = indices.names[idx.getIndex()];
        int count = 0;
        for (auto r = &ref; r; r = r->next.get()) {
            if (r->name)
                ++count;
        }
        if (count) {
            res.reserve(count);
            for (auto r = &ref; r; r = r->next.get()) {
                if (r->name)
                    res.emplace_back(r->name, r->sids);
            }
            return res;
        }
    }

    auto it = indices.children.upper_bound(idx.getIndex());
    if (it != indices.children.end()
        && it->second.indexedName.getIndex() + it->second.offset <= idx.getIndex()) {
        auto& child = it->second;
        IndexedName childIdx(idx.getType(), idx.getIndex() - child.offset);
        if (child.elementMap) {
            res = child.elementMap->findAll(childIdx);
            for (auto& v : res)
                v.first += child.postfix;
        }
        else
            res.emplace_back(MappedName(childIdx) + child.postfix, ElementIDRefs());
    }

    return res;
}

const MappedNameRef* ElementMap::findMappedRef(const IndexedName& idx) const
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end())
        return nullptr;
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size())
        return nullptr;
    return &indices.names[idx.getIndex()];
}

MappedNameRef* ElementMap::findMappedRef(const IndexedName& idx)
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end())
        return nullptr;
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size())
        return nullptr;
    return &indices.names[idx.getIndex()];
}

MappedNameRef& ElementMap::mappedRef(const IndexedName& idx)
{
    assert(idx);
    auto& indices = this->indexedNames[idx.getType()];
    if (idx.getIndex() >= (int)indices.names.size())
        indices.names.resize(idx.getIndex() + 1);
    return indices.names[idx.getIndex()];
}

bool ElementMap::hasChildElementMap() const
{
    return !childElements.empty();
}

void ElementMap::hashChildMaps(long masterTag)
{
    if (childElements.empty() || !this->hasher)
        return;
    std::ostringstream ss;
    for (auto& v : this->indexedNames) {
        for (auto& vv : v.second.children) {
            auto& child = vv.second;
            int len = 0;
            long tag;
            int pos = MappedName::fromRawData(child.postfix)
                          .findTagInElementName(&tag, &len, nullptr, nullptr, false, false);
            if (pos > 10) {
                MappedName postfix = hashElementName(
                    MappedName::fromRawData(child.postfix.constData(), pos), child.sids);
                ss.str("");
                ss << MAPPED_CHILD_ELEMENTS_PREFIX << postfix;
                MappedName tmp;
                encodeElementName(
                    child.indexedName[0], tmp, ss, nullptr, masterTag, nullptr, child.tag, true);
                this->childElements.remove(child.postfix);
                child.postfix = tmp.toBytes();
                this->childElements[child.postfix].childMap = &child;
            }
        }
    }
}

void ElementMap::collectChildMaps(std::map<const ElementMap*, int>& childMapSet,
                                  std::vector<const ElementMap*>& childMaps,
                                  std::map<QByteArray, int>& postfixMap,
                                  std::vector<QByteArray>& postfixes) const
{
    auto res = childMapSet.insert(std::make_pair(this, 0));
    if (!res.second)
        return;

    for (auto& v : this->indexedNames) {
        addPostfix(QByteArray::fromRawData(v.first, qstrlen(v.first)), postfixMap, postfixes);

        for (auto& vv : v.second.children) {
            auto& child = vv.second;
            if (child.elementMap)
                child.elementMap->collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);
        }
    }

    for (auto& v : this->mappedNames)
        addPostfix(v.first.constPostfix(), postfixMap, postfixes);

    childMaps.push_back(this);
    res.first->second = (int)childMaps.size();
}

void ElementMap::addChildElements(ElementMapPtr& elementMap, long masterTag,
                                  const std::vector<MappedChildElements>& children)
{
    std::ostringstream ss;
    ss << std::hex;

    // To avoid possibly very long recursive child map lookup, resulting very
    // long mapped names, we try to resolve the grand child map now.
    std::vector<MappedChildElements> expansion;
    for (auto it = children.begin(); it != children.end(); ++it) {
        auto& child = *it;
        if (!child.elementMap || child.elementMap->childElements.empty()) {
            if (expansion.size())
                expansion.push_back(child);
            continue;
        }
        auto& indices = child.elementMap->indexedNames[child.indexedName.getType()];
        if (indices.children.empty()) {
            if (expansion.size())
                expansion.push_back(child);
            continue;
        }

        // Note that it is allow to have both mapped names and child map. We
        // may have to split the current child mapping into pieces.

        int start = child.indexedName.getIndex();
        int end = start + child.count;
        for (auto iter = indices.children.upper_bound(start); iter != indices.children.end();
             ++iter) {
            auto& grandchild = iter->second;
            int istart = grandchild.indexedName.getIndex() + grandchild.offset;
            int iend = istart + grandchild.count;
            if (end <= istart)
                break;
            if (istart >= end) {
                if (expansion.size()) {
                    expansion.push_back(child);
                    expansion.back().indexedName.setIndex(start);
                    expansion.back().count = end - start;
                }
                break;
            }
            if (expansion.empty()) {
                expansion.reserve(children.size() + 10);
                expansion.insert(expansion.end(), children.begin(), it);
            }
            expansion.push_back(child);
            auto* entry = &expansion.back();
            if (istart > start) {
                entry->indexedName.setIndex(start);
                entry->count = istart - start;

                expansion.push_back(child);
                entry = &expansion.back();
            }
            else
                istart = start;

            if (iend > end)
                iend = end;

            entry->indexedName.setIndex(istart - grandchild.offset);
            entry->count = iend - istart;
            entry->offset += grandchild.offset;
            entry->elementMap = grandchild.elementMap;
            entry->sids += grandchild.sids;
            if (grandchild.postfix.size()) {
                if (entry->postfix.size()
                    && !entry->postfix.startsWith(ELEMENT_MAP_PREFIX.c_str())) {
                    entry->postfix =
                        grandchild.postfix + ELEMENT_MAP_PREFIX.c_str() + entry->postfix;
                }
                else
                    entry->postfix = grandchild.postfix + entry->postfix;
            }

            start = iend;
            if (start >= end)
                break;
        }
        if (expansion.size() && start < end) {
            expansion.push_back(child);
            expansion.back().indexedName.setIndex(start);
            expansion.back().count = end - start;
        }
    }

    for (auto& child : expansion.size() ? expansion : children) {
        if (!child.indexedName || !child.count) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_ERR("invalid mapped child element");
            continue;
        }

        ss.str("");
        MappedName tmp;

        ChildMapInfo* entry = nullptr;

        // do child mapping only if the child element count >= 5
        if (child.count >= 5 || !child.elementMap) {
            encodeElementName(child.indexedName[0],
                              tmp,
                              ss,
                              nullptr,
                              masterTag,
                              child.postfix.constData(),
                              child.tag,
                              true);

            // Perform some disambiguation in case the same shape is mapped
            // multiple times, e.g. draft array.
            entry = &childElements[tmp.toBytes()];
            int mapIndex = entry->mapIndices[child.elementMap.get()]++;
            ++entry->index;
            if (entry->index != 1 && child.elementMap && mapIndex == 0) {
                // This child has duplicated 'tag' and 'postfix', but it
                // has its own element map. We'll expand this map now.
                entry = nullptr;
            }
        }

        if (!entry) {
            IndexedName childIdx(child.indexedName);
            IndexedName idx(childIdx.getType(), childIdx.getIndex() + child.offset);
            for (int i = 0; i < child.count; ++i, ++childIdx, ++idx) {
                ElementIDRefs sids;
                MappedName name = child.elementMap->find(childIdx, &sids);
                if (!name) {
                    if (!child.tag || child.tag == masterTag) {
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                            FC_WARN("unmapped element");
                        continue;
                    }
                    name = MappedName(childIdx);
                }
                ss.str("");
                encodeElementName(
                    idx[0], name, ss, &sids, masterTag, child.postfix.constData(), child.tag);
                setElementName(idx, name, elementMap, masterTag, &sids);
            }
            continue;
        }

        if (entry->index != 1) {
            // There is some ambiguity in child mapping. We need some
            // additional postfix for disambiguation. NOTE: We are not
            // using ComplexGeoData::indexPostfix() so as to not confuse
            // other code that actually uses this postfix for indexing
            // purposes. Here, we just need some postfix for
            // disambiguation. We don't need to extract the index.
            ss.str("");
            ss << ELEMENT_MAP_PREFIX << ":C" << entry->index - 1;

            tmp.clear();
            encodeElementName(child.indexedName[0],
                              tmp,
                              ss,
                              nullptr,
                              masterTag,
                              child.postfix.constData(),
                              child.tag,
                              true);

            entry = &childElements[tmp.toBytes()];
            if (entry->childMap) {
                FC_ERR("duplicate mapped child element");
                continue;
            }
        }

        auto& indices = this->indexedNames[child.indexedName.getType()];
        auto res = indices.children.emplace(
            child.indexedName.getIndex() + child.offset + child.count, child);
        if (!res.second) {
            if (!entry->childMap)
                this->childElements.remove(tmp.toBytes());
            FC_ERR("duplicate mapped child element");
            continue;
        }

        auto& insertedChild = res.first->second;
        insertedChild.postfix = tmp.toBytes();
        entry->childMap = &insertedChild;
        childElementSize += insertedChild.count;
    }
}

std::vector<ElementMap::MappedChildElements> ElementMap::getChildElements() const
{
    std::vector<MappedChildElements> res;
    res.reserve(this->childElements.size());
    for (auto& v : this->childElements)
        res.push_back(*v.childMap);
    return res;
}

std::vector<MappedElement> ElementMap::getAll() const
{
    std::vector<MappedElement> ret;
    ret.reserve(size());
    for (auto& v : this->mappedNames)
        ret.emplace_back(v.first, v.second);
    for (auto& v : this->childElements) {
        auto& child = *v.childMap;
        IndexedName idx(child.indexedName);
        idx.setIndex(idx.getIndex() + child.offset);
        IndexedName childIdx(child.indexedName);
        for (int i = 0; i < child.count; ++i, ++idx, ++childIdx) {
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


}// Namespace Data
