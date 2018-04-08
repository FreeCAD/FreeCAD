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

using namespace App;

///////////////////////////////////////////////////////////

TYPESYSTEM_SOURCE_ABSTRACT(App::StringID, Base::BaseClass)

PyObject *StringID::getPyObject() {
    return new StringIDPy(this);
}

std::string StringID::toString() const {
    std::ostringstream ss;
    ss << '#' << value();
    return ss.str();
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
    int Threshold = 40;
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

StringIDRef StringHasher::getID(const char *text, int len) {
    if(len<0) len = strlen(text);
    return getID(QByteArray::fromRawData(text,len),false);
}

StringIDRef StringHasher::getID(QByteArray data, bool binary) {
    QByteArray hash;
    bool hashed = _hashes->Threshold>=0 && (int)data.size()>_hashes->Threshold;
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
    auto it = _hashes->right.find(id);
    if(it == _hashes->right.end())
        return StringIDRef();
    return it->info;
}

void StringHasher::Save(Base::Writer &writer) const {
    size_t count = _hashes->SaveAll?this->size():this->count();
    writer.incInd();
    writer.Stream() << writer.ind() << "<StringHasher count=\""<< count 
        << "\" saveall=\"" << _hashes->SaveAll
        << "\" threshold=\"" << _hashes->Threshold << "\">" << std::endl;
    for(auto &v : _hashes->right) {
        if(_hashes->SaveAll || v.info.getRefCount()>1) {
            // We are omiting the indentation to save some space in case of long list of hashes
            if(v.info->isHashed()) 
                writer.Stream() <<"<Item hash=\""<< v.second.toBase64().constData();
            else if(v.info->isBinary())
                writer.Stream() <<"<Item data=\""<< v.second.toBase64().constData();
            else
                writer.Stream() <<"<Item text=\""<< encodeAttribute(v.second.constData());
            writer.Stream() << "\" id=\""<<v.first<<"\"/>" << std::endl;
        }
    }
    writer.Stream() << writer.ind() << "</StringHasher>" << std::endl;
    writer.decInd();
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
    int count = reader.getAttributeAsInteger("count");
    _hashes->SaveAll = reader.getAttributeAsInteger("saveall")?true:false;
    _hashes->Threshold = reader.getAttributeAsInteger("threshold");
    for(int i=0;i<count;++i) {
        reader.readElement("Item");
        StringIDRef sid;
        long id = reader.getAttributeAsInteger("id");
        QByteArray data;
        bool hashed = reader.hasAttribute("hash");
        if(hashed || reader.hasAttribute("data")) {
            const char* value = hashed?reader.getAttribute("hash"):reader.getAttribute("data");
            data = QByteArray::fromBase64(QByteArray::fromRawData(value,strlen(value)));
            sid = new StringID(id,data,true,hashed);
        }else {
            data = QByteArray(reader.getAttribute("text"));
            sid = new StringID(id,data,false,false);
        }
        _hashes->right.insert(_hashes->right.end(),HashMap::right_map::value_type(sid->value(),data,sid));
    }
    reader.readEndElement("StringHasher");
}

unsigned int StringHasher::getMemSize (void) const {
    return (_hashes->SaveAll?size():count());
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
