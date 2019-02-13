/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include "ComplexGeoData.h"

FC_LOG_LEVEL_INIT("ComplexGeoData", true,true);

using namespace Data;

namespace Data {
typedef boost::bimap<
            boost::bimaps::set_of<std::string>,
            boost::bimaps::multiset_of<std::string>,
            boost::bimaps::with_info<std::vector<App::StringIDRef> > > ElementMapBase;
class ElementMap: public ElementMapBase {};
}

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment , Base::BaseClass);


TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData , Base::Persistence);


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
    std::ostringstream ss;
    ss << 3;
    if(Hasher) 
        ss << '.' << (Hasher->getThreshold()>0?Hasher->getThreshold():0);
    return ss.str();
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

size_t ComplexGeoData::getElementMapSize() const {
    return _ElementMap?_ElementMap->size():0;
}

const char *ComplexGeoData::getElementName(const char *name, int direction, 
        std::vector<App::StringIDRef> *sid) const 
{
    if(!name)
        return 0;
    if(!_ElementMap) {
        const char *txt = isMappedElement(name);
        if(txt && !boost::contains(txt,elementMapPrefix()))
            return txt;
        return name;
    }

    if(direction==1) {
        auto it = _ElementMap->right.find(name);
        if(it == _ElementMap->right.end())
            return name;
        if(sid) sid->insert(sid->end(),it->info.begin(),it->info.end());
        return it->second.c_str();
    }
    const char *txt = isMappedElement(name);
    if(!txt) {
        if(direction==0)
            return name;
        txt = name;
    }
    std::string _txt;
    // Strip out the trailing '.XXXX' if any
    const char *dot = strchr(txt,'.');
    if(dot) {
        _txt = std::string(txt,dot-txt);
        txt = _txt.c_str();
    }
    auto it = _ElementMap->left.find(txt);
    if(it == _ElementMap->left.end())
        return name;
    if(sid) sid->insert(sid->end(),it->info.begin(),it->info.end());
    return it->second.c_str();
}

std::vector<std::pair<std::string, std::vector<App::StringIDRef> > >
ComplexGeoData::getElementMappedNames(const char *element, bool needUnmapped) const {
    std::vector<std::pair<std::string, std::vector<App::StringIDRef> > > names;
    if(_ElementMap) {
        auto ret = _ElementMap->right.equal_range(element);
        size_t count=0;
        for(auto it=ret.first;it!=ret.second;++it)
            ++count;
        if(count) {
            names.reserve(count);
            for(auto it=ret.first;it!=ret.second;++it)
                names.emplace_back(it->second,it->info);
            return names;
        }
    }
    if(needUnmapped)
        names.emplace_back(element,std::vector<App::StringIDRef>());
    return names;
}

std::vector<std::pair<std::string,std::string> > 
ComplexGeoData::getElementNamesWithPrefix(const char *prefix) const {
    std::vector<std::pair<std::string,std::string> > names;
    if(!prefix || !prefix[0] || !_ElementMap)
        return names;
    const auto &p = elementMapPrefix();
    if(boost::starts_with(prefix,p))
        prefix += p.size();
    for(auto it=_ElementMap->left.lower_bound(prefix);it!=_ElementMap->left.end();++it) {
        if(boost::starts_with(it->first,prefix))
            names.emplace_back(it->first,it->second);
    }
    return names;
}

std::map<std::string, std::string> ComplexGeoData::getElementMap() const {
    std::map<std::string, std::string> ret;
    if(!_ElementMap) return ret;
    for(auto &v : _ElementMap->left)
        ret.emplace_hint(ret.cend(),v.first,v.second);
    return ret;
}

void ComplexGeoData::setElementMap(const std::map<std::string, std::string> &map) {
    resetElementMap();
    for(auto &v : map)
        setElementName(v.second.c_str(),v.first.c_str());
}

void ComplexGeoData::copyElementMap(const ComplexGeoData &data, const char *postfix) {
    _ElementMap.reset();
    if(!data._ElementMap)
        return;

    if(postfix && !postfix[0])
        postfix = 0;

    if(!Hasher)
        Hasher = data.Hasher;

    for(const auto &v : data._ElementMap->left) {
        auto name = v.first.c_str();
        if(Hasher==data.Hasher || !data.Hasher) {
            setElementName(v.second.c_str(), name, postfix, &v.info);
            continue;
        }
        if(postfix)
            setElementName(v.second.c_str(),name,postfix);
        else {
            // In case we have different hasher, but no additional postfix. 
            // Copy the element name as it is without hashing.
            setElementName(v.second.c_str(),name,0,false,true);
        }
    }
}

const char *ComplexGeoData::setElementName(const char *element, const char *name, 
        const char *postfix, const std::vector<App::StringIDRef> *sid, bool overwrite)
{
    if(!element || !element[0] || !name || !postfix)
        return setElementName(element,name,sid,overwrite);

    std::vector<App::StringIDRef> _sid;
    std::ostringstream ss;
    if((!sid || sid->empty()) && Hasher) {
        sid = &_sid;
        ss << hashElementName(name,_sid);
        name = "";
    }else
        ss << name;
    if(postfix && postfix[0]) {
        if(ss.tellp() && !boost::starts_with(postfix,elementMapPrefix()))
            ss << elementMapPrefix();
        ss << postfix;
    }
    return setElementName(element,ss.str().c_str(),sid,overwrite,!name[0]);
}

std::string ComplexGeoData::hashElementName(
        const char *name, std::vector<App::StringIDRef> &sid) const
{
    if(!name)
        throw Base::ValueError("invalid element name");
    if(!Hasher||!name[0])
        return name;
    std::string prefix;
    auto pos = strstr(name,elementMapPrefix().c_str());
    if(!pos)
        return name;
    sid.push_back(Hasher->getID(name));
    return sid.back()->toString();
}

std::string ComplexGeoData::dehashElementName(const char *name) const {
    if(!name)
        return std::string();
    if(boost::starts_with(name,elementMapPrefix()))
        name += elementMapPrefix().size();
    if(!Hasher)
        return name;
    long id = App::StringID::fromString(name,true);
    if(id<0)
        return name;
    auto sid = Hasher->getID(id);
    if(!sid) {
        if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE))
            FC_WARN("failed to find hash id " << id);
        else
            FC_LOG("failed to find hash id " << id);
        return name;
    }
    if(sid->isHashed()) {
        FC_LOG("cannot dehash id " << id);
        return name;
    }
    auto ret = sid->dataToText();
    FC_TRACE("dehash " << name << " -> " << ret);
    return ret;
}

const char *ComplexGeoData::setElementName(const char *element, const char *name, 
        const std::vector<App::StringIDRef> *sid, bool overwrite,bool nohash)
{
    if(!element || !element[0])
        throw Base::ValueError("Invalid input");
    if(!name || !name[0])  {
        if(_ElementMap)
            _ElementMap->right.erase(element);
        return element;
    }
    std::vector<App::StringIDRef> _sid;
    const char *mapped = isMappedElement(name);
    if(mapped)
        name = mapped;
    if(!_ElementMap) _ElementMap = std::make_shared<ElementMap>();
    std::string _name;
    if((!sid||sid->empty()) && Hasher && !nohash) {
        sid = &_sid;
        _name = hashElementName(name,_sid);
        name = _name.c_str();
    }else if(!sid || sid->empty()) {
        if(Hasher && nohash)
            _sid.push_back(App::StringID::getNullID());
        sid = &_sid;
    }
    int retry=1;
    mapped = name;
    std::ostringstream ss;
    std::string retry_name;
    while(1) {
        auto ret = _ElementMap->left.insert(ElementMap::left_map::value_type(mapped,element,*sid));
        if(ret.second || ret.first->second==element) {
            FC_TRACE(element << " -> " << name);
            return ret.first->first.c_str();
        }
        if(overwrite) {
            overwrite = false;
            _ElementMap->left.erase(ret.first);
            continue;
        }
        if(sid!=&_sid)
            _sid.insert(_sid.end(),sid->begin(),sid->end());
        retry_name = renameDuplicateElement(retry++,element,ret.first->second.c_str(),name,_sid);
        if(retry_name.empty())
            return ret.first->first.c_str();
        mapped = retry_name.c_str();
        sid = &_sid;
    }
}

char ComplexGeoData::elementType(const char *name) const {
    if(!name)
        return 0;
    if(isMappedElement(name)) {
        auto element = getElementName(name);
        if(element != name)
            return element[0];
    }
    const char* element = strchr(name,'.');
    std::string _name;
    if(element) {
        _name = std::string(name,element);
        ++element;
    }else{
        _name = name;
        if(!isMappedElement(name))
            element = getElementName(name,2);
    }

    char element_type=0;
    if(findTagInElementName(_name,0,0,0,&element_type)!=std::string::npos)
        return element_type;

    if(element) {
        for(auto &type : getElementTypes()) {
            if(boost::starts_with(element,type))
                return element[0];
        }
    }
    return 0;
}

std::string ComplexGeoData::renameDuplicateElement(int index, const char *element, 
            const char *element2, const char *name, std::vector<App::StringIDRef> &sids)
{
    std::ostringstream ss;
    ss << elementMapPrefix() << 'D' << index;
    std::string renamed(name);
    encodeElementName(element[0],renamed,ss,sids);
    renamed += ss.str();
    FC_LOG("duplicate element mapping '" << name << " -> " << renamed << ' ' 
            << element << '/' << element2);
    return renamed;
}

const std::string &ComplexGeoData::tagPostfix() {
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

size_t ComplexGeoData::findTagInElementName(const std::string &name, 
        long *tag, size_t *len, std::string *postfix, char *type) 
{
    size_t pos = name.rfind(tagPostfix());
    if(pos==std::string::npos)
        return pos;
    size_t offset = pos + tagPostfix().size();
    std::istringstream iss(name.c_str()+offset);
    long _tag = -1;
    int _len = -1;
    char sep = 0;
    char sep2 = 0;
    char tp = 0;
    char eof = 0;
    iss >> _tag >>  sep >> _len >> sep2 >> tp >> eof;
    if(_tag<0 || _len<0 || sep!=':' || sep2!=':' || eof!=0)
        return std::string::npos;
    if(type)
        *type = tp;
    if(tag)
        *tag = _tag;
    if(len)
        *len = (size_t)_len;
    if(postfix)
        *postfix=name.c_str()+pos;
    return pos;
}

// try to hash element name while preserving the source tag
void ComplexGeoData::encodeElementName(char element_type, std::string &name, std::ostringstream &ss, 
        std::vector<App::StringIDRef> &sids, const char* postfix, long tag) const
{
    if(postfix) {
        if(ss.tellp())
            ss << elementMapPrefix();
        ss << postfix;
    }
    long inputTag = 0;
    if(!ss.tellp()) {
        if(!tag || tag==Tag) {
            ss << name;
            name.clear();
            return;
        }
        findTagInElementName(name,&inputTag);
        if(inputTag == tag) {
            ss << name;
            name.clear();
            return;
        }
    }else if(!tag || tag==Tag) {
        findTagInElementName(name,&inputTag);
        if(inputTag)
            tag = inputTag;
    }
    if(Hasher)
        name = hashElementName(name.c_str(),sids);
    if(tag) {
        assert(element_type);
        ss << tagPostfix() << tag << ':' << name.size() << ':' << element_type;
    }
}

long ComplexGeoData::getElementHistory(const char *_name, 
        std::string *original, std::vector<std::string> *history) const 
{
    long tag = 0;
    size_t len = 0;
    std::string name;
    auto mapped = isMappedElement(_name);
    if(mapped)
        _name = mapped;
    auto dot = strchr(_name,'.');
    if(dot)
        name = std::string(_name,dot-_name);
    else
        name = _name;
    auto pos = findTagInElementName(name,&tag,&len);
    if(pos == std::string::npos) {
        if(original)
            *original = name;
        return tag;
    }
    if(!original&&!history)
        return tag;

    std::string tmp;
    std::string &ret = original?*original:tmp;
    bool first = true;
    while(1) {
        if(!len || len>pos) {
            FC_WARN("invalid name length " << name);
            return 0;
        }
        if(first) {
            first = false;
            size_t offset = 0;
            if(boost::starts_with(name,elementMapPrefix()))
                offset = elementMapPrefix().size();
            ret = name.substr(offset,len);
        }else
            ret = ret.substr(0,len);
        ret = dehashElementName(ret.c_str());
        long tag2 = 0;
        pos = findTagInElementName(ret,&tag2,&len);
        if(pos==std::string::npos || (tag2!=tag && tag!=Tag))
            return tag;
        tag = tag2;
        if(history)
            history->push_back(ret);
    }
}

void ComplexGeoData::Save(Base::Writer &writer) const {
    writer.Stream() << writer.ind() << "<ElementMap";
    if(!_ElementMap || _ElementMap->empty())
        writer.Stream() << "/>" << std::endl;
    else {
        writer.Stream() << " count=\"" << _ElementMap->left.size() << "\">" << std::endl;
        for(auto &v : _ElementMap->left) {
            // We are omitting indentation here to save some space in case of long list of elements
            writer.Stream() << "<Element key=\"" <<  
                v.first <<"\" value=\"" << v.second;
            if(v.info.size()) {
                writer.Stream() << "\" sid=\"" << v.info.front()->value();
                for(size_t i=1;i<v.info.size();++i)
                    writer.Stream() << '.' << v.info[i]->value();
            }
            writer.Stream() << "\"/>" << std::endl;
        }
        writer.Stream() << writer.ind() << "</ElementMap>" << std::endl ;
    }
}

void ComplexGeoData::Restore(Base::XMLReader &reader) {
    resetElementMap();
    reader.readElement("ElementMap");
    if(reader.hasAttribute("count")) {
        size_t count = reader.getAttributeAsUnsigned("count");
        size_t invalid_count = 0;
        for(size_t i=0;i<count;++i) {
            reader.readElement("Element");
            std::vector<App::StringIDRef> sids;
            if(reader.hasAttribute("sid")) {
                if(!Hasher) 
                    FC_ERR("missing hasher");
                else {
                    std::istringstream iss(reader.getAttribute("sid"));
                    long id;
                    while((iss >> id)) {
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
            setElementName(reader.getAttribute("value"),"",reader.getAttribute("key"),&sids);
        }
        if(invalid_count)
            FC_ERR("Found " << invalid_count << " invalid string id");
        reader.readEndElement("ElementMap");
    }
}

unsigned int ComplexGeoData::getMemSize(void) const {
    if(_ElementMap)
        return _ElementMap->size()*10;
    return 0;
}

bool ElementNameComp::operator()(const std::string &a, const std::string &b) const {
    size_t size = std::min(a.size(),b.size());
    if(!size)
        return a.size()<b.size();
    size_t i=0;

    if(b[0] == '#') {
        if(a[0]!='#')
            return true;
        // If both string starts with '#', compare the following hex digits by
        // its integer value.
        int res = 0;
        for(i=1;i<size;++i) {
            unsigned char ac = (unsigned char)a[i];
            unsigned char bc = (unsigned char)b[i];
            if(std::isxdigit(bc)) {
                if(!std::isxdigit(ac))
                    return true;
                if(res==0) {
                    if(ac<bc)
                        res = -1;
                    else if(ac>bc)
                        res = 1;
                }
            }else if(std::isxdigit(ac))
                return false;
            else
                break;
        }
        if(res < 0)
            return true;
        else if(res > 0)
            return false;
        return std::strcmp(&a[i],&b[i])<0;
    }

    // If the string does not start with '#', compare the non-digits prefix
    // using lexical order.
    for(i=0;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(!std::isdigit(bc)) {
            if(std::isdigit(ac))
                return true;
            if(ac<bc)
                return true;
            if(ac>bc)
                return false;
        } else if(!std::isdigit(ac)) {
            return false;
        } else
            break;
    }

    // Then compare the following digits part by integer value
    int res = 0;
    for(;i<size;++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if(std::isdigit(bc)) {
            if(!std::isdigit(ac))
                return true;
            if(res==0) {
                if(ac<bc)
                    res = -1;
                else if(ac>bc)
                    res = 1;
            }
        }else if(std::isdigit(ac))
            return false;
        else
            break;
    }
    if(res < 0)
        return true;
    else if(res > 0)
        return false;

    // Finally, compare the remaining tail using lexical order
    return std::strcmp(&a[i],&b[i])<0;
}
