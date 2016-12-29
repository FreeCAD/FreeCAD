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

// D/C/A/F/B/E/FBL/FBR/FTL/FTR
void Cube::initialize(Base::Vector3d r, Base::Vector3d rr, Base::Vector3d l, Base::Vector3d lr,
                      Base::Vector3d f, Base::Vector3d fr, Base::Vector3d k, Base::Vector3d kr,      //k for bacK (rear)
                      Base::Vector3d t, Base::Vector3d tr, Base::Vector3d b, Base::Vector3d br,
                      Base::Vector3d fbl, Base::Vector3d fblr, Base::Vector3d fbr, Base::Vector3d fbrr,
                      Base::Vector3d ftl, Base::Vector3d ftlr, Base::Vector3d ftr, Base::Vector3d ftrr)
{
    //these frames are only used at DPGI creation time? 
    m_mapFrameDir.clear();
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", b));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , f));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , l));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , k));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , r));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , t));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomLeft" , fbl));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomRight", fbr));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopLeft"    , ftl));
    m_mapFrameDir.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopRight"   , ftr));

    m_mapFrameRot.clear();
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Bottom", br));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Front" , fr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Left"  , lr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Rear"  , kr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Right" , rr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("Top"   , tr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopRight"   , ftrr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontTopLeft"    , ftlr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomLeft" , fblr));
    m_mapFrameRot.insert(std::map<std::string, Base::Vector3d>::value_type("FrontBottomRight", fbrr));
    
    m_conTab.initialize();   //all possible configs of ABCDEF in bottom/front/left/(k)rear/right/top order
}

void Cube::rotateUp()
{
    //Front -> Top -> Rear -> Bottom -> Front???
    saveSwap("Front");
    shiftFrame("Bottom", "Front");
    shiftFrame("Rear"  , "Bottom");
    shiftFrame("Top"   , "Rear");
    restoreSwap("Top");

    updateIsoDirs();                       //calculatge iso directions from ortho dirs
    updateRotsToConfig(getCurrConfig());   //update rotations for ortho views from config table
}    

void Cube::rotateDown()
{
    //Front -> Bottom -> Rear -> Top -> Front???
    saveSwap("Front");
    shiftFrame("Top"    , "Front");
    shiftFrame("Rear"   , "Top");
    shiftFrame("Bottom" , "Rear");
    restoreSwap("Bottom");

    updateIsoDirs();
    updateRotsToConfig(getCurrConfig());
} 

void Cube::rotateRight()
{
    //Front -> Right -> Rear -> Left -> Front???
    saveSwap("Front");
    shiftFrame("Left"  , "Front");
    shiftFrame("Rear"  , "Left");
    shiftFrame("Right" , "Rear");
    restoreSwap("Right");

    updateIsoDirs();
    updateRotsToConfig(getCurrConfig());
} 
   
void Cube::rotateLeft()
{
    //Front -> Left -> Rear -> Right -> Front???
    saveSwap("Front");
    shiftFrame("Right", "Front");
    shiftFrame("Rear" , "Right");
    shiftFrame("Left" , "Rear");
    restoreSwap("Left");

    updateIsoDirs();
    updateRotsToConfig(getCurrConfig());
} 

void Cube::spinCCW()
{
    //Right -> Top -> Left -> Bottom -> Right???
    saveSwap("Right");
    shiftFrame("Bottom", "Right");
    shiftFrame("Left"  , "Bottom");
    shiftFrame("Top"   , "Left");
    restoreSwap("Top");

    updateIsoDirs();
    updateRotsToConfig(getCurrConfig());
} 

void Cube::spinCW()
{
    //Left -> Top -> Right -> Bottom -> Left??? 
    saveSwap("Left");
    shiftFrame("Bottom", "Left");
    shiftFrame("Right" , "Bottom");
    shiftFrame("Top", "Right");
    restoreSwap("Top");

    updateIsoDirs();
    updateRotsToConfig(getCurrConfig());
} 

void Cube::updateIsoDirs() 
{
    Base::Vector3d flb = getFront() + getLeft()  + getBottom();
    Base::Vector3d frb = getFront() + getRight() + getBottom();
    Base::Vector3d flt = getFront() + getLeft()  + getTop();
    Base::Vector3d frt = getFront() + getRight() + getTop();
    m_mapFrameDir.at("FrontBottomLeft")  = flb;
    m_mapFrameDir.at("FrontBottomRight") = frb;
    m_mapFrameDir.at("FrontTopLeft")     = flt;
    m_mapFrameDir.at("FrontTopRight")    = frt;
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
    bool result = true;
    //check that Dirs match
    std::string strCfgDir;
    std::string strBoardDir;
    for (auto& f: m_mapFrameDir) {
        Base::Vector3d boardValue = f.second;
        strBoardDir += dirToView(boardValue);
    }
    
    strCfgDir = m_conTab.getCfgDirStr(cfg);
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
    return result;
}

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

//dumps the current "board" -ISO's
void Cube::dumpISO(char * title)
{
//FBL/FBR/FTL/FTR
//
    Base::Console().Message("Cube ISO: %s\n", title); 
    Base::Console().Message("FBL: %s/%s  \nFBR: %s/%s  \nFTL: %s/%s\nFTR: %s/%s\n", 
                            DrawUtil::formatVector(getFBL()).c_str(),DrawUtil::formatVector(getFBLRot()).c_str(),
                            DrawUtil::formatVector(getFBR()).c_str(),DrawUtil::formatVector(getFBRRot()).c_str(), 
                            DrawUtil::formatVector(getFTL()).c_str(),DrawUtil::formatVector(getFTLRot()).c_str(), 
                            DrawUtil::formatVector(getFTR()).c_str(),DrawUtil::formatVector(getFTRRot()).c_str());
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
    Base::Vector3d result;
    result = m_mapFrameDir.at("Right");
    return result;
}

Base::Vector3d Cube::getFront()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("Front");
    return result;
}

Base::Vector3d Cube::getTop()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("Top");
    return result;
}

Base::Vector3d Cube::getLeft()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("Left");
    return result;
}

Base::Vector3d Cube::getRear()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("Rear");
    return result;
}

Base::Vector3d Cube::getBottom()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("Bottom");
    return result;
}

Base::Vector3d Cube::getFBL()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("FrontBottomLeft");
    return result;
}

Base::Vector3d Cube::getFBR()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("FrontBottomRight");
    return result;
}

Base::Vector3d Cube::getFTL()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("FrontTopLeft");
    return result;
}

Base::Vector3d Cube::getFTR()
{
    Base::Vector3d result;
    result = m_mapFrameDir.at("FrontTopRight");
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
    Base::Vector3d result;
    result = m_mapFrameRot.at("Front");
    return result;
}

Base::Vector3d Cube::getTopRot()
{
    Base::Vector3d result;
    result = m_mapFrameRot.at("Top");
    return result;
}

Base::Vector3d Cube::getLeftRot()
{
    Base::Vector3d result;
    result = m_mapFrameRot.at("Left");
    return result;
}

Base::Vector3d Cube::getRearRot()
{
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

Base::Vector3d Cube::getFBLRot()
{
    Base::Vector3d result;
    double magic1 = 157.5 * M_PI / 180.0;                // 90 + 45 + magic1
    double magic2 = -17.632 * M_PI / 180.0;              //±35.264° / 2 "magic angle"??  
                                                         // <<https://en.wikipedia.org/wiki/Isometric_projection#Overview
    Base::Vector3d up = getTop();
    Base::Vector3d view = getFBL();
    Base::Vector3d cross = up.Cross(view);
    Base::Vector3d rot1  = DrawUtil::vecRotate(view,magic1,cross);
    Base::Vector3d rot2  = DrawUtil::vecRotate(rot1,magic2,up);
    result = rot2;
    Base::Vector3d viewA = getFront();
    if ((viewA == m_viewToStdDir.at("C")) ||
        (viewA == m_viewToStdDir.at("D"))) {
        result = Base::Vector3d(-1.0*result.x,-1.0*result.y,result.z);
    }
    return result;
}

Base::Vector3d Cube::getFBRRot()
{
    Base::Vector3d result;
    double magic1 = -22.5 * M_PI / 180.0;               //45*/2
    double magic2 = 17.632 * M_PI / 180.0;              //±35.264° / 2 "magic angle"
    Base::Vector3d up = getTop();
    Base::Vector3d view = getFBR();
    Base::Vector3d cross = up.Cross(view);
    Base::Vector3d rot1  = DrawUtil::vecRotate(view,magic1,cross);
    Base::Vector3d rot2  = DrawUtil::vecRotate(rot1,magic2,up);
    result = rot2;
    Base::Vector3d viewA = getFront();
    if ((viewA == m_viewToStdDir.at("C")) ||
        (viewA == m_viewToStdDir.at("D"))) {
        result = Base::Vector3d(-1.0*result.x,-1.0*result.y,result.z);
    }
    return result;
}

Base::Vector3d Cube::getFTLRot()
{
    Base::Vector3d result;
    double magic1 = -157.5 * M_PI / 180.0;
    double magic2 = -17.632 * M_PI / 180.0;
    
    Base::Vector3d up = getTop();
    Base::Vector3d view = getFTL();
    Base::Vector3d cross = up.Cross(view);
    Base::Vector3d rot1  = DrawUtil::vecRotate(view,magic1,cross);
    Base::Vector3d rot2  = DrawUtil::vecRotate(rot1,magic2,up);
    result = rot2;
    Base::Vector3d viewA = getFront();
    if ((viewA == m_viewToStdDir.at("C")) ||
        (viewA == m_viewToStdDir.at("D"))) {
        result = Base::Vector3d(-1.0*result.x,-1.0*result.y,result.z);
    }
    return result;
}

Base::Vector3d Cube::getFTRRot()
{
    Base::Vector3d result;
    double magic1 = 22.5 * M_PI / 180.0;
    double magic2 = 17.632 * M_PI / 180.0;
    Base::Vector3d up = getTop();
    Base::Vector3d view = getFTR();
    Base::Vector3d cross = up.Cross(view);
    Base::Vector3d rot1  = DrawUtil::vecRotate(view,magic1,cross);
    Base::Vector3d rot2  = DrawUtil::vecRotate(rot1,magic2,up);
    result = rot2;
    Base::Vector3d viewA = getFront();
    if ((viewA == m_viewToStdDir.at("C")) ||
        (viewA == m_viewToStdDir.at("D"))) {
        result = Base::Vector3d(-1.0*result.x,-1.0*result.y,result.z);
    }
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
    std::string result;
    for (auto& i: itemMap) {
        Base::Vector3d itemVec = i.second;
        result += Cube::dirToView(itemVec);
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
    for (auto& l: list) {
        if (k == l.getKey()) {
            result = l;
            break;
        }
    }
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
    dirs.clear();
    rots.clear();
    configLine cl;

//Rotations
// Rots - b/f/l/k/r/t
    cl = configLine( 1 , "AB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 2 , "AC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 3 , "AD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 4 , "AE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 5 , "BA", Base::Vector3d(0,-1,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 6 , "BC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 7 , "BD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 8 , "BF", Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 9 , "CA", Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 10 , "CB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 11 , "CE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 12 , "CF", Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0) );
    addRotItem(cl);
    cl = configLine( 13 , "DA", Base::Vector3d(0,1,0), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 14 , "DB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 15 , "DE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 16 , "DF", Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0) );
    addRotItem(cl);
    cl = configLine( 17 , "EA", Base::Vector3d(0,1,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 18 , "EC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 19 , "ED", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 20 , "EF", Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,1,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 21 , "FB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 22 , "FC", Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 23 , "FD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 24 , "FE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);


//Directions items
// Dirs  - b/f/l/k/r/t
    cl = configLine( 1 , "AB", Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), 
                               Base::Vector3d(0,1,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0) );
    addDirItem(cl);
    cl = configLine( 2 , "AC", Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0), 
                               Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1) );
    addDirItem(cl);
    cl = configLine( 3 , "AD", Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), 
                               Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1) );
    addDirItem(cl);
    cl = configLine( 4 , "AE", Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), 
                               Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0) );
    addDirItem(cl);
    cl = configLine( 5 , "BA", Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), 
                               Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0) );
    addDirItem(cl);
    //BC = FBDECA
    cl = configLine( 6 , "BC", Base::Vector3d(0,1,0), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), 
                               Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0) );
    addDirItem(cl);
    cl = configLine( 7 , "BD", Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), 
                               Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), Base::Vector3d(0,1,0) );
    addDirItem(cl);
    cl = configLine( 8 , "BF", Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), 
                               Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0) );
    addDirItem(cl);
    cl = configLine( 9 , "CA", Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), 
                               Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1) );
    addDirItem(cl);
    cl = configLine( 10 , "CB", Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0) );
    addDirItem(cl);
    cl = configLine( 11 , "CE", Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0) );
    addDirItem(cl);
    cl = configLine( 12 , "CF", Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1) );
    addDirItem(cl);
    cl = configLine( 13 , "DA", Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1) );
    addDirItem(cl);
    cl = configLine( 14 , "DB", Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0) );
    addDirItem(cl);
    cl = configLine( 15 , "DE", Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0) );
    addDirItem(cl);
    cl = configLine( 16 , "DF", Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,1) );
    addDirItem(cl);
    cl = configLine( 17 , "EA", Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0) );
    addDirItem(cl);
    cl = configLine( 18 , "EC", Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0) );
    addDirItem(cl);
    cl = configLine( 19 , "ED", Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0) );
    addDirItem(cl);
    cl = configLine( 20 , "EF", Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0) );
    addDirItem(cl);
    cl = configLine( 21 , "FB", Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0) );
    addDirItem(cl);
    cl = configLine( 22 , "FC", Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1) );
    addDirItem(cl);
    cl = configLine( 23 , "FD", Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1) );
    addDirItem(cl);
    cl = configLine( 24 , "FE", Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0) );
    addDirItem(cl);
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


