/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include <boost/algorithm/string/predicate.hpp>
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

FC_LOG_LEVEL_INIT("App",true,true)

using namespace App;

///////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE_ABSTRACT(App::StringID, Base::BaseClass)

PyObject *StringID::getPyObject() {
    return new StringIDPy(this);
}

static StringIDRef _StringIDNull(new StringID(0,"(null)",0));

StringIDRef StringID::getNullID() {
    return _StringIDNull;
}

bool StringID::isNull() const {
    return this == _StringIDNull;
}

std::string StringID::toString() const {
    std::ostringstream ss;
    ss << '#' << std::hex << value();
    return ss.str();
}

long StringID::fromString(const char *name, bool eof) {
    std::istringstream iss(name);
    char sep = 0;
    long id = -1;
    iss >> sep >> std::hex >> id;
    if((eof && !iss.eof()) || sep!='#')
        return -1;
    return id;
}

std::string StringID::dataToText() const {
    if(isHashed() || isBinary())
        return _data.toBase64().constData();
    return _data.constData();
}

///////////////////////////////////////////////////////////
//
namespace boost {
template<>
struct hash<QByteArray> {
    size_t operator()(const QByteArray &data) const {
        return qHash(data);
    }
};
}

typedef boost::bimap<
    boost::bimaps::unordered_set_of<QByteArray>,
    boost::bimaps::set_of<long>, 
    boost::bimaps::with_info<StringIDRef> > HashMapBase;

class StringHasher::HashMap: public HashMapBase 
{
public:
    bool SaveAll = false;
    int Threshold = 0;
};

///////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE(App::StringHasher, Base::Persistence)

StringHasher::StringHasher()
    :_hashes(new HashMap)
{}

StringHasher::~StringHasher() {
}

void StringHasher::setSaveAll(bool enable) {
    _hashes->SaveAll = enable;
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

StringIDRef StringHasher::getID(QByteArray data, bool binary, bool hashable) {
    QByteArray hash;

    bool hashed = hashable && _hashes->Threshold>0 
                           && (int)data.size()>_hashes->Threshold;

    if(hashed) {
        QCryptographicHash hasher(QCryptographicHash::Sha1);
        hasher.addData(data);
        hash = hasher.result();
    }else
        hash = data;

    auto it = _hashes->left.find(hash);
    if(it!=_hashes->left.end())
        return it->info;

    StringIDRef sid;
    if(hashed) {
        // if hashed, discard the original data
        data = hash;
    }else{
        // if not hashed, make a deep copy of the data
        data = QByteArray(data.constData(),data.size());
        hash = data;
    }
    sid = new StringID(lastID()+1,data,binary,hashed);
    _hashes->right.insert(_hashes->right.end(),HashMap::right_map::value_type(sid->value(),hash,sid));
    return sid;
}

StringIDRef StringHasher::getID(long id) const {
    if(id<=0)
        return _StringIDNull;
    auto it = _hashes->right.find(id);
    if(it == _hashes->right.end())
        return StringIDRef();
    return it->info;
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
    size_t count = _hashes->SaveAll?this->size():this->count();
    writer.Stream() << writer.ind() << "<StringHasher saveall=\"" 
        << _hashes->SaveAll << "\" threshold=\"" << _hashes->Threshold;

    if(!count) {
        writer.Stream() << "\" count=\"0\"></StringHasher>\n";
        return;
    }

    if(_filename.size()) {
        writer.Stream() << "\" file=\"" 
            << writer.addFile(_filename+".txt",this)
            << "\"/>\n";
        return;
    }

    writer.Stream() << "\" count=\"" << count << "\">\n";
    if(writer.getFileVersion() > 1) {
        saveStream(writer.beginCharStream(false) << '\n');
        writer.endCharStream() << '\n';
    } else {
        for(auto &v : _hashes->right) {
            if(_hashes->SaveAll || v.info.getRefCount()>1) {
                // We are omitting the indentation to save some space in case of long list of hashes
                if(v.info->isHashed()) 
                    writer.Stream() <<"<Item hash=\""<< v.second.toBase64().constData();
                else if(v.info->isBinary())
                    writer.Stream() <<"<Item data=\""<< v.second.toBase64().constData();
                else
                    writer.Stream() <<"<Item text=\""<< encodeAttribute(v.second.constData());
                writer.Stream() << "\" id=\""<<v.first<<"\"/>\n";
            }
        }
    }
    writer.Stream() << writer.ind() << "</StringHasher>\n";
}

void StringHasher::SaveDocFile (Base::Writer &writer) const {
    std::size_t count = _hashes->SaveAll?this->size():this->count();
    writer.Stream() << count << '\n';
    saveStream(writer.Stream());
}

void StringHasher::saveStream(std::ostream &s) const {
    Base::OutputStream str(s,false);
    for(auto &v : _hashes->right) {
        if(_hashes->SaveAll || v.info.getRefCount()>1) {
            // We do not use OutputStream to save the id and flags because
            // we don't want to use '\n' as delimiter. It makes no difference
            // to restoring.
            s << v.first << ' ' << v.info->_flags.to_ulong() << ' ';

            // We DO rely on OutputStream to save the string which may
            // contain multiple lines.
            str << v.info->dataToText();
        }
    }
}

void StringHasher::RestoreDocFile (Base::Reader &reader) {
    std::size_t count;
    reader >> count;
    restoreStream(reader,count);
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
        if(sid->isHashed() || sid->isBinary()) {
            sid->_data = QByteArray::fromBase64(content.c_str());
        } else
            sid->_data = QByteArray(content.c_str());
        _hashes->right.insert(_hashes->right.end(),
                HashMap::right_map::value_type(sid->value(),sid->_data,sid));
    }
}

void StringHasher::clear() {
    _hashes->clear();
}

size_t StringHasher::size() const {
    return _hashes->size();
}

size_t StringHasher::count() const {
    size_t count = 0;
    for(auto &v : _hashes->right) 
        if(v.info.getRefCount()>1)
            ++count;
    return count;
}

void StringHasher::Restore(Base::XMLReader &reader) {
    clear();
    reader.readElement("StringHasher");
    _hashes->SaveAll = reader.getAttributeAsInteger("saveall")?true:false;
    _hashes->Threshold = reader.getAttributeAsInteger("threshold");

    if(reader.hasAttribute("file")) {
        const char *file = reader.getAttribute("file");
        if(*file)
            reader.addFile(file,this);
        return;
    }

    std::size_t count = reader.getAttributeAsUnsigned("count");
    if(reader.FileVersion > 1) {
        restoreStream(reader.beginCharStream(false),count);
    } else {
        for(std::size_t i=0;i<count;++i) {
            reader.readElement("Item");
            StringIDRef sid;
            long id = reader.getAttributeAsInteger("id");
            QByteArray data;
            bool hashed = reader.hasAttribute("hash");
            if(hashed || reader.hasAttribute("data")) {
                const char* value = hashed?reader.getAttribute("hash"):reader.getAttribute("data");
                data = QByteArray::fromBase64(value);
                sid = new StringID(id,data,true,hashed);
            }else {
                data = QByteArray(reader.getAttribute("text"));
                sid = new StringID(id,data,false,false);
            }
            _hashes->right.insert(_hashes->right.end(),HashMap::right_map::value_type(sid->value(),data,sid));
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
        ret.emplace_hint(ret.end(),v.first,v.info);
    return ret;
}
