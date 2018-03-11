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
#include "ComplexGeoData.h"

using namespace Data;

namespace Data {
typedef boost::bimap<
            boost::bimaps::multiset_of<std::string>,
            boost::bimaps::unordered_set_of<std::string>,
            boost::bimaps::with_info<App::StringIDRef> > ElementMapBase;
class ElementMap: public ElementMapBase {};
}

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment , Base::BaseClass);


TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData , Base::Persistence);


ComplexGeoData::ComplexGeoData(void)
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
    ss << 1;
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
        return std::string(name,dot);
    return name;
}

size_t ComplexGeoData::getElementMapSize() const {
    return _ElementMap?_ElementMap->size():0;
}

const char *ComplexGeoData::getElementName(const char *name, bool reverse) const {
    if(!name || !_ElementMap) 
        return name;

    if(reverse) {
        auto it = _ElementMap->right.find(name);
        if(it == _ElementMap->right.end())
            return name;
        return it->second.c_str();
    }
    const char *txt = isMappedElement(name);
    if(!txt) 
        return name;
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
    return it->second.c_str();
}

std::vector<const char *> ComplexGeoData::getElementMappedNames(const char *element) const {
    std::vector<const char *> names;
    auto ret = _ElementMap->left.equal_range(element);
    for(auto it=ret.first;it!=ret.second;++it)
        names.push_back(it->second.c_str());
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
    if(!_ElementMap)
        _ElementMap = std::make_shared<ElementMap>();
    else
        _ElementMap->clear();
    for(auto &v : map)
        setElementName(v.first.c_str(),v.second.c_str());
}

const char *ComplexGeoData::setElementName(const char *element, const char *name, 
        bool overwrite, App::StringIDRef sid)
{
    if(!element || !element[0])
        throw Base::ValueError("Invalid input");
    if(!name || !name[0])  {
        _ElementMap->right.erase(element);
        return element;
    }
    const char *mapped = isMappedElement(name);
    if(mapped)
        name = mapped;
    if(!_ElementMap) _ElementMap = std::make_shared<ElementMap>();
    if(overwrite)
        _ElementMap->left.erase(name);
    std::string _name;
    if(!sid && !Hasher.isNull()) {
        sid = Hasher->getID(name);
        _name = sid->toString();
        name = _name.c_str();
        if(overwrite)
            _ElementMap->left.erase(name);
    }
    auto ret = _ElementMap->left.insert(ElementMap::left_map::value_type(name,element,sid));
    if(!ret.second && ret.first->second!=element) {
        std::ostringstream ss;
        ss << "duplicate element mapping '" << name << "->" << element << '/' << ret.first->second;
        throw Base::ValueError(ss.str().c_str());
    }
    return ret.first->first.c_str();
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
            if(v.info)
                writer.Stream() << "\" sid=\"" << v.info->value();
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
        for(size_t i=0;i<count;++i) {
            reader.readElement("Element");
            auto sid = App::StringIDRef();
            if(!Hasher.isNull() && reader.hasAttribute("sid"))
                sid = Hasher->getID(reader.getAttributeAsInteger("sid"));
            setElementName(reader.getAttribute("value"),reader.getAttribute("key"),false,sid);
        }
        reader.readEndElement("ElementMap");
    }
}

unsigned int ComplexGeoData::getMemSize(void) const {
    if(_ElementMap)
        return _ElementMap->size()*10;
    return 0;
}
