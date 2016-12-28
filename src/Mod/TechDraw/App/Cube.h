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
#ifndef _CUBE_H_
#define _CUBE_H_

#include <map>

#include <Base/Vector3D.h>

namespace TechDraw {

class configLine 
{
public:
    configLine() {}
    ~configLine() {}
    
    configLine(int ln, std::string ky,  Base::Vector3d b, Base::Vector3d f,
                                        Base::Vector3d l, Base::Vector3d k,
                                        Base::Vector3d r, Base::Vector3d t);
    Base::Vector3d getItem(std::string s);
    std::string getKey(void) { return key; }
    std::string getString();
    void dump(char* title);
    
    std::map<std::string, Base::Vector3d> itemMap;
//    Base::Vector3d getItem(std::string s);    
private:
    int lineNumber;
    std::string key;
//    std::map<std::string, Base::Vector3d> itemMap;
};

class configTable
{
public:
    configTable(void) {}
    ~configTable(void) {}
    void initialize();
    std::string getCfgDirStr(std::string cfg);
    std::string getCfgRotStr(std::string cfg);
    
    configLine getDirLine(std::string k);
    configLine getRotLine(std::string k);
    Base::Vector3d getDirItem(std::string k, std::string v);
    Base::Vector3d getRotItem(std::string k, std::string v);
    void addDirItem(configLine);
    void addRotItem(configLine);
    //bool checkConfig(std::vector<std::string> c);
    void dump(char* title);
    
private:
    configLine getLine(std::string k, std::vector<configLine> list);
    Base::Vector3d getItem(std::string k, std::string v, std::vector<configLine> list);
    std::vector<configLine> dirs;
    std::vector<configLine> rots;
};


class Cube
{
public:
    Cube();
    ~Cube();
    
void initialize(Base::Vector3d r, Base::Vector3d rr, Base::Vector3d l, Base::Vector3d lr,
                      Base::Vector3d f, Base::Vector3d fr, Base::Vector3d k, Base::Vector3d kr,      //k for bacK (rear)
                      Base::Vector3d t, Base::Vector3d tr, Base::Vector3d b, Base::Vector3d br,
                      Base::Vector3d fbl, Base::Vector3d fblr, Base::Vector3d fbr, Base::Vector3d fbrr,
                      Base::Vector3d ftl, Base::Vector3d ftlr, Base::Vector3d ftr, Base::Vector3d ftrr);



    void rotateUp();
    void rotateDown();
    void rotateRight();
    void rotateLeft ();
    void spinCCW();
    void spinCW();
    
    void updateIsoDirs();

    Base::Vector3d getRight();
    Base::Vector3d getFront();
    Base::Vector3d getTop();
    Base::Vector3d getLeft();
    Base::Vector3d getRear();
    Base::Vector3d getBottom();
    Base::Vector3d getFBL();
    Base::Vector3d getFBR();
    Base::Vector3d getFTL();
    Base::Vector3d getFTR();

    Base::Vector3d getRightRot();
    Base::Vector3d getFrontRot();
    Base::Vector3d getTopRot();
    Base::Vector3d getLeftRot();
    Base::Vector3d getRearRot();
    Base::Vector3d getBottomRot();
    Base::Vector3d getFBLRot();
    Base::Vector3d getFBRRot();
    Base::Vector3d getFTLRot();
    Base::Vector3d getFTRRot();
    
    static std::string dirToView(Base::Vector3d v);
    void updateDirsToConfig(std::string cfg);
    void updateRotsToConfig(std::string cfg);
    bool validateBoard(std::string cfg);
    std::string getCurrConfig(void);
    
    void dump(char * title);
    void dumpISO(char * title);
    void dumpState(char * title);

private:
//    Base::Vector3d currRight;
//    Base::Vector3d currRightRot;
//    Base::Vector3d currFront;
//    Base::Vector3d currFrontRot;
//    Base::Vector3d currTop;
//    Base::Vector3d currTopRot;
//    Base::Vector3d currLeft;
//    Base::Vector3d currLeftRot;
//    Base::Vector3d currRear;
//    Base::Vector3d currRearRot;
//    Base::Vector3d currBottom;
//    Base::Vector3d currBottomRot;
    
    
    //the current state of the "board"
    //the "board" is always full - entries for every position
    std::map<std::string, Base::Vector3d> m_mapFrameDir;
    std::map<std::string, Base::Vector3d> m_mapFrameRot;

    static const std::map<std::string,Base::Vector3d> m_viewToStdDir;      //would really like this map the other way round. 

    void shiftFrame(std::string from, std::string to);
    void saveSwap(std::string frame);
    void restoreSwap(std::string frame);
    Base::Vector3d m_swapDir;
    Base::Vector3d m_swapRot;
    configTable m_conTab;
    
};


}
#endif


