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
//#include <Base/Vector3D.h>

#include "DrawUtil.h"
#include "Cube.h"

using namespace TechDraw;

const std::map<std::string,Base::Vector3d> Cube::m_viewToStdDir = {
        { "A", Base::Vector3d(0, -1, 0) },      //front
        { "B", Base::Vector3d(0, 0, 1) },       //top
        { "C", Base::Vector3d(-1, 0, 0) },      //left
        { "D", Base::Vector3d(1, 0, 0) },       //right
        { "E", Base::Vector3d(0, 0, -1) },      //bottom
        { "F", Base::Vector3d(0, 1, 0) },       //rear
        { "G", Base::Vector3d(-1,-1,1) },       //FTL
        { "H", Base::Vector3d(1, -1, 1) },      //FTR
        { "I", Base::Vector3d(1, -1, -1) },     //FBR
        { "J", Base::Vector3d(-1, -1, -1) } };  //FBL


Cube::Cube(void)
{
}

Cube::~Cube(void)
{
}

void Cube::initialize(Base::Vector3d r, Base::Vector3d rr, Base::Vector3d l, Base::Vector3d lr,
                      Base::Vector3d f, Base::Vector3d fr, Base::Vector3d k, Base::Vector3d kr,      //k for bacK (rear)
                      Base::Vector3d t, Base::Vector3d tr, Base::Vector3d b, Base::Vector3d br)
{
    Base::Console().Message("TRACE - Cube::init()\n");
    //Base::Vector3d FTR = f+t+r;
    //Base::Vector3d FTL = f+t-r;
    //Base::Vector3d FBL = f-t-r;
    //Base::Vector3d FBR = -f-t-r;
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", b));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , f));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , l));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , k));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , r));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , t));
//    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopRight"   , b));
//    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopLeft"    , b));
//    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomLeft" , b));
//    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomRight", b));

    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", br));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , fr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , lr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , kr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , rr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , tr));
//    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopRight"   , br));
//    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopLeft"    , br));
//    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomLeft" , br));
//    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomRight", br));
    
    m_conTab.initialize();   //all possible configs of ABCDEF in RightFrontTopLeftRearBottom
//    m_conTab.dump("conTab after init");
}

void Cube::rotateUp()
{
    //Front -> Top -> Rear -> Bottom -> Front???
    saveSwap("Front");
    shiftFrame("Bottom", "Front");
    shiftFrame("Rear"  , "Bottom");
    shiftFrame("Top"   , "Rear");
    restoreSwap("Top");
    
    updateRotsToConfig(getCurrConfig());
    dump("RotateUp(board after Rot update)");
//    dumpState("RotateUp(after update)");
    //validateBoard();
    
}    

void Cube::rotateDown()
{

    //Front -> Bottom -> Rear -> Top -> Front???
    saveSwap("Front");
    shiftFrame("Top"    , "Front");
    shiftFrame("Rear"   , "Top");
    shiftFrame("Bottom" , "Rear");
    restoreSwap("Bottom");

    updateRotsToConfig(getCurrConfig());
    dump("RotateDown(board after Rot update)");
//    dumpState("RotateDown(after update)");
    //validateBoard();
    
} 

void Cube::rotateRight()
{
    //dump("RotateRight (board before)");
    
    //Front -> Right -> Rear -> Left -> Front???
    saveSwap("Front");
    shiftFrame("Left"  , "Front");
    shiftFrame("Rear"  , "Left");
    shiftFrame("Right" , "Rear");
    restoreSwap("Right");

    updateRotsToConfig(getCurrConfig());
    dump("RotateRight(board after Rot update)");
//    bool boardState = validateBoard(getCurrConfig());
//    Base::Console().Message("TRACE - Cube::rotateRight - boardState: %d\n",boardState);
//    dumpState("RotateRight(state after update)");
} 
   
void Cube::rotateLeft()
{
    //Front -> Left -> Rear -> Right -> Front???
    saveSwap("Front");
    shiftFrame("Right", "Front");
    shiftFrame("Rear" , "Right");
    shiftFrame("Left" , "Rear");
    restoreSwap("Left");

    updateRotsToConfig(getCurrConfig());
    dump("RotateLeft(board after Rot updates)");
//    dumpState("RotateLeft(after update)");
    
} 

void Cube::spinCCW()
{
    //Right -> Top -> Left -> Bottom -> Right???
    saveSwap("Right");
    shiftFrame("Bottom", "Right");
    shiftFrame("Left"  , "Bottom");
    shiftFrame("Top"   , "Left");
    restoreSwap("Top");

    updateRotsToConfig(getCurrConfig());
    dump("SpinCCW(board after Rot updates)");
//    dumpState("SpinCCW(after update)");
} 

void Cube::spinCW()
{
    //Left -> Top -> Right -> Bottom -> Left??? 
    saveSwap("Left");
    shiftFrame("Bottom", "Left");
    shiftFrame("Right" , "Bottom");
    shiftFrame("Top", "Right");
    restoreSwap("Top");

    updateRotsToConfig(getCurrConfig());
    dump("spinCW(board after Rot updates)");
//    dumpState("SpinCW(after update)");
} 

std::string Cube::dirToView(Base::Vector3d v)
{
    std::string result;
    for (auto& i: m_viewToStdDir) {
        if (i.second == v) {
            result = i.first;
            break;
        }
    }
    return result;
}

void Cube::updateDirsToConfig(std::string cfg)
{
//    Base::Console().Message("TRACE - Cube::updateDirs(%s) \n",cfg.c_str());
    Base::Vector3d boardValue = m_conTab.getDirItem(cfg,"Front");
    m_mapFrameDir.at("Front") = boardValue;
    boardValue = m_conTab.getDirItem(cfg,"Rear");
    m_mapFrameDir.at("Rear") = boardValue;
    boardValue = m_conTab.getDirItem(cfg,"Left");
    m_mapFrameDir.at("Left") = boardValue;
    boardValue = m_conTab.getDirItem(cfg,"Right");
    m_mapFrameDir.at("Right") = boardValue;
    boardValue = m_conTab.getDirItem(cfg,"Top");
    m_mapFrameDir.at("Top") = boardValue;
    boardValue = m_conTab.getDirItem(cfg,"Bottom");
    m_mapFrameDir.at("Bottom") = boardValue;
}

void Cube::updateRotsToConfig(std::string cfg)
{
    Base::Console().Message("TRACE - Cube::updateRots(%s) \n",cfg.c_str());
    Base::Vector3d boardValue = m_conTab.getRotItem(cfg,"Front");
    m_mapFrameRot.at("Front") = boardValue;
    boardValue = m_conTab.getRotItem(cfg,"Rear");
    m_mapFrameRot.at("Rear") = boardValue;
    boardValue = m_conTab.getRotItem(cfg,"Left");
    m_mapFrameRot.at("Left") = boardValue;
    boardValue = m_conTab.getRotItem(cfg,"Right");
    m_mapFrameRot.at("Right") = boardValue;
    boardValue = m_conTab.getRotItem(cfg,"Top");
    m_mapFrameRot.at("Top") = boardValue;
    boardValue = m_conTab.getRotItem(cfg,"Bottom");
    m_mapFrameRot.at("Bottom") = boardValue;
}

bool Cube::validateBoard(std::string cfg)
{
//    Base::Console().Message("TRACE - Cube::validateBoard(%s)\n",cfg.c_str());
    bool result = true;
    //check that Dirs match
    std::string strCfgDir;
    std::string strBoardDir;
    
//    Base::Console().Message("TRACE - Cube::validateBoard(%s) - BoardDirCount: %d BoardRotCount: %d\n",
//                            cfg.c_str(),m_mapFrameDir.size(),m_mapFrameRot.size());
    for (auto& f: m_mapFrameDir) {
        Base::Vector3d boardValue = f.second;
        strBoardDir += dirToView(boardValue);
    }
    
    strCfgDir = m_conTab.getCfgDirStr(cfg);
//    Base::Console().Message("TRACE - Cube::validateBoard(%s) - Config Dirs: %s  Board Dirs: %s\n",
//                            cfg.c_str(),strCfgDir.c_str(),strBoardDir.c_str());
    if (strCfgDir != strBoardDir) {
        result = false;
        return result;
    }
    //check that Rots match
    std::string strCfgRot;
    std::string strBoardRot;
    
    for (auto& f: m_mapFrameRot) {
        Base::Vector3d boardValue = f.second;
        strBoardRot += dirToView(boardValue);
    }
    
    strCfgRot = m_conTab.getCfgRotStr(cfg);
    if (strCfgRot != strBoardRot) {
        result = false;
    }
//    Base::Console().Message("TRACE - Cube::validateBoard - Config Rots: %s  Board Rots: %s\n",strCfgRot.c_str(),strBoardRot.c_str());

//    Base::Console().Message("TRACE - Cube::validateBoard - result: %d\n",result);
    return result;
}

//dupl!!
std::string Cube::getBoardKey()
{
    std::string result; 
    Base::Vector3d frontDir = m_mapFrameDir.at("Front");
    std::string frontView = dirToView(frontDir);
    Base::Vector3d rightDir = m_mapFrameDir.at("Right");  
    std::string rightView = dirToView(rightDir);
    result = frontView + rightView;
    return result;
}

//get the current configuration on the board
std::string Cube::getCurrConfig(void)
{
    Base::Vector3d boardValue = m_mapFrameDir.at("Front");  //what's in the bucket "Front"
    std::string viewFront = dirToView(boardValue);
    boardValue = m_mapFrameDir.at("Right");                 
    std::string viewRight = dirToView(boardValue);
    std::string result = viewFront + viewRight;
//    Base::Console().Message("TRACE - Cube::getCurrCon - Result: %s Front: %s Right: %s\n",result.c_str(),viewFront.c_str(),viewRight.c_str());
//    for (auto& i : m_mapFrameDir) {
//        Base::Console().Message("m_mapFrameDir: %s - %s - %s\n",
//                                (i.first).c_str(),DrawUtil::formatVector(i.second).c_str(),dirToView(i.second).c_str());
//    }
    return result;
}

//std::string Cube::stdDirToFace(Base::Vector3d dir)
//{
//    std::string result;
////    int i = 0;
////    auto it = m_stdDirToFaceKeys.begin();                //map find and at don't much like vector3d
////    for (; it != m_stdDirToFaceKeys.end(); it++) {
////        Base::Vector3d key = (*it);
////        if ( key == dir ) {
////            result = m_stdDirToFaceData.at(i);
////            break;
////        }
////        i++;
////    }
//    return result;
//}

void Cube::saveSwap(std::string frame)
{
    m_swapDir = m_mapFrameDir.at(frame);
    m_swapRot = m_mapFrameRot.at(frame);
}

void Cube::restoreSwap(std::string frame)
{
    m_mapFrameDir.at(frame) = m_swapDir;
    m_mapFrameRot.at(frame) = m_swapRot;
}

void Cube::shiftFrame(std::string from, std::string to)
{
    m_mapFrameDir.at(to) = m_mapFrameDir.at(from);
    m_mapFrameRot.at(to) = m_mapFrameRot.at(from);
}


//dumps the current "board"
void Cube::dump(char * title)
{
//Bottom/Front/Left/Rear/Right/Top
// EACFDB
    Base::Console().Message("Cube: %s\n", title); 
    Base::Console().Message("B: %s/%s  \nF: %s/%s  \nL: %s/%s\n", 
                            DrawUtil::formatVector(getBottom()).c_str(),DrawUtil::formatVector(getBottomRot()).c_str(),
                            DrawUtil::formatVector(getFront()).c_str(),DrawUtil::formatVector(getFrontRot()).c_str(), 
                            DrawUtil::formatVector(getLeft()).c_str(),DrawUtil::formatVector(getLeftRot()).c_str());
    Base::Console().Message("K: %s/%s  \nR: %s/%s  \nT: %s/%s\n", 
                            DrawUtil::formatVector(getRear()).c_str(),DrawUtil::formatVector(getRearRot()).c_str(),
                            DrawUtil::formatVector(getRight()).c_str(),DrawUtil::formatVector(getRightRot()).c_str(), 
                            DrawUtil::formatVector(getTop()).c_str(),DrawUtil::formatVector(getTopRot()).c_str());
    std::string boardDirs = dirToView(getBottom()) + dirToView(getFront()) + dirToView(getLeft()) + 
                            dirToView(getRear()) + dirToView(getRight()) + dirToView(getTop());
    std::string boardRots = dirToView(getBottomRot()) + dirToView(getFrontRot()) + dirToView(getLeftRot()) + 
                            dirToView(getRearRot()) + dirToView(getRightRot()) + dirToView(getTopRot());
    std::string boardConfig = dirToView(getFront()) + dirToView(getRight());
    Base::Console().Message("Cube: Board State - config: %s Dirs: %s  Rots: %s\n",boardConfig.c_str(),boardDirs.c_str(),boardRots.c_str());
}

//dumps the config state
void Cube::dumpState(char* title)
{
    std::string cfg = getCurrConfig();
    std::string strCfgDir = m_conTab.getCfgDirStr(cfg);
    std::string strCfgRot = m_conTab.getCfgRotStr(cfg);
    Base::Console().Message("Cube: dumpState - %s - config: %s Dirs: %s  Rots: %s\n",title,cfg.c_str(),strCfgDir.c_str(),strCfgRot.c_str());
}

Base::Vector3d Cube::getRight()
{
    std::string myFace = "D";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Right");
    return result;
}

Base::Vector3d Cube::getFront()
{
    std::string myFace = "A";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Front");
    return result;
}

Base::Vector3d Cube::getTop()
{
    std::string myFace = "B";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Top");
    return result;
}

Base::Vector3d Cube::getLeft()
{
    std::string myFace = "C";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Left");
    return result;
}

Base::Vector3d Cube::getRear()
{
    std::string myFace = "F";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Rear");
    return result;
}

Base::Vector3d Cube::getBottom()
{
    std::string myFace = "E";
    Base::Vector3d result;
    result = m_mapFrameDir.at("Bottom");
    return result;
}

Base::Vector3d Cube::getRightRot()
{
    std::string myFace = "D";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Right");
    return result;
}

Base::Vector3d Cube::getFrontRot()
{
    std::string myFace = "A";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Front");
    return result;
}

Base::Vector3d Cube::getTopRot()
{
    std::string myFace = "B";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Top");
    return result;
}

Base::Vector3d Cube::getLeftRot()
{
    std::string myFace = "C";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Left");
    return result;
}

Base::Vector3d Cube::getRearRot()
{
    std::string myFace = "F";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Rear");
    return result;
}

Base::Vector3d Cube::getBottomRot()
{
    std::string myFace = "E";
    Base::Vector3d result;
    result = m_mapFrameRot.at("Bottom");
    return result;
}

//********************************************************
configLine::configLine(int ln, std::string ky, Base::Vector3d b, Base::Vector3d f,
                                               Base::Vector3d l, Base::Vector3d k,
                                               Base::Vector3d r, Base::Vector3d t)
{
    lineNumber = ln;
    key = ky;
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", b));
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Front",  f));
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Left",   l));
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Rear",   k));
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Right",  r));
    itemMap.insert(std::map<std::string, Base::Vector3d>::value_type("Top",    t));
}

Base::Vector3d configLine::getItem(std::string frame)
{
    return itemMap.at(frame);
}

std::string configLine::getString()
{
    //this outputs Bottom/Left/Front/Rear/Right/Top 
//    Base::Console().Message("TRACE - conLine::getString() - key: %s\n", key.c_str());
    std::string result;
    for (auto& i: itemMap) {
        Base::Vector3d itemVec = i.second;
        result += Cube::dirToView(itemVec);
//        Base::Console().Message("TRACE - %s - %s/%s\n",
//                                i.first.c_str(),DrawUtil::formatVector(i.second).c_str(), Cube::dirToView(itemVec).c_str());
    }
    return result;
}

void configLine::dump(char* title)
{
    Base::Console().Message("DUMP:  configLine %s key: %s\n",title,key.c_str());
    for (auto& i: itemMap) {
        Base::Console().Message(">>> %s - %s - %s\n",
                               i.first.c_str(),DrawUtil::formatVector(i.second).c_str(),Cube::dirToView(i.second).c_str());
    }
}

//********************************************************

std::string configTable::getCfgDirStr(std::string cfg)
{
    std::string result;
//    Base::Console().Message("TRACE - conTab::getCFGDirStr(%s)\n",cfg.c_str());
    configLine dirLine = getDirLine(cfg);
    result = dirLine.getString();
    return result;
}

std::string configTable::getCfgRotStr(std::string cfg)
{
    std::string result;
    configLine dirLine = getRotLine(cfg);
    result = dirLine.getString();
    return result;
}

configLine configTable::getDirLine(std::string key)
{
    return getLine(key, dirs);
}

configLine configTable::getRotLine(std::string k)
{
    return getLine(k,rots);
}

configLine configTable::getLine(std::string k, std::vector<configLine> list)
{
    configLine result;
//    Base::Console().Message("TRACE - conTab::getLine(%s)\n",k.c_str());
    for (auto& l: list) {
        if (k == l.getKey()) {
            result = l;
            break;
        }
    }
//    Base::Console().Message("result: %s\n",result.getString().c_str());
    return result;
}

Base::Vector3d configTable::getDirItem(std::string k, std::string v)
{
    return getItem(k, v, dirs);
}

Base::Vector3d configTable::getRotItem(std::string k, std::string v)
{
    return getItem(k, v, rots);
}

Base::Vector3d configTable::getItem(std::string k, std::string v, std::vector<configLine> list)
{
    Base::Vector3d result;
    configLine line = getLine(k, list);
    result = line.itemMap.at(v);
    return result;
}

void configTable::addDirItem(configLine cl)
{
    dirs.push_back(cl);
}

void configTable::addRotItem(configLine cl)
{
    rots.push_back(cl);
}

void configTable::initialize(void)
{
    Base::Console().Message("TRACE - cT::initialize()\n");
    configLine cl;
//Rotations
#include "rots.cpp"    
//Directions items
#include "dirs.cpp"

    Base::Console().Message("TRACE - cT::initialize dirs: %d rots %d\n",dirs.size(),rots.size());
}    

void configTable::dump(char* title)
{
    Base::Console().Message("DUMP: configTable - %s\n",title);
    for (auto& cl: dirs) {
        cl.dump("dirs - ");
    }
    for (auto& cl: rots) {
        cl.dump("rots - ");
    }
}


