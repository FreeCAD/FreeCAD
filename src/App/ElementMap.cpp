#include "PreCompiled.h"
#ifndef _PreComp_
#include <unordered_map>
#ifndef FC_DEBUG
#include <random>
#endif
#endif

#include "ElementMap.h"
#include "ElementNamingUtils.h"

#include "App/Application.h"
#include "Base/Console.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>


FC_LOG_LEVEL_INIT("ElementMap", true, 2);// NOLINT

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
static std::unordered_map<const ElementMap*, unsigned> _elementMapToId;
static std::unordered_map<unsigned, ElementMapPtr> _idToElementMap;


void ElementMap::init()
{
    static bool inited;
    if (!inited) {
        inited = true;
        ::App::GetApplication().signalStartSaveDocument.connect(
            [](const ::App::Document&, const std::string&) {
                _elementMapToId.clear();
            });
        ::App::GetApplication().signalFinishSaveDocument.connect(
            [](const ::App::Document&, const std::string&) {
                _elementMapToId.clear();
            });
        ::App::GetApplication().signalStartRestoreDocument.connect([](const ::App::Document&) {
            _idToElementMap.clear();
        });
        ::App::GetApplication().signalFinishRestoreDocument.connect([](const ::App::Document&) {
            _idToElementMap.clear();
        });
    }
}

ElementMap::ElementMap()
{
    init();
}


void ElementMap::beforeSave(const ::App::StringHasherRef& hasherRef) const
{
    unsigned& id = _elementMapToId[this];
    if (id == 0U) {
        id = _elementMapToId.size();
    }
    this->_id = id;

    for (auto& indexedName : this->indexedNames) {
        for (const MappedNameRef& mappedName : indexedName.second.names) {
            for (const MappedNameRef* ref = &mappedName; ref; ref = ref->next.get()) {
                for (const ::App::StringIDRef& sid : ref->sids) {
                    if (sid.isFromSameHasher(hasherRef)) {
                        sid.mark();
                    }
                }
            }
        }
        for (auto& childPair : indexedName.second.children) {
            if (childPair.second.elementMap) {
                childPair.second.elementMap->beforeSave(hasherRef);
            }
            for (auto& sid : childPair.second.sids) {
                if (sid.isFromSameHasher(hasherRef)) {
                    sid.mark();
                }
            }
        }
    }
}

void ElementMap::save(std::ostream& stream, int index,
                      const std::map<const ElementMap*, int>& childMapSet,
                      const std::map<QByteArray, int>& postfixMap) const
{
    stream << "\nElementMap " << index << ' ' << this->_id << ' ' << this->indexedNames.size()
           << '\n';

    for (auto& indexedName : this->indexedNames) {
        stream << '\n' << indexedName.first << '\n';

        stream << "\nChildCount " << indexedName.second.children.size() << '\n';
        for (auto& vv : indexedName.second.children) {
            auto& child = vv.second;
            int mapIndex = 0;
            if (child.elementMap) {
                auto it = childMapSet.find(child.elementMap.get());
                if (it == childMapSet.end() || it->second == 0) {
                    FC_ERR("Invalid child element map");// NOLINT
                }
                else {
                    mapIndex = it->second;
                }
            }
            stream << child.indexedName.getIndex() << ' ' << child.offset << ' ' << child.count
                   << ' ' << child.tag << ' ' << mapIndex << ' ';
            stream.write(child.postfix.constData(), child.postfix.size());
            stream << ' ' << '0';
            for (auto& sid : child.sids) {
                if (sid.isMarked()) {
                    stream << '.' << sid.value();
                }
            }
            stream << '\n';
        }

        stream << "\nNameCount " << indexedName.second.names.size() << '\n';
        if (indexedName.second.names.empty()) {
            continue;
        }

        boost::io::ios_flags_saver ifs(stream);
        stream << std::hex;

        for (auto& dequeueOfMappedNameRef : indexedName.second.names) {
            for (auto ref = &dequeueOfMappedNameRef; ref; ref = ref->next.get()) {
                if (!ref->name) {
                    break;
                }

                ::App::StringID::IndexID prefixID {};
                prefixID.id = 0;
                IndexedName idx(ref->name.dataBytes());
                bool printName = true;
                if (idx) {
                    auto key = QByteArray::fromRawData(idx.getType(),
                                                       static_cast<int>(qstrlen(idx.getType())));
                    auto it = postfixMap.find(key);
                    if (it != postfixMap.end()) {
                        stream << ':' << it->second << '.' << idx.getIndex();
                        printName = false;
                    }
                }
                else {
                    prefixID = ::App::StringID::fromString(ref->name.dataBytes());
                    if (prefixID.id != 0) {
                        for (auto& sid : ref->sids) {
                            if (sid.isMarked() && sid.value() == prefixID.id) {
                                stream << '$';
                                stream.write(ref->name.dataBytes().constData(),
                                             ref->name.dataBytes().size());
                                printName = false;
                                break;
                            }
                        }
                        if (printName) {
                            prefixID.id = 0;
                        }
                    }
                }
                if (printName) {
                    stream << ';';
                    stream.write(ref->name.dataBytes().constData(), ref->name.dataBytes().size());
                }

                const QByteArray& postfix = ref->name.postfixBytes();
                if (postfix.isEmpty()) {
                    stream << ".0";
                }
                else {
                    auto it = postfixMap.find(postfix);
                    assert(it != postfixMap.end());
                    stream << '.' << it->second;
                }
                for (auto& sid : ref->sids) {
                    if (sid.isMarked() && sid.value() != prefixID.id) {
                        stream << '.' << sid.value();
                    }
                }

                stream << ' ';
            }
            stream << "0\n";
        }
    }
    stream << "\nEndMap\n";
}

void ElementMap::save(std::ostream& stream) const
{
    std::map<const ElementMap*, int> childMapSet;
    std::vector<const ElementMap*> childMaps;
    std::map<QByteArray, int> postfixMap;
    std::vector<QByteArray> postfixes;

    collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);

    stream << this->_id << " PostfixCount " << postfixes.size() << '\n';
    for (auto& postfix : postfixes) {
        stream.write(postfix.constData(), postfix.size());
        stream << '\n';
    }
    int index = 0;
    stream << "\nMapCount " << childMaps.size() << '\n';
    for (auto& elementMap : childMaps) {
        elementMap->save(stream, ++index, childMapSet, postfixMap);
    }
}

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasherRef, std::istream& stream)
{
    const char* msg = "Invalid element map";

    unsigned id = 0;
    int count = 0;
    std::string tmp;
    if (!(stream >> id >> tmp >> count) || tmp != "PostfixCount") {
        FC_THROWM(Base::RuntimeError, msg);// NOLINT
    }

    auto& map = _idToElementMap[id];
    if (map) {
        return map;
    }

    std::vector<std::string> postfixes;
    postfixes.reserve(count);
    for (int i = 0; i < count; ++i) {
        postfixes.emplace_back();
        stream >> postfixes.back();
    }

    std::vector<ElementMapPtr> childMaps;
    count = 0;
    if (!(stream >> tmp >> count) || tmp != "MapCount" || count == 0) {
        FC_THROWM(Base::RuntimeError, msg);// NOLINT
    }
    childMaps.reserve(count - 1);
    for (int i = 0; i < count - 1; ++i) {
        childMaps.push_back(
            std::make_shared<ElementMap>()->restore(hasherRef, stream, childMaps, postfixes));
    }

    return restore(hasherRef, stream, childMaps, postfixes);
}

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasherRef, std::istream& stream,
                                  std::vector<ElementMapPtr>& childMaps,
                                  const std::vector<std::string>& postfixes)
{
    const char* msg = "Invalid element map";
    const int hexBase {16};
    const int decBase {10};
    std::string tmp;
    int index = 0;
    int typeCount = 0;
    unsigned id = 0;
    if (!(stream >> tmp >> index >> id >> typeCount) || tmp != "ElementMap") {
        FC_THROWM(Base::RuntimeError, msg);// NOLINT
    }

    auto& map = _idToElementMap[id];
    if (map) {
        while (tmp != "EndMap") {
            if (!std::getline(stream, tmp)) {
                FC_THROWM(Base::RuntimeError, "unexpected end of child element map");// NOLINT
            }
        }
        return map;
    }

    const char* hasherWarn = nullptr;
    const char* hasherIDWarn = nullptr;
    const char* postfixWarn = nullptr;
    const char* childSIDWarn = nullptr;
    std::vector<std::string> tokens;

    for (int i = 0; i < typeCount; ++i) {
        int outerCount = 0;
        if (!(stream >> tmp)) {
            FC_THROWM(Base::RuntimeError, "missing element type");// NOLINT
        }
        IndexedName idx(tmp.c_str(), 1);

        if (!(stream >> tmp >> outerCount) || tmp != "ChildCount") {
            FC_THROWM(Base::RuntimeError, "missing element child count");// NOLINT
        }

        auto& indices = this->indexedNames[idx.getType()];
        for (int j = 0; j < outerCount; ++j) {
            int cIndex = 0;
            int offset = 0;
            int count = 0;
            long tag = 0;
            int mapIndex = 0;
            if (!(stream >> cIndex >> offset >> count >> tag >> mapIndex >> tmp)) {
                FC_THROWM(Base::RuntimeError, "Invalid element child");// NOLINT
            }
            if (cIndex < 0) {
                FC_THROWM(Base::RuntimeError, "Invalid element child index");// NOLINT
            }
            if (offset < 0) {
                FC_THROWM(Base::RuntimeError, "Invalid element child offset");// NOLINT
            }
            if (mapIndex >= index || mapIndex < 0 || mapIndex > (int)childMaps.size()) {
                FC_THROWM(Base::RuntimeError, "Invalid element child map index");// NOLINT
            }
            auto& child = indices.children[cIndex + offset + count];
            child.indexedName = IndexedName::fromConst(idx.getType(), cIndex);
            child.offset = offset;
            child.count = count;
            child.tag = tag;
            if (mapIndex > 0) {
                child.elementMap = childMaps[mapIndex - 1];
            }
            else {
                child.elementMap = nullptr;
            }
            child.postfix = tmp.c_str();
            this->childElements[child.postfix].childMap = &child;
            this->childElementSize += child.count;

            if (!(stream >> tmp)) {
                FC_THROWM(Base::RuntimeError, "Invalid element child string id");// NOLINT
            }

            tokens.clear();
            boost::split(tokens, tmp, boost::is_any_of("."));
            if (tokens.size() > 1) {
                child.sids.reserve(static_cast<int>(tokens.size()) - 1);
                for (unsigned k = 1; k < tokens.size(); ++k) {
                    // The element child string ID is saved as decimal
                    // instead of hex by accident. To simplify maintenance
                    // of backward compatibility, it is not corrected, and
                    // just restored as decimal here.
                    long childID = strtol(tokens[k].c_str(), nullptr, decBase);
                    auto sid = hasherRef->getID(childID);
                    if (!sid) {
                        childSIDWarn = "Missing element child string id";
                    }
                    else {
                        child.sids.push_back(sid);
                    }
                }
            }
        }

        if (!(stream >> tmp >> outerCount) || tmp != "NameCount") {
            FC_THROWM(Base::RuntimeError, "missing element name outerCount");// NOLINT
        }

        boost::io::ios_flags_saver ifs(stream);
        stream >> std::hex;

        indices.names.resize(outerCount);
        for (int j = 0; j < outerCount; ++j) {
            idx.setIndex(j);
            auto* ref = &indices.names[j];
            int innerCount = 0;
            while (true) {
                if (!(stream >> tmp)) {
                    FC_THROWM(Base::RuntimeError, "Failed to read element name");// NOLINT
                }
                if (tmp == "0") {
                    break;
                }
                if (innerCount++ != 0) {
                    ref->next = std::make_unique<MappedNameRef>();
                    ref = ref->next.get();
                }
                tokens.clear();
                boost::split(tokens, tmp, boost::is_any_of("."));
                if (tokens.size() < 2) {
                    FC_THROWM(Base::RuntimeError, "Invalid element entry");// NOLINT
                }

                int offset = 1;
                ::App::StringID::IndexID prefixID {};
                prefixID.id = 0;

                switch (tokens[0][0]) {
                    case ':': {
                        if (tokens.size() < 3) {
                            FC_THROWM(Base::RuntimeError, "Invalid element entry");// NOLINT
                        }
                        ++offset;
                        long elementNameIndex = strtol(tokens[0].c_str() + 1, nullptr, hexBase);
                        if (elementNameIndex <= 0 || elementNameIndex > (int)postfixes.size()) {
                            FC_THROWM(Base::RuntimeError, "Invalid element name index");// NOLINT
                        }
                        long elementIndex = strtol(tokens[1].c_str(), nullptr, hexBase);
                        ref->name = MappedName(
                            IndexedName::fromConst(postfixes[elementNameIndex - 1].c_str(),
                                                   static_cast<int>(elementIndex)));
                        break;
                    }
                    case '$':
                        ref->name = MappedName(tokens[0].c_str() + 1);
                        prefixID = ::App::StringID::fromString(ref->name.dataBytes());
                        break;
                    case ';':
                        ref->name = MappedName(tokens[0].c_str() + 1);
                        break;
                    default:
                        FC_THROWM(Base::RuntimeError, "Invalid element name marker");// NOLINT
                }

                if (tokens[offset] != "0") {
                    long postfixIndex = strtol(tokens[offset].c_str(), nullptr, hexBase);
                    if (postfixIndex <= 0 || postfixIndex > (int)postfixes.size()) {
                        postfixWarn = "Invalid element postfix index";
                    }
                    else {
                        ref->name += postfixes[postfixIndex - 1];
                    }
                }

                this->mappedNames.emplace(ref->name, idx);

                if (!hasherRef) {
                    if (offset + 1 < (int)tokens.size()) {
                        hasherWarn = "No hasherRef";
                    }
                    continue;
                }

                ref->sids.reserve((tokens.size() - offset - 1 + prefixID.id) != 0U ? 1 : 0);
                if (prefixID.id != 0) {
                    auto sid = hasherRef->getID(prefixID.id);
                    if (!sid) {
                        hasherIDWarn = "Missing element name prefix id";
                    }
                    else {
                        ref->sids.push_back(sid);
                    }
                }
                for (int l = offset + 1; l < (int)tokens.size(); ++l) {
                    long readID = strtol(tokens[l].c_str(), nullptr, hexBase);
                    auto sid = hasherRef->getID(readID);
                    if (!sid) {
                        hasherIDWarn = "Invalid element name string id";
                    }
                    else {
                        ref->sids.push_back(sid);
                    }
                }
            }
        }
    }
    if (hasherWarn) {
        FC_WARN(hasherWarn);// NOLINT
    }
    if (hasherIDWarn) {
        FC_WARN(hasherIDWarn);// NOLINT
    }
    if (postfixWarn) {
        FC_WARN(postfixWarn);// NOLINT
    }
    if (childSIDWarn) {
        FC_WARN(childSIDWarn);// NOLINT
    }

    if (!(stream >> tmp) || tmp != "EndMap") {
        FC_THROWM(Base::RuntimeError, "unexpected end of child element map");// NOLINT
    }

    return shared_from_this();
}

MappedName ElementMap::addName(MappedName& name, const IndexedName& idx, const ElementIDRefs& sids,
                               bool overwrite, IndexedName* existing)
{
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        if (name.find("#") >= 0 && name.findTagInElementName() < 0) {
            FC_ERR("missing tag postfix " << name);// NOLINT
        }
    }
    while (true) {
        if (overwrite) {
            erase(idx);
        }
        auto ret = mappedNames.insert(std::make_pair(name, idx));
        if (ret.second) {              // element just inserted did not exist yet in the map
            ret.first->first.compact();// FIXME see MappedName.cpp
            mappedRef(idx).append(ret.first->first, sids);
            FC_TRACE(idx << " -> " << name);// NOLINT
            return ret.first->first;
        }
        if (ret.first->second == idx) {
            FC_TRACE("duplicate " << idx << " -> " << name);// NOLINT
            return ret.first->first;
        }
        if (!overwrite) {
            if (existing) {
                *existing = ret.first->second;
            }
            return {};
        }

        erase(ret.first->first);
    };
}

void ElementMap::addPostfix(const QByteArray& postfix, std::map<QByteArray, int>& postfixMap,
                            std::vector<QByteArray>& postfixes)
{
    if (postfix.isEmpty()) {
        return;
    }
    auto res = postfixMap.insert(std::make_pair(postfix, 0));
    if (res.second) {
        postfixes.push_back(postfix);
        res.first->second = (int)postfixes.size();
    }
}

MappedName ElementMap::setElementName(const IndexedName& element, const MappedName& name,
                                      long masterTag, const ElementIDRefs* sid, bool overwrite)
{
    if (!element) {
        throw Base::ValueError("Invalid input");
    }
    if (!name) {
        erase(element);
        return {};
    }

    for (int i = 0, count = name.size(); i < count; ++i) {
        char check = name[i];
        if (check == '.' || (std::isspace((int)check) != 0)) {
            FC_THROWM(Base::RuntimeError, "Illegal character in mapped name: " << name);// NOLINT
        }
    }
    for (const char* readChar = element.getType(); *readChar != 0; ++readChar) {
        char check = *readChar;
        if (check == '.' || (std::isspace((int)check) != 0)) {
            FC_THROWM(Base::RuntimeError,// NOLINT
                      "Illegal character in element name: " << element);
        }
    }

    ElementIDRefs _sid;
    if (!sid) {
        sid = &_sid;
    }

    std::ostringstream ss;
    Data::MappedName mappedName(name);
    for (int i = 0;;) {
        IndexedName existing;
        MappedName res = this->addName(mappedName, element, *sid, overwrite, &existing);
        if (res) {
            return res;
        }
        const int maxAttempts {100};
        if (++i == maxAttempts) {
            FC_ERR("unresolved duplicate element mapping '"// NOLINT
                   << name << ' ' << element << '/' << existing);
            return name;
        }
        if (sid != &_sid) {
            _sid = *sid;
        }
        mappedName = renameDuplicateElement(i, element, existing, name, _sid, masterTag);
        if (!mappedName) {
            return name;
        }
        sid = &_sid;
    }
}

// try to hash element name while preserving the source tag
void ElementMap::encodeElementName(char element_type, MappedName& name, std::ostringstream& ss,
                                   ElementIDRefs* sids, long masterTag, const char* postfix,
                                   long tag, bool forceTag) const
{
    if (postfix && (postfix[0] != 0)) {
        if (!boost::starts_with(postfix, ELEMENT_MAP_PREFIX)) {
            ss << ELEMENT_MAP_PREFIX;
        }
        ss << postfix;
    }
    long inputTag = 0;
    if (!forceTag && (ss.tellp() == 0)) {
        if ((tag == 0) || tag == masterTag) {
            return;
        }
        name.findTagInElementName(&inputTag, nullptr, nullptr, nullptr, true);
        if (inputTag == tag) {
            return;
        }
    }
    else if ((tag == 0) || (!forceTag && tag == masterTag)) {
        int pos = name.findTagInElementName(&inputTag, nullptr, nullptr, nullptr, true);
        if (inputTag != 0) {
            tag = inputTag;
            // About to encode the same tag used last time. This usually means
            // the owner object is doing multistep modeling. Let's not
            // recursively encode the same tag too many times. It will be a
            // waste of memory, because the intermediate shapes has no
            // corresponding objects, so no real value for history tracing.
            //
            // On the other hand, we still need to distinguish the original name
            // from the input object from the element name of the intermediate
            // shapes. So we limit ourselves to encode only one extra level
            // using the same tag. In order to do that, we need to de-hash the
            // previous level name, and check for its tag.
            Data::MappedName mappedName(name, 0, pos);
            Data::MappedName prev = dehashElementName(mappedName);
            long prevTag = 0;
            prev.findTagInElementName(&prevTag, nullptr, nullptr, nullptr, true);
            if (prevTag == inputTag || prevTag == -inputTag) {
                name = mappedName;
            }
        }
    }

    if (sids && this->hasher) {
        name = hashElementName(name, *sids);
        if (!forceTag && (tag == 0) && (ss.tellp() != 0)) {
            forceTag = true;
        }
    }
    if (forceTag || (tag != 0)) {
        assert(element_type);
        auto pos = ss.tellp();
        boost::io::ios_flags_saver ifs(ss);
        ss << POSTFIX_TAG << std::hex;
        if (tag < 0) {
            ss << '-' << -tag;
        }
        else if (tag != 0) {
            ss << tag;
        }
        assert(pos >= 0);
        if (pos != 0) {
            ss << ':' << pos;
        }
        ss << ',' << element_type;
    }
    name += ss.str();
}

MappedName ElementMap::hashElementName(const MappedName& name, ElementIDRefs& sids) const
{
    if (!this->hasher || !name) {
        return name;
    }
    if (name.find(ELEMENT_MAP_PREFIX) < 0) {
        return name;
    }
    App::StringIDRef sid = this->hasher->getID(name, sids);
    const auto& related = sid.relatedIDs();
    if (related == sids) {
        sids.clear();
        sids.push_back(sid);
    }
    else {
        ElementIDRefs tmp;
        tmp.push_back(sid);
        for (auto& checkSID : sids) {
            if (related.indexOf(checkSID) < 0) {
                tmp.push_back(checkSID);
            }
        }
        sids = tmp;
    }
    return MappedName(sid.toString());
}

MappedName ElementMap::dehashElementName(const MappedName& name) const
{
    if (name.empty()) {
        return name;
    }
    if (!this->hasher) {
        return name;
    }
    auto id = App::StringID::fromString(name.toRawBytes());
    if (!id) {
        return name;
    }
    auto sid = this->hasher->getID(id);
    if (!sid) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE)) {
            FC_WARN("failed to find hash id " << id);// NOLINT
        }
        else {
            FC_LOG("failed to find hash id " << id);// NOLINT
        }
        return name;
    }
    if (sid.isHashed()) {
        FC_LOG("cannot de-hash id " << id);// NOLINT
        return name;
    }
    MappedName ret(
        sid.toString());// FIXME .toString() was missing in original function. is this correct?
    FC_TRACE("de-hash " << name << " -> " << ret);// NOLINT
    return ret;
}

MappedName ElementMap::renameDuplicateElement(int index, const IndexedName& element,
                                              const IndexedName& element2, const MappedName& name,
                                              ElementIDRefs& sids, long masterTag) const
{
    int idx {0};
#ifdef FC_DEBUG
    idx = index;
#else
    static std::random_device _RD;
    static std::mt19937 _RGEN(_RD());
    static std::uniform_int_distribution<> _RDIST(1, 10000);
    (void)index;
    idx = _RDIST(_RGEN);
#endif
    std::ostringstream ss;
    ss << ELEMENT_MAP_PREFIX << 'D' << std::hex << idx;
    MappedName renamed(name);
    encodeElementName(element.getType()[0], renamed, ss, &sids, masterTag);
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        FC_WARN("duplicate element mapping '"// NOLINT
                << name << " -> " << renamed << ' ' << element << '/' << element2);
    }
    return renamed;
}

void ElementMap::erase(const MappedName& name)
{
    auto it = this->mappedNames.find(name);
    if (it == this->mappedNames.end()) {
        return;
    }
    MappedNameRef* ref = findMappedRef(it->second);
    if (!ref) {
        return;
    }
    ref->erase(name);
    this->mappedNames.erase(it);
}

void ElementMap::erase(const IndexedName& idx)
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end()) {
        return;
    }
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size()) {
        return;
    }
    auto& ref = indices.names[idx.getIndex()];
    for (auto* nameRef = &ref; nameRef; nameRef = nameRef->next.get()) {
        this->mappedNames.erase(nameRef->name);
    }
    ref.clear();
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
    auto nameIter = mappedNames.find(name);
    if (nameIter == mappedNames.end()) {
        if (childElements.isEmpty()) {
            return IndexedName();
        }

        int len = 0;
        if (name.findTagInElementName(nullptr, &len, nullptr, nullptr, false, false) < 0) {
            return IndexedName();
        }
        QByteArray key = name.toRawBytes(len);
        auto it = this->childElements.find(key);
        if (it == this->childElements.end()) {
            return IndexedName();
        }

        const auto& child = *it.value().childMap;
        IndexedName res;

        MappedName childName = MappedName::fromRawData(name, 0, len);
        if (child.elementMap) {
            res = child.elementMap->find(childName, sids);
        }
        else {
            res = childName.toIndexedName();
        }

        if (res && boost::equals(res.getType(), child.indexedName.getType())
            && child.indexedName.getIndex() <= res.getIndex()
            && child.indexedName.getIndex() + child.count > res.getIndex()) {
            res.setIndex(res.getIndex() + it.value().childMap->offset);
            return res;
        }

        return IndexedName();
    }

    if (sids) {
        const MappedNameRef* ref = findMappedRef(nameIter->second);
        for (; ref; ref = ref->next.get()) {
            if (ref->name == name) {
                if (sids->empty()) {
                    *sids = ref->sids;
                }
                else {
                    *sids += ref->sids;
                }
                break;
            }
        }
    }
    return nameIter->second;
}

MappedName ElementMap::find(const IndexedName& idx, ElementIDRefs* sids) const
{
    if (!idx) {
        return {};
    }

    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end()) {
        return {};
    }

    auto& indices = iter->second;
    if (idx.getIndex() < (int)indices.names.size()) {
        const MappedNameRef& ref = indices.names[idx.getIndex()];
        if (ref.name) {
            if (sids) {
                if (!sids->size()) {
                    *sids = ref.sids;
                }
                else {
                    *sids += ref.sids;
                }
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
        if (child.elementMap) {
            name = child.elementMap->find(childIdx, sids);
        }
        else {
            name = MappedName(childIdx);
        }
        if (name) {
            name += child.postfix;
            return name;
        }
    }
    return {};
}

std::vector<std::pair<MappedName, ElementIDRefs>> ElementMap::findAll(const IndexedName& idx) const
{
    std::vector<std::pair<MappedName, ElementIDRefs>> res;
    if (!idx) {
        return res;
    }

    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end()) {
        return res;
    }

    auto& indices = iter->second;
    if (idx.getIndex() < (int)indices.names.size()) {
        const MappedNameRef& ref = indices.names[idx.getIndex()];
        int count = 0;
        for (auto nameRef = &ref; nameRef; nameRef = nameRef->next.get()) {
            if (nameRef->name) {
                ++count;
            }
        }
        if (count != 0) {
            res.reserve(count);
            for (auto nameRef = &ref; nameRef; nameRef = nameRef->next.get()) {
                if (nameRef->name) {
                    res.emplace_back(nameRef->name, nameRef->sids);
                }
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
            for (auto& v : res) {
                v.first += child.postfix;
            }
        }
        else {
            res.emplace_back(MappedName(childIdx) + child.postfix, ElementIDRefs());
        }
    }

    return res;
}

const MappedNameRef* ElementMap::findMappedRef(const IndexedName& idx) const
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end()) {
        return nullptr;
    }
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size()) {
        return nullptr;
    }
    return &indices.names[idx.getIndex()];
}

MappedNameRef* ElementMap::findMappedRef(const IndexedName& idx)
{
    auto iter = this->indexedNames.find(idx.getType());
    if (iter == this->indexedNames.end()) {
        return nullptr;
    }
    auto& indices = iter->second;
    if (idx.getIndex() >= (int)indices.names.size()) {
        return nullptr;
    }
    return &indices.names[idx.getIndex()];
}

MappedNameRef& ElementMap::mappedRef(const IndexedName& idx)
{
    assert(idx);
    auto& indices = this->indexedNames[idx.getType()];
    if (idx.getIndex() >= (int)indices.names.size()) {
        indices.names.resize(idx.getIndex() + 1);
    }
    return indices.names[idx.getIndex()];
}

bool ElementMap::hasChildElementMap() const
{
    return !childElements.empty();
}

void ElementMap::hashChildMaps(long masterTag)
{
    if (childElements.empty() || !this->hasher) {
        return;
    }
    std::ostringstream ss;
    for (auto& indexedNameIndexedElements : this->indexedNames) {
        for (auto& indexedChild : indexedNameIndexedElements.second.children) {
            auto& child = indexedChild.second;
            int len = 0;
            long tag = 0;
            int pos = MappedName::fromRawData(child.postfix)
                          .findTagInElementName(&tag, &len, nullptr, nullptr, false, false);
            // TODO: What is this 10?
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
    if (!res.second) {
        return;
    }

    for (auto& indexedName : this->indexedNames) {
        addPostfix(QByteArray::fromRawData(indexedName.first,
                                           static_cast<int>(qstrlen(indexedName.first))),
                   postfixMap,
                   postfixes);

        for (auto& childPair : indexedName.second.children) {
            auto& child = childPair.second;
            if (child.elementMap) {
                child.elementMap->collectChildMaps(childMapSet, childMaps, postfixMap, postfixes);
            }
        }
    }

    for (auto& mappedName : this->mappedNames) {
        addPostfix(mappedName.first.constPostfix(), postfixMap, postfixes);
    }

    childMaps.push_back(this);
    res.first->second = (int)childMaps.size();
}

void ElementMap::addChildElements(long masterTag, const std::vector<MappedChildElements>& children)
{
    std::ostringstream ss;
    ss << std::hex;

    // To avoid possibly very long recursive child map lookup, resulting very
    // long mapped names, we try to resolve the grand child map now.
    std::vector<MappedChildElements> expansion;
    for (auto it = children.begin(); it != children.end(); ++it) {
        auto& child = *it;
        if (!child.elementMap || child.elementMap->childElements.empty()) {
            if (!expansion.empty()) {
                expansion.push_back(child);
            }
            continue;
        }
        auto& indices = child.elementMap->indexedNames[child.indexedName.getType()];
        if (indices.children.empty()) {
            if (!expansion.empty()) {
                expansion.push_back(child);
            }
            continue;
        }

        // Note that it is allowable to have both mapped names and child map. We
        // may have to split the current child mapping into pieces.

        int start = child.indexedName.getIndex();
        int end = start + child.count;
        for (auto iter = indices.children.upper_bound(start); iter != indices.children.end();
             ++iter) {
            auto& grandchild = iter->second;
            int istart = grandchild.indexedName.getIndex() + grandchild.offset;
            int iend = istart + grandchild.count;
            if (end <= istart) {
                break;
            }
            if (istart >= end) {
                if (!expansion.empty()) {
                    expansion.push_back(child);
                    expansion.back().indexedName.setIndex(start);
                    expansion.back().count = end - start;
                }
                break;
            }
            if (expansion.empty()) {
                const int extra {10};
                expansion.reserve(children.size() + extra);
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
            else {
                istart = start;
            }

            if (iend > end) {
                iend = end;
            }

            entry->indexedName.setIndex(istart - grandchild.offset);
            entry->count = iend - istart;
            entry->offset += grandchild.offset;
            entry->elementMap = grandchild.elementMap;
            entry->sids += grandchild.sids;
            if (grandchild.postfix.size() != 0) {
                if ((entry->postfix.size() != 0)
                    && !entry->postfix.startsWith(ELEMENT_MAP_PREFIX)) {
                    entry->postfix = grandchild.postfix + ELEMENT_MAP_PREFIX + entry->postfix;
                }
                else {
                    entry->postfix = grandchild.postfix + entry->postfix;
                }
            }

            start = iend;
            if (start >= end) {
                break;
            }
        }
        if (!expansion.empty() && start < end) {
            expansion.push_back(child);
            expansion.back().indexedName.setIndex(start);
            expansion.back().count = end - start;
        }
    }

    for (auto& child : expansion.empty() ? children : expansion) {
        if (!child.indexedName || (child.count == 0)) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_ERR("invalid mapped child element");// NOLINT
            }
            continue;
        }

        ss.str("");
        MappedName tmp;

        ChildMapInfo* entry = nullptr;

        // do child mapping only if the child element count >= 5
        const int threshold {5};
        if (child.count >= threshold || !child.elementMap) {
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
                    if ((child.tag == 0) || child.tag == masterTag) {
                        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                            FC_WARN("unmapped element");// NOLINT
                        }
                        continue;
                    }
                    name = MappedName(childIdx);
                }
                ss.str("");
                encodeElementName(
                    idx[0], name, ss, &sids, masterTag, child.postfix.constData(), child.tag);
                setElementName(idx, name, masterTag, &sids);
            }
            continue;
        }

        if (entry->index != 1) {
            // There is some ambiguity in child mapping. We need some
            // additional postfix for disambiguation. NOTE: We are not
            // using ComplexGeoData::indexPostfix() so we don't confuse
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
                FC_ERR("duplicate mapped child element");// NOLINT
                continue;
            }
        }

        auto& indices = this->indexedNames[child.indexedName.getType()];
        auto res = indices.children.emplace(
            child.indexedName.getIndex() + child.offset + child.count, child);
        if (!res.second) {
            if (!entry->childMap) {
                this->childElements.remove(tmp.toBytes());
            }
            FC_ERR("duplicate mapped child element");// NOLINT
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
    for (auto& childElement : this->childElements) {
        res.push_back(*childElement.childMap);
    }
    return res;
}

std::vector<MappedElement> ElementMap::getAll() const
{
    std::vector<MappedElement> ret;
    ret.reserve(size());
    for (auto& mappedName : this->mappedNames) {
        ret.emplace_back(mappedName.first, mappedName.second);
    }
    for (auto& childElement : this->childElements) {
        auto& child = *childElement.childMap;
        IndexedName idx(child.indexedName);
        idx.setIndex(idx.getIndex() + child.offset);
        IndexedName childIdx(child.indexedName);
        for (int i = 0; i < child.count; ++i, ++idx, ++childIdx) {
            MappedName name;
            if (child.elementMap) {
                name = child.elementMap->find(childIdx);
            }
            else {
                name = MappedName(childIdx);
            }
            if (name) {
                name += child.postfix;
                ret.emplace_back(name, idx);
            }
        }
    }
    return ret;
}

long ElementMap::getElementHistory(const MappedName& name, long masterTag, MappedName* original,
                                   std::vector<MappedName>* history) const
{
    long tag = 0;
    int len = 0;
    int pos = name.findTagInElementName(&tag, &len, nullptr, nullptr, true);
    if (pos < 0) {
        if (original) {
            *original = name;
        }
        return tag;
    }
    if (!original && !history) {
        return tag;
    }

    MappedName tmp;
    MappedName& ret = original ? *original : tmp;
    if (name.startsWith(ELEMENT_MAP_PREFIX)) {
        unsigned offset = ELEMENT_MAP_PREFIX_SIZE;
        ret = MappedName::fromRawData(name, static_cast<int>(offset));
    }
    else {
        ret = name;
    }

    while (true) {
        if ((len == 0) || len > pos) {
            FC_WARN("invalid name length " << name);// NOLINT
            return 0;
        }
        bool deHashed = false;
        if (ret.startsWith(MAPPED_CHILD_ELEMENTS_PREFIX, len)) {
            int offset = (int)POSTFIX_TAG_SIZE;
            MappedName tmp2 = MappedName::fromRawData(ret, len + offset, pos - len - offset);
            MappedName postfix = dehashElementName(tmp2);
            if (postfix != tmp2) {
                deHashed = true;
                ret = MappedName::fromRawData(ret, 0, len) + postfix;
            }
        }
        if (!deHashed) {
            ret = dehashElementName(MappedName::fromRawData(ret, 0, len));
        }

        long tag2 = 0;
        pos = ret.findTagInElementName(&tag2, &len, nullptr, nullptr, true);
        if (pos < 0 || (tag2 != tag && tag2 != -tag && tag != masterTag && -tag != masterTag)) {
            return tag;
        }
        tag = tag2;
        if (history) {
            history->push_back(ret.copy());
        }
    }
}


}// Namespace Data
