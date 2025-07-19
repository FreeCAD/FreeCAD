#include "PreCompiled.h"
#include <cctype>
#include <string>
#ifndef _PreComp_
#include <unordered_map>
#ifndef FC_DEBUG
#include <random>
#include <chrono>
#endif
#endif

#include "ElementMap.h"
#include "ElementNamingUtils.h"

#include "App/Application.h"
#include "Base/Console.h"
#include "Document.h"
#include "DocumentObject.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>


FC_LOG_LEVEL_INIT("ElementMap", true, 2);  // NOLINT

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

void ElementMap::save(std::ostream& stream,
                      int index,
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
                    FC_ERR("Invalid child element map");  // NOLINT
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

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasherRef, std::istream& stream)
{
    const char* msg = "Invalid element map";

    unsigned id = 0;
    int count = 0;
    std::string tmp;
    if (!(stream >> id >> tmp >> count) || tmp != "PostfixCount") {
        FC_THROWM(Base::RuntimeError, msg);  // NOLINT
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
    constexpr int practicalMaximum {(1 << 30) / sizeof(ElementMapPtr)};  // a 1GB child map vector: almost certainly a bug
    if (!(stream >> tmp >> count) || tmp != "MapCount" || count == 0 || count > practicalMaximum) {
        FC_THROWM(Base::RuntimeError, msg);  // NOLINT
    }
    childMaps.reserve(count - 1);
    for (int i = 0; i < count - 1; ++i) {
        childMaps.push_back(
            std::make_shared<ElementMap>()->restore(hasherRef, stream, childMaps, postfixes));
    }

    return restore(hasherRef, stream, childMaps, postfixes);
}

ElementMapPtr ElementMap::restore(::App::StringHasherRef hasherRef,
                                  std::istream& stream,
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
        FC_THROWM(Base::RuntimeError, msg);  // NOLINT
    }
    constexpr int maxTypeCount(1000);
    if (typeCount < 0 || typeCount > maxTypeCount) {
        FC_THROWM(Base::RuntimeError, "Bad type count in element map, ignoring map");  // NOLINT
    }

    auto& map = _idToElementMap[id];
    if (map) {
        while (tmp != "EndMap") {
            if (!std::getline(stream, tmp)) {
                FC_THROWM(Base::RuntimeError, "unexpected end of child element map");  // NOLINT
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
            FC_THROWM(Base::RuntimeError, "missing element type");  // NOLINT
        }
        IndexedName idx(tmp.c_str(), 1);

        if (!(stream >> tmp >> outerCount) || tmp != "ChildCount") {
            FC_THROWM(Base::RuntimeError, "missing element child count");  // NOLINT
        }

        auto& indices = this->indexedNames[idx.getType()];
        for (int j = 0; j < outerCount; ++j) {
            int cIndex = 0;
            int offset = 0;
            int count = 0;
            long tag = 0;
            int mapIndex = 0;
            if (!(stream >> cIndex >> offset >> count >> tag >> mapIndex >> tmp)) {
                FC_THROWM(Base::RuntimeError, "Invalid element child");  // NOLINT
            }
            if (cIndex < 0) {
                FC_THROWM(Base::RuntimeError, "Invalid element child index");  // NOLINT
            }
            if (offset < 0) {
                FC_THROWM(Base::RuntimeError, "Invalid element child offset");  // NOLINT
            }
            if (mapIndex >= index || mapIndex < 0 || mapIndex > (int)childMaps.size()) {
                FC_THROWM(Base::RuntimeError, "Invalid element child map index");  // NOLINT
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
                FC_THROWM(Base::RuntimeError, "Invalid element child string id");  // NOLINT
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
            FC_THROWM(Base::RuntimeError, "missing element name outerCount");  // NOLINT
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
                    FC_THROWM(Base::RuntimeError, "Failed to read element name");  // NOLINT
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
                    FC_THROWM(Base::RuntimeError, "Invalid element entry");  // NOLINT
                }

                int offset = 1;
                ::App::StringID::IndexID prefixID {};
                prefixID.id = 0;

                switch (tokens[0][0]) {
                    case ':': {
                        if (tokens.size() < 3) {
                            FC_THROWM(Base::RuntimeError, "Invalid element entry");  // NOLINT
                        }
                        ++offset;
                        long elementNameIndex = strtol(tokens[0].c_str() + 1, nullptr, hexBase);
                        if (elementNameIndex <= 0 || elementNameIndex > (int)postfixes.size()) {
                            FC_THROWM(Base::RuntimeError, "Invalid element name index");  // NOLINT
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
                        FC_THROWM(Base::RuntimeError, "Invalid element name marker");  // NOLINT
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
        FC_WARN(hasherWarn);  // NOLINT
    }
    if (hasherIDWarn) {
        FC_WARN(hasherIDWarn);  // NOLINT
    }
    if (postfixWarn) {
        FC_WARN(postfixWarn);  // NOLINT
    }
    if (childSIDWarn) {
        FC_WARN(childSIDWarn);  // NOLINT
    }

    if (!(stream >> tmp) || tmp != "EndMap") {
        FC_THROWM(Base::RuntimeError, "unexpected end of child element map");  // NOLINT
    }

    return shared_from_this();
}

MappedName ElementMap::addName(MappedName& name,
                               const IndexedName& idx,
                               const ElementIDRefs& sids,
                               bool overwrite,
                               IndexedName* existing)
{
    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        if (name.find("#") >= 0 && name.findTagInElementName() < 0) {
            FC_ERR("missing tag postfix " << name);  // NOLINT
        }
    }
    while (true) {
        if (overwrite) {
            erase(idx);
        }
        auto ret = mappedNames.insert(std::make_pair(name, idx));
        if (ret.second) {                // element just inserted did not exist yet in the map
            ret.first->first.compact();  // FIXME see MappedName.cpp
            mappedRef(idx).append(ret.first->first, sids);
            FC_TRACE(idx << " -> " << name);  // NOLINT
            return ret.first->first;
        }
        if (ret.first->second == idx) {
            FC_TRACE("duplicate " << idx << " -> " << name);  // NOLINT
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

void ElementMap::addPostfix(const QByteArray& postfix,
                            std::map<QByteArray, int>& postfixMap,
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

MappedName ElementMap::setElementName(const IndexedName& element,
                                      const MappedName& name,
                                      long masterTag,
                                      const ElementIDRefs* sid,
                                      bool overwrite)
{
    if (!this) {
        return {};
    }
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
            FC_THROWM(Base::RuntimeError, "Illegal character in mapped name: " << name);  // NOLINT
        }
    }
    for (const char* readChar = element.getType(); *readChar != 0; ++readChar) {
        char check = *readChar;
        if (check == '.' || (std::isspace((int)check) != 0)) {
            FC_THROWM(Base::RuntimeError,  // NOLINT
                      "Illegal character in element name: " << element);
        }
    }

    // Originally in ComplexGeoData::setElementName
    // LinkStable/src/App/ComplexGeoData.cpp#L1631
    // No longer possible after map separated in ElementMap.cpp

    // if(!_ElementMap)
    //     resetElementMap(std::make_shared<ElementMap>());

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
            FC_ERR("unresolved duplicate element mapping '"  // NOLINT
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
void ElementMap::encodeElementName(char element_type,
                                   MappedName& name,
                                   std::ostringstream& ss,
                                   ElementIDRefs* sids,
                                   long masterTag,
                                   const char* postfix,
                                   long tag,
                                   bool forceTag) const
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
            FC_WARN("failed to find hash id " << id);  // NOLINT
        }
        else {
            FC_LOG("failed to find hash id " << id);  // NOLINT
        }
        return name;
    }
    if (sid.isHashed()) {
        FC_LOG("cannot de-hash id " << id);  // NOLINT
        return name;
    }
    MappedName ret(sid);

    //        sid.toString());// FIXME .toString() was missing in original function. is this
    //        correct?
    // FC_TRACE("de-hash " << name << " -> " << ret);  // NOLINT
    return ret;
}

// TODO: maybe merge this into the above function? or make one public function and one private function?
// this reverses the compression of hashed persistent names.
// an example: #3d:2;:G3#3f;CUT;:H-1216:b,E --> g2;SKT;:H1215,E;FAC;:H1215:4,F;:G0;XTR;:H1215:8,F;:G3(g6;SKT;:H1213,E;:G;...
MappedName ElementMap::fullDehashElementName(const MappedName& name) const {
    std::string dehashedName;
    std::vector<std::map<std::string, std::array<int, 2>>> dehashTree;
    std::string currentTreeString = "";
    std::string selTreeString = "";
    std::string hashedString = "";
    std::string dehashedString = "";

    bool isDehashed = false;
    bool dehash = false;
    bool foundHash = false;
    int currentTreePos = 0;
    int i = 0;
    int replaceStart = -1;
    int replaceLen = -1;
    int limit = 100000;
    int limiti = 0;
    std::map<std::string, std::array<int, 2>> insertMap = std::map<std::string, std::array<int, 2>>();
    std::map<std::string, std::array<int, 2>> currentMap;
    
    insertMap[name.toString()] = std::array<int, 2>({-1, -1});
    dehashTree.push_back(insertMap);

    while(!isDehashed && limiti < limit) {
        currentMap = dehashTree[currentTreePos];
        currentTreeString = currentMap.begin()->first;
        limiti++;

        if(!dehash) {
            if(i >= currentTreeString.size()) {
                if(foundHash) {
                    dehash = true;
                    foundHash = false;
                } else {
                    if(currentTreePos == 0) {
                        isDehashed = true;
                        dehashedName = currentTreeString;
                        break;
                    }

                    replaceStart = dehashTree[currentTreePos - 1].begin()->second[0];
                    replaceLen = dehashTree[currentTreePos - 1].begin()->second[1];

                    if(replaceStart != -1 && replaceLen != -1) {
                        selTreeString = dehashTree[currentTreePos - 1].begin()->first;
                        dehashTree[currentTreePos - 1].erase(selTreeString);

                        selTreeString.replace(replaceStart, replaceLen, currentTreeString);
                        dehashTree[currentTreePos - 1][selTreeString] = std::array<int, 2>({replaceStart, replaceLen});

                        dehashTree.erase(dehashTree.begin() + currentTreePos);
                        i = 0;
                        currentTreePos--;
                    }
                }
                continue;
            }

            if(currentTreeString[i] == '#' && !foundHash) {
                hashedString = "";
                foundHash = true;
                dehash = false;

                dehashTree[currentTreePos][currentTreeString] = std::array<int, 2>({i, -1});
            } else if((currentTreeString[i] == ',' || currentTreeString[i] == ';') && foundHash) {
                foundHash = false;
                dehash = true;

                dehashTree[currentTreePos][currentTreeString] = std::array<int, 2>({currentMap[currentTreeString][0], static_cast<int>(hashedString.length())});
            }

            if(foundHash) {
                hashedString += currentTreeString[i];
            }

            i++;
        } else {
            if(hashedString.size() > 1) {
                dehashedString = dehashElementName(MappedName(hashedString)).toString();

                if(dehashedString != hashedString) {
                    currentTreePos++;

                    i = 0;

                    insertMap = std::map<std::string, std::array<int, 2>>();
                    insertMap[dehashedString] = std::array<int, 2>({-1, -1});
                    dehashTree.push_back(insertMap);
                } else {
                    hashedString.pop_back();
                    continue;
                }
            }
            dehash = false;
            foundHash = false;
        }
    }

    // cleanup to avoid memory leaks or unnecessary memory usage
    dehashTree.shrink_to_fit();
    currentTreeString.shrink_to_fit();
    selTreeString.shrink_to_fit();
    hashedString.shrink_to_fit();
    dehashedString.shrink_to_fit();
    insertMap.clear();
    currentMap.clear();
 
    return MappedName(dehashedName);
}

MappedName ElementMap::renameDuplicateElement(int index,
                                              const IndexedName& element,
                                              const IndexedName& element2,
                                              const MappedName& name,
                                              ElementIDRefs& sids,
                                              long masterTag) const
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
        FC_WARN("duplicate element mapping '"  // NOLINT
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

// this method finds the percent similarity of two strings
// used for the "score" of an element that passes the checks in `complexFind`
double ElementMap::percentSimilarity(const std::string& a, const std::string& b) const {
    int n = a.size();
    int m = b.size();
    if (n == 0 && m == 0) return 0;

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min(
                std::min(dp[i - 1][j] + 1,     // deletion
                         dp[i][j - 1] + 1),    // insertion
                dp[i - 1][j - 1] + cost        // substitution
            );
        }
    }

    int editDist = dp[n][m];
    int maxLen = std::max(n, m);
    double similarity = (1.0 - static_cast<double>(editDist) / maxLen);
    return similarity;
}

// this splits a `name` string (a persistent name that tracks history for a element)
// you can get one with the `toString` method in MappedName
// this might need a refactor to make it simpler and easier to understand
std::vector<std::pair<std::string, char>> ElementMap::splitNameIntoSections(
        const std::string &name,
        const bool &filterSections,
        const bool &findSmallSections,
        std::map<int, std::vector<std::string>> *parenMapPtr,
        std::vector<int> *postfixNumbersPtr,
        std::vector<std::pair<std::string, char>> *outputVecPtr) const {
        
    std::vector<char> filterPostFixes = {'M', 'D'};
    std::vector<char> postFixTypes = {'G', 'M', 'U'};

    std::vector<std::pair<std::string, char>> result;
    std::string current;
    std::string parenthesesCurrent;
    std::vector<char>::iterator it;
    std::vector<char>::iterator itAllowed;
    std::vector<int> postfixNumbers;
    bool itAllowedUsed = false;
    std::map<int, std::vector<std::string>> parenthesesMap;
    bool itUsed = false;
    int lastColon = -1;
    bool hasBadPostFix = false;
    int parenthesesLevel = 0;
    int parenthesesLevelBuffer = 1;
    std::string postfixNumber;

    bool foundPostfixNumber = false;
    std::string currentNumber;
    char currentPostfix = '-';
    for (size_t i = 0; i < name.size(); ++i) {
        if (foundPostfixNumber && i + 2 < (int) name.size()) {
            if (std::isdigit(name[i + 2]) && parenthesesLevel == 0) {
                postfixNumber.clear();
                postfixNumber.push_back(name[i + 2]);
            } else {
                foundPostfixNumber = false;

                if (!postfixNumber.empty()) {
                    postfixNumbers.push_back(std::stoi(postfixNumber));
                }
            }
        }

        if(name[i] == ':' && parenthesesLevel == 0) {
            lastColon = i;
            if (lastColon + 1 < (int)name.size()) {
                itAllowed = std::find(postFixTypes.begin(), postFixTypes.end(), name[lastColon + 1]);
                itAllowedUsed = true;

                it = std::find(filterPostFixes.begin(), filterPostFixes.end(), name[lastColon + 1]);
                itUsed = true;

                if (itAllowed != postFixTypes.end() && (!filterSections || it == filterPostFixes.end())) {
                    currentPostfix = name[i + 1];

                    if (i + 2 < (int)name.size()) {
                        if (std::isdigit(name[i + 2])) {
                            foundPostfixNumber = true;
                            postfixNumber.push_back(name[i + 2]);
                        } else {
                            foundPostfixNumber = false;
                            postfixNumbers.push_back(-1);
                        }
                    }
                }
            } else {
                currentPostfix = '-';
            }
        } else if(name[i] == '(') {
            if(parenthesesLevel == 0) {
                parenthesesCurrent.clear();
            }

            parenthesesLevel++;
        } else if(name[i] == ')' && parenthesesLevel != 0) {
            parenthesesLevel--;
        }

        if (parenthesesLevel > 0) {
            hasBadPostFix = filterSections ? (lastColon + 1 >= 0 && it != filterPostFixes.end()) : false;

            if((name[i] == '(' || name[i] == ')') || (name[i] == ';' && (i + 1 >= name.size() || name[i + 1] != ':'))) {
                if(parenthesesCurrent.size() != 0) {
                    parenthesesMap[parenthesesLevelBuffer].push_back(parenthesesCurrent);
                }

                parenthesesLevelBuffer = parenthesesLevel;
                parenthesesCurrent.clear();

            } else if(i < name.size() && !hasBadPostFix) {
                parenthesesCurrent += name[i];
            }
        }

        if(parenthesesLevel == 0) {
            if((name[i] == ';' && (i + 1 >= name.size() || (!findSmallSections && name[i + 1] != ':'))) || (findSmallSections && name[i + 1] == ':')) {
                if(!hasBadPostFix) {
                    result.emplace_back(current, currentPostfix);

                    if (outputVecPtr) outputVecPtr->emplace_back(current, currentPostfix);
                }

                current.clear();

                it = std::find(filterPostFixes.begin(), filterPostFixes.end(), name[lastColon + 1]);
                itUsed = true;
                hasBadPostFix = filterSections ? (lastColon + 1 >= 0 && it != filterPostFixes.end()) : false;
            } else {
                current += name[i];
            }
        }
    }

    if(!hasBadPostFix) {
        if (!current.empty()) {
            result.emplace_back(current, currentPostfix);
        }
        if (outputVecPtr) outputVecPtr->emplace_back(current, currentPostfix);
    }

    if(postfixNumbersPtr != nullptr) {
        *postfixNumbersPtr = postfixNumbers;
    }
    
    if(parenMapPtr != nullptr) {
        *parenMapPtr = parenthesesMap;
    }

    // cleanup to avoid memory leaks or unnecessary memory usage
    if(itUsed) filterPostFixes.erase(it);
    if(itAllowedUsed) postFixTypes.erase(itAllowed);

    return result;
}

std::vector<std::string> ElementMap::findGeometryOpCodes(const std::vector<std::string> &name) const {
    std::vector<std::string> opCodes = {"XTR", "FAC", "SKT", "FUS", "CUT", "PSM"};
    std::vector<std::string> result;
    std::vector<std::string>::iterator it;
    bool itUsed = false;
    std::string checkStr;
    for(auto &section : name) {
        std::cout << section << "\n";

        for (size_t i = 0; i < section.size(); ++i) {
            checkStr.clear();
            checkStr.push_back(section[i]);
            checkStr.push_back(section[i+1]);
            checkStr.push_back(section[i+2]);

            it = std::find(opCodes.begin(), opCodes.end(), checkStr);
            itUsed = true;

            if (it != opCodes.end()) {
                result.push_back(*it);
            }
        }
    }

    // cleanup to avoid memory leaks or unnecessary memory usage
    if(itUsed) opCodes.erase(it);

    return result;
}

ElementMap::ComplexFindData ElementMap::compileComplexFindData(const MappedName& name) const {
    ComplexFindData data;
    data.isHashed = (this->hasher != nullptr);
    data.originalName = name.toString();
    if (data.isHashed) {
        data.originalName = fullDehashElementName(name).toString();
    }
    
    data.unfilteredMajorSections = splitNameIntoSections(data.originalName, false, false, nullptr, &data.unfilteredPostfixNumbers);
    data.majorSections = splitNameIntoSections(data.originalName, true, false, &data.parenthesesMap, &data.postfixNumbers);
    // looseSections: remove the first entry from majorSections
    data.looseSections = data.majorSections;
    if (!data.looseSections.empty()) {
        auto it = data.looseSections.begin();
        data.looseSections.erase(it);
    }

    // geometryOpCodes: extract opcodes from section names (e.g., "XTR", "FAC", etc.)
    static const std::vector<std::string> opCodes = {"XTR", "FAC", "SKT", "FUS", "CUT", "PSM"};
    data.geometryOpCodes.clear();
    for (const auto& kv : data.majorSections) {
        for (const auto& code : opCodes) {
            if (kv.first.find(code) != std::string::npos) {
                data.geometryOpCodes.push_back(code);
                break;
            }
        }
    }
    data.geometryDefSections.clear();
    if (!data.geometryOpCodes.empty() && data.majorSections.size() >= 2) {
        auto it = data.majorSections.begin();
        ++it;
        if (it != data.majorSections.end()) {
            auto defSections = splitNameIntoSections(it->first, false, true, nullptr);
            for (const auto& section : defSections) {
                data.geometryDefSections.push_back(section.first);
            }
        }
    }

    return data;
}

// TODO: allow modifier sections to be used if the same amount of them are found in both name (in the same positions??)
// TODO: check if the postfix tags either have numbers next to them in both instances at the same index or do not have numbers.
IndexedName ElementMap::complexFind(const MappedName& name) const
{
    auto startTime = std::chrono::high_resolution_clock::now();

    IndexedName foundIndexedName = IndexedName();
    IndexedName defIN = IndexedName();
    double foundNameScore = 0;

    ComplexFindData originalData = compileComplexFindData(name);
    ComplexFindData originalDataStart = originalData;

    std::string originalName = originalData.originalName;

    std::vector<std::string> origLooseKeys;
    std::vector<std::string> origLooseKeysStart;
    std::vector<std::string> loopLooseKeys;
    std::vector<std::string> origMajorKeys;
    std::vector<std::string> origMajorKeysStart;
    std::vector<std::string> loopMajorKeys;
    std::vector<std::string> looseLargestVec;
    std::vector<std::string> looseSmallestVec;
    __gnu_cxx::__normal_iterator<std::pair<std::basic_string<char>, char> *, std::vector<std::pair<std::basic_string<char>, char>>> origUnfilteredEnd = std::prev(originalData.unfilteredMajorSections.end());
    __gnu_cxx::__normal_iterator<std::pair<std::basic_string<char>, char> *, std::vector<std::pair<std::basic_string<char>, char>>> loopCheckUnfilteredEnd;
    std::string largeVecSection;
    std::string smallVecSection;
    std::vector<double> avgDifferenceVec;
    double avgDifferenceNum = 0;
    double score = 0;
    ComplexFindData loopCheckData;
    size_t geoDefMinSize = 0;

    bool initialSecCheck = false;
    bool geomDefsSame = false;
    bool elementTypeIsSame = false;
    bool innerSectionTest = false;
    bool sameSizeCheck = false;
    bool strictOccurenceTolCheck = false;
    bool strictOccurencePercentCheck = false;
    bool avgDifferenceCheck = false;
    bool postFixNumCheck = false;
    const double avgDifferencePassConst = .65;
    int looseSectionsMatches = 0;

    if(!originalData.majorSections.empty()) {
        origLooseKeys.clear();
        for (const auto& kv : originalData.looseSections) origLooseKeys.push_back(kv.first);
        origLooseKeysStart = origLooseKeys;

        origMajorKeys.clear();
        for (const auto& kv : originalData.majorSections) origMajorKeys.push_back(kv.first);
        origMajorKeysStart = origMajorKeys;

        for(const auto& loopName : mappedNames) {
            originalData = originalDataStart;
            origMajorKeys = origMajorKeysStart;
            origLooseKeys = origLooseKeysStart;

            loopCheckData = compileComplexFindData(loopName.first);
            if(loopCheckData.majorSections.empty()) continue;

            loopLooseKeys.clear();
            for (const auto& kv : loopCheckData.looseSections) loopLooseKeys.push_back(kv.first);

            loopCheckUnfilteredEnd = std::prev(loopCheckData.unfilteredMajorSections.end());

            if (origUnfilteredEnd->second == 'M' &&
                loopCheckUnfilteredEnd->second == 'M') {

                origMajorKeys.push_back(origUnfilteredEnd->first);
                origLooseKeys.push_back(origUnfilteredEnd->first);

                loopMajorKeys.push_back(loopCheckUnfilteredEnd->first);
                loopLooseKeys.push_back(loopCheckUnfilteredEnd->first);

                if(originalData.unfilteredPostfixNumbers.size() != 0 && loopCheckData.unfilteredPostfixNumbers.size() != 0) {
                    originalData.postfixNumbers.push_back(originalData.unfilteredPostfixNumbers[originalData.unfilteredPostfixNumbers.size() - 1]);
                    loopCheckData.postfixNumbers.push_back(loopCheckData.unfilteredPostfixNumbers[loopCheckData.unfilteredPostfixNumbers.size() - 1]);
                }
            }

            looseLargestVec = origLooseKeys;
            looseSmallestVec = loopLooseKeys;
            if(loopLooseKeys.size() > origLooseKeys.size()) {
                looseLargestVec = loopLooseKeys;
                looseSmallestVec = origLooseKeys;
            }

            geoDefMinSize = std::min(originalData.geometryDefSections.size(), loopCheckData.geometryDefSections.size());
            if(geoDefMinSize > 1) {
                geomDefsSame = true;
                for (size_t i = 0; i < geoDefMinSize; ++i) {
                    if (originalData.geometryDefSections[i] != loopCheckData.geometryDefSections[i]) {
                        geomDefsSame = false;
                        break;
                    }
                }
            }

            loopMajorKeys.clear();
            for (const auto& kv : loopCheckData.majorSections) loopMajorKeys.push_back(kv.first);

            initialSecCheck = (!origMajorKeys.empty() && !loopMajorKeys.empty() && origMajorKeys[0] == loopMajorKeys[0]) 
                && ((!originalData.geometryOpCodes.empty() && !loopCheckData.geometryOpCodes.empty() && originalData.geometryOpCodes[0] == loopCheckData.geometryOpCodes[0]
                && originalData.geometryOpCodes[0] != "SKT") || (((origMajorKeys.size() < 2 || loopMajorKeys.size() < 2) || (origMajorKeys[1] == loopMajorKeys[1] 
                    || (originalData.geometryDefSections.size() >= 2 && loopCheckData.geometryDefSections.size() >= 2 && geomDefsSame)))
                && (originalData.geometryOpCodes.size() < 1 || loopCheckData.geometryOpCodes.size() < 1 || originalData.geometryOpCodes.back() == loopCheckData.geometryOpCodes.back())));

            elementTypeIsSame = originalName[originalName.size() - 1] == loopCheckData.originalName[loopCheckData.originalName.size() - 1];
            sameSizeCheck = originalData.majorSections.size() == loopCheckData.majorSections.size();

            if(originalData.parenthesesMap.size() == 0 || loopCheckData.parenthesesMap.size() == 0) {
                innerSectionTest = true;
            } else if(originalData.parenthesesMap[1].size() != 0 && loopCheckData.parenthesesMap[1].size() != 0) {
                if(originalData.parenthesesMap[1][0] == loopCheckData.parenthesesMap[1][0]) {
                    innerSectionTest = true;
                } else {
                    innerSectionTest = false;
                }
            } else {
                innerSectionTest = false;
            }

            if (originalData.postfixNumbers.size() == loopCheckData.postfixNumbers.size()) {
                postFixNumCheck = true;

                if (!originalData.postfixNumbers.empty() && !loopCheckData.postfixNumbers.empty()) {
                    for(int i = 0; i < originalData.postfixNumbers.size(); i++) {
                        if (originalData.postfixNumbers[i] == -1 && loopCheckData.postfixNumbers[i] == -1) {
                            continue;
                        } else if(originalData.postfixNumbers[i] != -1 && loopCheckData.postfixNumbers[i] != -1) {
                            continue;
                        } else {
                            postFixNumCheck = false;
                            break;
                        }
                    }
                }
            } else {
                postFixNumCheck = false;
            }

            if(!initialSecCheck || !elementTypeIsSame || !sameSizeCheck || !innerSectionTest || !postFixNumCheck) continue;

            looseSectionsMatches = 0;
            for (const auto& element1 : origLooseKeys) {
                for (const auto& element2 : loopLooseKeys) {
                    if(element1 == element2) looseSectionsMatches++;
                }
            }

            strictOccurenceTolCheck = looseSectionsMatches >= (looseLargestVec.size() - 1);

            if(looseSectionsMatches != 0 && looseLargestVec.size() != 0) {
                strictOccurencePercentCheck = (looseSectionsMatches / static_cast<double>(looseLargestVec.size())) > .5;
            } else {
                strictOccurencePercentCheck = false;
            }

            avgDifferenceVec.clear();
            avgDifferenceNum = 0;

            for(int i = 0; i < static_cast<int>(looseLargestVec.size()); i++) {
                largeVecSection = looseLargestVec[i];
                if(looseSmallestVec.size() > static_cast<size_t>(i)) {
                    smallVecSection = looseSmallestVec[i];
                } else {
                    smallVecSection = "";
                }
                avgDifferenceVec.push_back(percentSimilarity(largeVecSection, smallVecSection));
            }

            for(auto &percent : avgDifferenceVec) {
                avgDifferenceNum += percent;
            }

            avgDifferenceNum /= avgDifferenceVec.size();
            avgDifferenceCheck = avgDifferenceNum >= avgDifferencePassConst;

            if(initialSecCheck && sameSizeCheck && elementTypeIsSame && innerSectionTest && postFixNumCheck && (strictOccurenceTolCheck || strictOccurencePercentCheck || avgDifferenceCheck)) {
                score = percentSimilarity(originalName, loopCheckData.originalName);
                if(score > foundNameScore) {
                    foundIndexedName = loopName.second;
                }
            }
        }
    }

    if(foundIndexedName != defIN) {
        FC_WARN("Complex check found element: " << foundIndexedName.toString());
    }

    // cleanup to avoid memory leaks or unnecessary memory usage
    origLooseKeys.clear();
    loopLooseKeys.clear();
    origMajorKeys.clear();
    loopMajorKeys.clear();
    looseLargestVec.clear();
    looseSmallestVec.clear();
    avgDifferenceVec.clear();
    avgDifferenceVec.shrink_to_fit();
    largeVecSection.clear();
    smallVecSection.clear();

    auto endTime = std::chrono::high_resolution_clock::now();
    FC_LOG("complex find time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    return foundIndexedName;
}

IndexedName ElementMap::find(const MappedName& name, ElementIDRefs* sids) const
{
    auto nameIter = mappedNames.find(name);
    if (nameIter == mappedNames.end()) {
        IndexedName res = IndexedName();
        const IndexedName def = IndexedName();

        if (this->childElements.size() == 0) {
            // complex find returns either the proper element, or an uninited indexedname,
            // meaning it can be returned directly and nothing will segfault
            return complexFind(name); // complex find does not depend on child elements, so it can be run.
            // return IndexedName();
        }

        // repeat !res check in all these IFs because we will add more complex checks
        int len = 0;
        if (name.findTagInElementName(nullptr, &len, nullptr, nullptr, false, false) < 0 && res == def) {
            return complexFind(name);
            // return IndexedName();
        }
        QByteArray key = name.toRawBytes(len);
        auto it = this->childElements.find(key);
        if (it == this->childElements.end() && res == def) {
            return complexFind(name);
            // return IndexedName();
        }

        const auto& child = *it.value().childMap;

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

        // return complexFind(name);
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
    return nameIter->second; // success
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
                MappedName postfix =
                    hashElementName(MappedName::fromRawData(child.postfix.constData(), pos),
                                    child.sids);
                ss.str("");
                ss << MAPPED_CHILD_ELEMENTS_PREFIX << postfix;
                MappedName tmp;
                encodeElementName(child.indexedName[0],
                                  tmp,
                                  ss,
                                  nullptr,
                                  masterTag,
                                  nullptr,
                                  child.tag,
                                  true);
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
                FC_ERR("invalid mapped child element");  // NOLINT
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
                            FC_WARN("unmapped element");  // NOLINT
                        }
                        continue;
                    }
                    name = MappedName(childIdx);
                }
                ss.str("");
                encodeElementName(idx[0],
                                  name,
                                  ss,
                                  &sids,
                                  masterTag,
                                  child.postfix.constData(),
                                  child.tag);
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
                FC_ERR("duplicate mapped child element");  // NOLINT
                continue;
            }
        }

        auto& indices = this->indexedNames[child.indexedName.getType()];
        auto res =
            indices.children.emplace(child.indexedName.getIndex() + child.offset + child.count,
                                     child);
        if (!res.second) {
            if (!entry->childMap) {
                this->childElements.remove(tmp.toBytes());
            }
            FC_ERR("duplicate mapped child element");  // NOLINT
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

long ElementMap::getElementHistory(const MappedName& name,
                                   long masterTag,
                                   MappedName* original,
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
            FC_WARN("invalid name length " << name);  // NOLINT
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

void ElementMap::traceElement(const MappedName& name, long masterTag, TraceCallback cb) const
{
    long encodedTag = 0;
    int len = 0;

    auto pos = name.findTagInElementName(&encodedTag, &len, nullptr, nullptr, true);
    if (cb(name, len, encodedTag, masterTag) || pos < 0) {
        return;
    }

    if (name.startsWith(POSTFIX_EXTERNAL_TAG, len)) {
        return;
    }

    std::set<long> tagSet;

    std::vector<MappedName> names;
    if (masterTag) {
        tagSet.insert(std::abs(masterTag));
    }
    if (encodedTag) {
        tagSet.insert(std::abs(encodedTag));
    }
    names.push_back(name);

    masterTag = encodedTag;
    MappedName tmp;
    bool first = true;

    // TODO: element tracing without object is inherently unsafe, because of
    // possible external linking object which means the element may be encoded
    // using external string table. Looking up the wrong table may accidentally
    // cause circular mapping, and is actually quite easy to reproduce. See
    //
    // https://github.com/realthunder/FreeCAD_assembly3/issues/968
    //
    // An arbitrary depth limit is set here to not waste time. 'tagSet' above is
    // also used for early detection of 'recursive' mapping.

    for (int index = 0; index < 50; ++index) {
        if (!len || len > pos) {
            return;
        }
        if (first) {
            first = false;
            size_t offset = 0;
            if (name.startsWith(ELEMENT_MAP_PREFIX)) {
                offset = ELEMENT_MAP_PREFIX_SIZE;
            }
            tmp = MappedName(name, offset, len);
        }
        else {
            tmp = MappedName(tmp, 0, len);
        }
        tmp = dehashElementName(tmp);
        names.push_back(tmp);
        encodedTag = 0;
        pos = tmp.findTagInElementName(&encodedTag, &len, nullptr, nullptr, true);
        if (pos >= 0 && tmp.startsWith(POSTFIX_EXTERNAL_TAG, len)) {
            break;
        }

        if (encodedTag && masterTag != std::abs(encodedTag)
            && !tagSet.insert(std::abs(encodedTag)).second) {
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("circular element mapping");
                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE)) {
                    auto doc = App::GetApplication().getActiveDocument();
                    if (doc) {
                        auto obj = doc->getObjectByID(masterTag);
                        if (obj) {
                            FC_LOG("\t" << obj->getFullName() << obj->getFullName() << "." << name);
                        }
                    }
                    for (auto& errname : names) {
                        FC_ERR("\t" << errname);
                    }
                }
            }
            break;
        }

        if (cb(tmp, len, encodedTag, masterTag) || pos < 0) {
            return;
        }
        masterTag = encodedTag;
    }
}
}  // Namespace Data
