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
#include <boost/regex.hpp>

#include "ComplexGeoData.h"
#include <Base/BoundBox.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>


using namespace Data;

TYPESYSTEM_SOURCE_ABSTRACT(Data::Segment , Base::BaseClass)


TYPESYSTEM_SOURCE_ABSTRACT(Data::ComplexGeoData , Base::Persistence)


ComplexGeoData::ComplexGeoData()
    :Tag(0)
{
}

ComplexGeoData::~ComplexGeoData()
{
}

Data::Segment* ComplexGeoData::getSubElementByName(const char* name) const
{
    int index = 0;
    std::string element;
    boost::regex ex("^([^0-9]*)([0-9]*)$");
    boost::cmatch what;

    if (boost::regex_match(name, what, ex)) {
        element = what[1].str();
        index = std::atoi(what[2].str().c_str());
    }

    return getSubElement(element.c_str(), static_cast<unsigned long>(index));
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

void ComplexGeoData::getLinesFromSubElement(const Segment*,
                                            std::vector<Base::Vector3d> &Points,
                                            std::vector<Line> &lines) const
{
    (void)Points;
    (void)lines;
}

void ComplexGeoData::getFacesFromSubElement(const Segment*,
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
    return nullptr;
}

std::string ComplexGeoData::newElementName(const char *name) {
    if(!name)
        return std::string();
    const char *dot = strrchr(name,'.');
    if(!dot || dot==name)
        return name;
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
    if(!name)
        return std::string();
    const char *dot = strrchr(name,'.');
    if(!dot || dot==name)
        return name;
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
    if(!name)
        return std::string();
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
