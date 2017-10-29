/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <sstream>
#endif

#include <Base/Console.h>

#include "DrawUtil.h"
#include "Cube.h"

using namespace TechDraw;

//default starting dirs & rots
const std::map<std::string,Base::Vector3d> Cube::m_viewDefault = {
        { "Front",            Base::Vector3d(0, -1, 0) },    //front
        { "Rear",             Base::Vector3d(0, 1, 0) },     //rear
        { "Right",            Base::Vector3d(1, 0, 0) },     //right
        { "Left",             Base::Vector3d(-1, 0, 0) },    //left
        { "Top",              Base::Vector3d(0, 0, 1) },     //top
        { "Bottom",           Base::Vector3d(0, 0, -1) },    //bottom
        };
                                                              
const std::map<std::string,Base::Vector3d> Cube::m_rotDefault = {
        { "Front",            Base::Vector3d(1,  0, 0) },    //front    RightDir
        { "Rear",             Base::Vector3d(-1, 0, 0) },    //rear     LeftDir
        { "Right",            Base::Vector3d(0, -1, 0) },    //right    FrontDir
        { "Left",             Base::Vector3d(0,  1, 0) },    //left     RearDir
        { "Top",              Base::Vector3d(1,  0, 0) },    //top      RightDir
        { "Bottom",           Base::Vector3d(1,  0, 0) },    //bottom   RightDir
        }; 

const Base::Vector3d _Y(0,1,0);


Cube::Cube(void)
{
}

Cube::~Cube(void)
{
}

void Cube::initialize()
{
    m_mapCubeDir = m_viewDefault;
    m_mapCubeRot = m_rotDefault;
}

// left,right,front,back,top,bottom
void Cube::initialize(Base::Vector3d r, Base::Vector3d rr, Base::Vector3d l, Base::Vector3d lr,
                      Base::Vector3d f, Base::Vector3d fr, Base::Vector3d k, Base::Vector3d kr,      //k for bacK (rear)
                      Base::Vector3d t, Base::Vector3d tr, Base::Vector3d b, Base::Vector3d br)
{
    m_mapCubeDir.clear();
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", b));
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , f));
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , l));
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , k));
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , r));
    m_mapCubeDir.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , t));

    m_mapCubeRot.clear();
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", br));
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , fr));
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , lr));
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , kr));
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , rr));
    m_mapCubeRot.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , tr));
}

// rotate/spin the subject inside the glass cube
// effectively, rotate/spin the cube in reverse of the apparent subject movement

//TODO: there is a problem with calculaion of view rotation vector when the axis of rotation is 
//      +/-Y.  There is hack code here to handle it, but there should be a more elegant solution

// subject CW about Right 
void Cube::rotateUp(double angle)
{
    //Front -> Top -> Rear -> Bottom -> Front
    Base::Vector3d axis = getRight();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }

    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,-angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }

    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
        newRots["Right"] = newRots["Right"] * flipper;
        newRots["Left"]  = newRots["Left"] * flipper;
    }

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
}    

// CCW about Right
void Cube::rotateDown(double angle)
{
    //Front -> Bottom -> Rear -> Top -> Front
    Base::Vector3d axis = getRight();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,-angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }

    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }

    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
        newRots["Right"] = newRots["Right"] * flipper;
        newRots["Left"]  = newRots["Left"] * flipper;
    }

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
} 

// CCW about Top
void Cube::rotateRight(double angle)
{
    //Front -> Right -> Rear -> Left -> Front
    Base::Vector3d axis = getTop();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,-angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }

    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }

    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
        newRots["Top"]   = newRots["Top"] * flipper;
        newRots["Bottom"] = newRots["Bottom"] * flipper;
        newRots["Left"]   = newDirs["Front"];
        newRots["Right"]  = newRots["Left"] * -1.0;
    }
    newRots["Front"]  = newRots["Top"];
    newRots["Rear"]   = newRots["Front"] * -1.0;

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
} 
 
// CW about Top
void Cube::rotateLeft(double angle)
{
    //Front -> Left -> Rear -> Right -> Front
    Base::Vector3d axis = getTop();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }

    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,-angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }
    
    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
        newRots["Top"]   = newRots["Top"] * flipper;
        newRots["Bottom"] = newRots["Bottom"] * flipper;
        newRots["Left"]   = newDirs["Front"];
        newRots["Right"]  = newRots["Left"] * -1.0;
    }
    newRots["Front"]  = newRots["Top"];
    newRots["Rear"]   = newRots["Front"] * -1.0;

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
} 

// CCW about Front
void Cube::spinCCW(double angle)
{
    Base::Vector3d axis = getFront();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,-angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }
    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }

    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
    }
    newRots["Front"] = newRots["Front"] * flipper;
    newRots["Rear"]  = newRots["Rear"] * flipper;
    newRots["Top"]   = newRots["Front"];
    newRots["Bottom"] = newRots["Front"];

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
} 

// CW about Front
void Cube::spinCW(double angle)
{
    //Right -> Bottom -> Left -> Top -> Right
    Base::Vector3d axis = getFront();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = rodrigues(d.second,angle,axis);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
    }

    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = rodrigues(r.second,-angle,axis);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
    }
    double flipper = 1.0;
    if (DrawUtil::checkParallel(axis,_Y)) {
        flipper = -flipper;
        newRots["Front"] = newRots["Front"] * flipper;
        newRots["Rear"]  = newRots["Rear"] * flipper;
    }
    newRots["Top"]   = newRots["Front"];
    newRots["Bottom"] = newRots["Front"];

    m_mapCubeDir = newDirs;
    m_mapCubeRot = newRots;
} 

//dumps the current ortho state of cube 
void Cube::dump(char * title)
{
//Bottom/Front/Left/Rear/Right/Top
    Base::Console().Message("Cube: %s\n", title); 
    Base::Console().Message("B: %s/%s  \nF: %s/%s  \nL: %s/%s\n", 
                            DrawUtil::formatVector(getBottom()).c_str(),DrawUtil::formatVector(getBottomRot()).c_str(),
                            DrawUtil::formatVector(getFront()).c_str(),DrawUtil::formatVector(getFrontRot()).c_str(), 
                            DrawUtil::formatVector(getLeft()).c_str(),DrawUtil::formatVector(getLeftRot()).c_str());
    Base::Console().Message("K: %s/%s  \nR: %s/%s  \nT: %s/%s\n", 
                            DrawUtil::formatVector(getRear()).c_str(),DrawUtil::formatVector(getRearRot()).c_str(),
                            DrawUtil::formatVector(getRight()).c_str(),DrawUtil::formatVector(getRightRot()).c_str(), 
                            DrawUtil::formatVector(getTop()).c_str(),DrawUtil::formatVector(getTopRot()).c_str());
}

//dumps the current "board" -ISO's
void Cube::dumpISO(char * title)
{
//FBL/FBR/FTL/FTR
    Base::Console().Message("Cube ISO: %s\n", title); 
    Base::Console().Message("FBL: %s/%s  \nFBR: %s/%s  \nFTL: %s/%s\nFTR: %s/%s\n", 
                            DrawUtil::formatVector(getFBL()).c_str(),DrawUtil::formatVector(getFBLRot()).c_str(),
                            DrawUtil::formatVector(getFBR()).c_str(),DrawUtil::formatVector(getFBRRot()).c_str(), 
                            DrawUtil::formatVector(getFTL()).c_str(),DrawUtil::formatVector(getFTLRot()).c_str(), 
                            DrawUtil::formatVector(getFTR()).c_str(),DrawUtil::formatVector(getFTRRot()).c_str());
}

Base::Vector3d Cube::getViewDir(std::string name)
{
    Base::Vector3d result;
    auto x = m_mapCubeDir.find(name);
    if (x != m_mapCubeDir.end())  {
        result = m_mapCubeDir.at(name);
    } else {
        if (name == "FrontTopRight") {
            result = getFTR();
        } else if (name == "FrontBottomRight") {
            result = getFBR();
        } else if (name == "FrontTopLeft") {
            result = getFTL();
        } else if (name == "FrontBottomLeft") {
            result = getFBL();
        } else {
            result = Base::Vector3d(0,-1,0);
            Base::Console().Log("Cube: invalid direction name - %s\n",name.c_str());
        }
    }
    return result;
}

Base::Vector3d Cube::getRight()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Right");
    return result;
}

Base::Vector3d Cube::getFront()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Front");
    return result;
}

Base::Vector3d Cube::getTop()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Top");
    return result;
}

Base::Vector3d Cube::getLeft()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Left");
    return result;
}

Base::Vector3d Cube::getRear()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Rear");
    return result;
}

Base::Vector3d Cube::getBottom()
{
    Base::Vector3d result;
    result = m_mapCubeDir.at("Bottom");
    return result;
}

Base::Vector3d Cube::getFBL()
{
    Base::Vector3d result;
    result = getFront() + getBottom() + getLeft();
    return result;
}

Base::Vector3d Cube::getFBR()
{
    Base::Vector3d result;
    result = getFront() + getBottom() + getRight();
    return result;
}

Base::Vector3d Cube::getFTL()
{
    Base::Vector3d result;
    result = getFront() + getTop() + getLeft();
    return result;
}

Base::Vector3d Cube::getFTR()
{
    Base::Vector3d result;
    result = getFront() + getTop() + getRight();
    return result;
}

Base::Vector3d Cube::getRotationDir(std::string name)
{
    Base::Vector3d result;
    auto x = m_mapCubeRot.find(name);
    if (x != m_mapCubeRot.end())  {
        result = m_mapCubeRot.at(name);
    } else {
        if (name == "FrontTopRight") {
            result = getFTRRot();
        } else if (name == "FrontBottomRight") {
            result = getFBRRot();
        } else if (name == "FrontTopLeft") {
            result = getFTLRot();
        } else if (name == "FrontBottomLeft") {
            result = getFBLRot();
        } else {
            result = Base::Vector3d(1,0,0);
            Base::Console().Log("Cube: invalid rotation name - %s\n",name.c_str());
        }
    }
    return result;
}

Base::Vector3d Cube::getRightRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Right");
    return result;
}

Base::Vector3d Cube::getFrontRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Front");
    return result;
}

Base::Vector3d Cube::getTopRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Top");
    return result;
}

Base::Vector3d Cube::getLeftRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Left");
    return result;
}

Base::Vector3d Cube::getRearRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Rear");
    return result;
}

Base::Vector3d Cube::getBottomRot()
{
    Base::Vector3d result;
    result = m_mapCubeRot.at("Bottom");
    return result;
}

Base::Vector3d Cube::getFBLRot()
{
    Base::Vector3d result;
    result = getFrontRot() + getLeftRot();
    return result;
}

Base::Vector3d Cube::getFBRRot()
{
    Base::Vector3d result;
    result = getFrontRot() + getRightRot();
    return result;
}

Base::Vector3d Cube::getFTLRot()
{
    Base::Vector3d result;
    result = getFrontRot() + getLeftRot();
    return result;
}

Base::Vector3d Cube::getFTRRot()
{
    Base::Vector3d result;
    result = getFrontRot() + getRightRot();
    return result;
}

Base::Vector3d Cube::rodrigues(Base::Vector3d v,
                               double angle,
                               Base::Vector3d axis)
{
    Base::Vector3d result;
    if (DrawUtil::checkParallel(v,axis)) {
        result = v;
    } else {
        Base::Vector3d around = axis;
        around.Normalize();
        double theta = angle * (M_PI / 180.0);
        Base::Vector3d t1 = cos(theta) * v;
        Base::Vector3d t2 = sin(theta) * around.Cross(v);
        Base::Vector3d t3 = around.Dot(v) * (1 - cos(theta)) * around;
        result = t1 + t2 + t3;
    }
    return result;
}

std::vector<Base::Vector3d> Cube::getAllDirs(void)
{
    std::vector<Base::Vector3d> result;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = d.second;
        result.push_back(v);
    }
    return result;
}

std::vector<Base::Vector3d> Cube::getAllRots(void)
{
    std::vector<Base::Vector3d> result;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = r.second;
        result.push_back(v);
    }
    return result;
}

void Cube::setAllDirs(std::vector<Base::Vector3d> dirs)
{
    if (dirs.size() != 6) {
        Base::Console().Error("Cube:setAllDirs - missing dirs: %d\n",dirs.size());
        return;   //throw something?
    }

    auto i = dirs.begin();
    std::map<std::string, Base::Vector3d> newDirs;
    for (auto& d: m_mapCubeDir) {
        Base::Vector3d v = (*i);
        newDirs.insert(std::map<std::string, Base::Vector3d>::value_type(d.first, v));
        i++;
    }
    m_mapCubeDir = newDirs;
}

void Cube::setAllRots(std::vector<Base::Vector3d> rots)
{
    if (rots.size() != 6) {
        Base::Console().Error("Cube:setAllRots - missing rots: %d\n",rots.size());
        return;   //throw something?
    }

    auto i = rots.begin();
    std::map<std::string, Base::Vector3d> newRots;
    for (auto& r: m_mapCubeRot) {
        Base::Vector3d v = (*i);
        newRots.insert(std::map<std::string, Base::Vector3d>::value_type(r.first, v));
        i++;
    }
    m_mapCubeRot = newRots;
}




