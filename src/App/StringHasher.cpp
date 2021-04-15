/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <deque>
#include <boost/io/ios_state.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <QHash>
#include <QCryptographicHash>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <App/StringHasher.h>
#include <App/StringHasherPy.h>
#include <App/StringIDPy.h>
#include <App/DocumentParams.h>

FC_LOG_LEVEL_INIT("App",true,true)

namespace bio = boost::iostreams;
using namespace App;

///////////////////////////////////////////////////////////

struct StringIDHasher {
    std::size_t operator()(const StringID *sid) const {
        if (!sid)
            return 0;
        return qHash(sid->data(), qHash(sid->postfix()));
    }

    bool operator()(const StringID *a, const StringID *b) const {
        if (a == b)
            return true;
        if (!a || !b)
            return false;
        return a->data() == b->data() && a->postfix() == b->postfix();
    }
};

typedef boost::bimap<
            boost::bimaps::unordered_set_of<StringID*,
                                            StringIDHasher,
                                            StringIDHasher>,
            boost::bimaps::set_of<long> >
            HashMapBase;

class StringHasher::HashMap: public HashMapBase 
{
public:
    bool SaveAll = false;
    int Threshold = 0;
};

///////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE_ABSTRACT(App::StringID, Base::BaseClass)

StringID::~StringID()
{
    if (_hasher)
        _hasher->_hashes->right.erase(_id);
}

PyObject *StringID::getPyObject() {
    return new StringIDPy(this);
}

PyObject *StringID::getPyObjectWithIndex(int index) {
    auto res = new StringIDPy(this);
    res->_index = index;
    return res;
}

std::string StringID::toString(int index) const {
    std::ostringstream ss;
    ss << '#' << std::hex << value();
    if (index)
        ss << ':' << index;
    return ss.str();
}

StringID::IndexID StringID::fromString(const char *name, bool eof, int size) {
    IndexID res;
    res.id = 0;
    res.index = 0;
    if (!name) {
        res.id = -1;
        return res;
    }
    if (size < 0)
        size = std::strlen(name);
    bio::stream<bio::array_source> iss(name, size);
    char sep = 0;
    char sep2 = 0;
    iss >> sep >> std::hex >> res.id >> sep2 >> res.index;
    if((eof && !iss.eof()) || sep!='#' || (sep2!=0 && sep2!=':')) {
        res.id = -1;
        return res;
    }
    return res;
}

std::string StringID::dataToText(int index) const {
    if(isHashed() || isBinary())
        return _data.toBase64().constData();

    std::string res(_data.constData());
    if (index) 
        res += std::to_string(index);
    if (_postfix.size())
        res += _postfix.constData();
    return res;
}

void StringID::mark() const
{
    if (isMarked())
        return;
    _flags.set(Marked);
    for (auto & sid : _sids)
        sid.deref().mark();
}

///////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(App::StringHasher, Base::Persistence)

StringHasher::StringHasher()
    :_hashes(new HashMap)
{}

StringHasher::~StringHasher() {
    clear();
}

void StringHasher::setSaveAll(bool enable) {
    if (_hashes->SaveAll == enable)
        return;
    _hashes->SaveAll = enable;
    compact();
}

void StringHasher::compact()
{
    if (_hashes->SaveAll)
        return;

    std::deque<StringIDRef> pendings;
    for (auto & v : _hashes->right) {
        if (!v.second->isPersistent() && v.second->getRefCount() == 1)
            pendings.emplace_back(v.second);
    }
    while(pendings.size()) {
        StringIDRef sid = pendings.front();
        pendings.pop_front();
        if (!_hashes->right.erase(sid.value()))
            continue;
        sid._sid->_hasher = nullptr;
        sid._sid->unref();
        for (auto & s : sid._sid->_sids) {
            if (s._sid->_hasher == this
                    && !s._sid->isPersistent()
                    && s._sid->getRefCount() == 2)
                pendings.push_back(s);
        }
    }
}

bool StringHasher::getSaveAll() const {
    return _hashes->SaveAll;
}

void StringHasher::setThreshold(int threshold) {
    _hashes->Threshold = threshold;
}

int StringHasher::getThreshold() const {
    return _hashes->Threshold;
}

long StringHasher::lastID() const {
    if(_hashes->right.empty())
        return 0;
    auto it = _hashes->right.end();
    --it;
    return it->first;
}

StringIDRef StringHasher::getID(const char *text, int len, bool hashable) {
    if(len<0) len = strlen(text);
    return getID(QByteArray::fromRawData(text,len),false,hashable);
}

StringIDRef StringHasher::getID(const QByteArray &data, bool binary, bool hashable, bool nocopy)
{
    bool hashed = hashable && _hashes->Threshold>0 
                           && (int)data.size()>_hashes->Threshold;

    StringID d;
    if(hashed) {
        QCryptographicHash hasher(QCryptographicHash::Sha1);
        hasher.addData(data);
        d._data = hasher.result();
    } else
        d._data = data;

    auto it = _hashes->left.find(&d);
    if(it!=_hashes->left.end())
        return StringIDRef(it->first);

    if(!hashed && !nocopy) {
        // if not hashed, make a deep copy of the data
        d._data = QByteArray(data.constData(), data.size());
    }

    StringIDRef sid(new StringID(lastID()+1,d._data,binary,hashed));
    return StringIDRef(insert(sid));
}

StringIDRef StringHasher::getID(long id, int index) const {
    if(id<=0)
        return StringIDRef();
    auto it = _hashes->right.find(id);
    if(it == _hashes->right.end())
        return StringIDRef();
    StringIDRef res(it->second);
    res._index = index;
    return res;
}

void StringHasher::setPersistenceFileName(const char *filename) const {
    if(!filename)
        filename = "";
    _filename = filename;
}

const std::string &StringHasher::getPersistenceFileName() const {
    return _filename;
}

void StringHasher::Save(Base::Writer &writer) const {

    size_t count;
    if (_hashes->SaveAll)
        count = _hashes->size();
    else {
        count = 0;
        for (auto & v : _hashes->right) {
            if (v.second->isMarked() || v.second->isPersistent())
                ++count;
        }
    }

    writer.Stream() << writer.ind()
        << "<StringHasher saveall=\"" << _hashes->SaveAll
        << "\" threshold=\"" << _hashes->Threshold << "\"";

    if(!count) {
        writer.Stream() << " count=\"0\"></StringHasher>\n";
        return;
    }

    writer.Stream() << " count=\"0\" new=\"1\"/>\n";

    writer.Stream() << writer.ind() << "<StringHasher2 ";
    if(_filename.size()) {
        writer.Stream() << " file=\"" 
            << writer.addFile(_filename+".txt",this)
            << "\"/>\n";
        return;
    }

    writer.Stream() << " count=\"" << count << "\">\n";
    saveStream(writer.beginCharStream(false) << '\n');
    writer.endCharStream() << '\n';
    writer.Stream() << writer.ind() << "</StringHasher2>\n";
}

void StringHasher::SaveDocFile (Base::Writer &writer) const {
    std::size_t count = _hashes->SaveAll?this->size():this->count();
    writer.Stream() << count << '\n';
    saveStream(writer.Stream());
}

void StringHasher::saveStream(std::ostream &s) const {
    Base::OutputStream str(s,false);
    boost::io::ios_flags_saver ifs(s);
    s << std::hex;

    bool allowRealtive = DocumentParams::getRelativeStringID();
    long anchor = 0;
    const StringID *last = nullptr;
    long lastid = 0;
    bool relative = false;

    for(auto &v : _hashes->right) {
        auto & d = *v.second;
        long id = d._id;
        if (!_hashes->SaveAll && !d.isMarked() && !d.isPersistent())
            continue;

        if (!allowRealtive)
            s << id;
        else {
            // We use relative coding to save space. But in order to have some
            // minimum protection against corruption, write an absolute value every
            // once a while.
            relative = (id - anchor) < 1000;
            if (relative)
                s << '-' << id - lastid;
            else  {
                anchor = id;
                s << id;
            }
            lastid = id;
        }

        int offset = d.isPostfixEncoded() ? 1 : 0;

        StringID::IndexID prefixid;
        prefixid.id = 0;
        prefixid.index = 0;
        if (d.isPrefixID()) {
            assert(d._sids.size() > offset);
            prefixid.id = d._sids[offset].value();
        }
        else if (d.isPrefixIDIndex()) {
            prefixid = StringID::fromString(d._data);
            assert(d._sids.size() > offset && d._sids[offset].value() == prefixid.id);
        }

        auto flags = d._flags;
        flags.reset(StringID::Marked);
        s << '.' << flags.to_ulong();

        int i = 0;
        if (!relative) {
            for (; i<d._sids.size(); ++i)
                s << '.' << d._sids[i].value();
        } else {
            if (last) {
                for (; i<d._sids.size() && i<last->_sids.size(); ++i) {
                    long m = last->_sids[i].value();
                    long n = d._sids[i].value();
                    if (n < m)
                        s << ".-" << m-n;
                    else
                        s << '.' << n-m;
                }
            }
            for (; i<d._sids.size(); ++i)
                s << '.' << id - d._sids[i].value();
        }

        last = & d;

        // Having postfix means it is a geometry element name, which
        // guarantees to be a single line without space. So it is safe to
        // store in raw stream.
        if (d.isPostfixed()) {
            if (d.isPrefixIDIndex())
                s << ' ' << prefixid.index;
            else if (!d.isIndexed() && !d.isPrefixID())
                s << ' ' << d._data.constData();

            if (!d.isPostfixEncoded())
                s << ' ' << d._postfix.constData();
            s << '\n';
        }
        else {
            // Reaching here means the string may contain space and newlines
            // We rely on OutputStream (i.e. str) to save the string.
            s << ' ';
            str << d._data.constData();
        }
    }
}

void StringHasher::RestoreDocFile (Base::Reader &reader) {
    std::string marker, ver;
    reader >> marker;
    std::size_t count;
    _hashes->clear();
    if (marker == "StringTableStart") {
        reader >> ver >> count;
        if (ver != "v1")
            FC_WARN("Unknown string table format");
        restoreStreamNew(reader, count);
        return;
    }
    reader >> count;
    restoreStream(reader,count);
}

void StringHasher::restoreStreamNew(std::istream &s, std::size_t count) {
    Base::InputStream str(s,false);
    _hashes->clear();
    std::string content;
    boost::io::ios_flags_saver ifs(s);
    s >> std::hex;
    std::vector<std::string> tokens;
    long lastid = 0;
    const StringID * last = nullptr;

    std::string tmp;

    for(uint32_t i=0;i<count;++i) {
        if (!(s >> tmp))
            FC_THROWM(Base::RuntimeError, "Invalid string table");

        tokens.clear();
        boost::split(tokens, tmp, boost::is_any_of("."));
        if (tokens.size() < 2)
            FC_THROWM(Base::RuntimeError, "Invalid string table");

        long id;
        bool relative = false;
        if (tokens[0][0] == '-') {
            relative = true;
            id = lastid + strtol(tokens[0].c_str()+1, nullptr, 16);
        } else
            id = strtol(tokens[0].c_str(), nullptr, 16);

        lastid = id;

        unsigned long flag = strtol(tokens[1].c_str(), nullptr, 16);
        StringIDRef sid(new StringID(id,QByteArray(),flag));

        StringID & d = *sid._sid;
        d._sids.reserve(tokens.size()-2);

        int j = 2;
        if (relative && last) {
            for (;j<(int)tokens.size() && j-2<last->_sids.size(); ++j) {
                long m = last->_sids[j-2].value();
                long n;
                if (tokens[j][0] == '-')
                    n = -strtol(&tokens[j][1], nullptr, 16);
                else
                    n = strtol(&tokens[j][0], nullptr, 16);
                StringIDRef sid = getID(m + n);
                if (!sid)
                    FC_THROWM(Base::RuntimeError, "Invalid string id reference");
                d._sids.push_back(sid);
            }
        }
        for (;j<(int)tokens.size(); ++j) {
            long n = strtol(&tokens[j][0], nullptr, 16) ;
            StringIDRef sid = getID(relative ? id - n : n);
            if (!sid)
                FC_THROWM(Base::RuntimeError, "Invalid string id reference");
            d._sids.push_back(sid);
        }

        if (!d.isPostfixed()) {
            str >> content;
            if(d.isHashed() || d.isBinary())
                d._data = QByteArray::fromBase64(content.c_str());
            else
                d._data = content.c_str();
        } else {
            int offset = 0;
            if (d.isPostfixEncoded()) {
                offset = 1;
                if (d._sids.empty())
                    FC_THROWM(Base::RuntimeError, "Missing string postfix");
                d._postfix = d._sids[0]._sid->_data;
            }
            if (d.isIndexed()) {
                if (d._sids.size() <= offset)
                    FC_THROWM(Base::RuntimeError, "Missing string prefix");
                d._data = d._sids[offset]._sid->_data;
            }
            else if (d.isPrefixID() || d.isPrefixIDIndex()) {
                if (d._sids.size() <= offset)
                    FC_THROWM(Base::RuntimeError, "Missing string prefix id");
                int index = 0;
                if (d.isPrefixIDIndex()) {
                    if (!(s >> index))
                        FC_THROWM(Base::RuntimeError, "Missing string prefix index");
                }
                d._data = d._sids[offset]._sid->toString(index).c_str();
            } else {
                s >> content;
                d._data = content.c_str();
            }
            if (!d.isPostfixEncoded()) {
                s >> content;
                d._postfix = content.c_str();
            }
        }

        last = insert(sid);
    }
}

StringID * StringHasher::insert(const StringIDRef & sid)
{
    assert(sid && sid._sid->_hasher == nullptr);
    auto & d = *sid._sid;
    d._hasher = this;
    d.ref();
    auto res = _hashes->right.insert(_hashes->right.end(),
            HashMap::right_map::value_type(sid.value(),&d));
    if (res->second != &d) {
        d._hasher = nullptr;
        d.unref();
    }
    return res->second;
}

void StringHasher::restoreStream(std::istream &s, std::size_t count) {
    Base::InputStream str(s,false);
    _hashes->clear();
    std::string content;
    for(uint32_t i=0;i<count;++i) {
        int32_t id;
        uint8_t type;
        str >> id >> type >> content;
        StringIDRef sid = new StringID(id,QByteArray(),type);
        if(sid.isHashed() || sid.isBinary()) {
            sid._sid->_data = QByteArray::fromBase64(content.c_str());
        } else
            sid._sid->_data = QByteArray(content.c_str());
        insert(sid);
    }
}

void StringHasher::clear() {
    for (auto & v : _hashes->right) {
        v.second->_hasher = nullptr;
        v.second->unref();
    }
    _hashes->clear();
}

size_t StringHasher::size() const {
    return _hashes->size();
}

size_t StringHasher::count() const {
    size_t count = 0;
    for(auto &v : _hashes->right) 
        if(v.second->getRefCount()>1)
            ++count;
    return count;
}

void StringHasher::Restore(Base::XMLReader &reader) {
    clear();
    reader.readElement("StringHasher");
    _hashes->SaveAll = reader.getAttributeAsInteger("saveall")?true:false;
    _hashes->Threshold = reader.getAttributeAsInteger("threshold");

    bool newtag = false;
    if (reader.getAttributeAsInteger("new","0") > 0) {
        reader.readElement("StringHasher2");
        newtag = true;
    }

    if(reader.hasAttribute("file")) {
        const char *file = reader.getAttribute("file");
        if(*file)
            reader.addFile(file,this);
        return;
    }

    std::size_t count = reader.getAttributeAsUnsigned("count");
    if (newtag) {
        restoreStreamNew(reader.beginCharStream(false),count);
        reader.readEndElement("StringHasher2");
        return;
    }
    else if(count && reader.FileVersion > 1)
        restoreStream(reader.beginCharStream(false),count);
    else {
        for(std::size_t i=0;i<count;++i) {
            reader.readElement("Item");
            StringIDRef sid;
            long id = reader.getAttributeAsInteger("id");
            bool hashed = reader.hasAttribute("hash");
            if(hashed || reader.hasAttribute("data")) {
                const char* value = hashed?reader.getAttribute("hash"):reader.getAttribute("data");
                sid = new StringID(id,QByteArray::fromBase64(value),true,hashed);
            }else {
                sid = new StringID(id,QByteArray(reader.getAttribute("text")),false,false);
            }
            insert(sid);
        }
    }
    reader.readEndElement("StringHasher");
}

unsigned int StringHasher::getMemSize (void) const {
    return (_hashes->SaveAll?size():count()) * 10;
}

PyObject *StringHasher::getPyObject() {
    return new StringHasherPy(this);
}

std::map<long,StringIDRef> StringHasher::getIDMap() const {
    std::map<long,StringIDRef> ret;
    for(auto &v : _hashes->right)
        ret.emplace_hint(ret.end(), v.first, StringIDRef(v.second));
    return ret;
}

void StringHasher::clearMarks() const
{
    for (auto & v : _hashes->right)
        v.second->_flags.reset(StringID::Marked);
}
