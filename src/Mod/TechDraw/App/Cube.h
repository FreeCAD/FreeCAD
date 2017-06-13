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

namespace TechDraw
{


class Cube
{
public:
    Cube();
    ~Cube();
    
void initialize();
void initialize(Base::Vector3d r, Base::Vector3d rr, Base::Vector3d l, Base::Vector3d lr,
                      Base::Vector3d f, Base::Vector3d fr, Base::Vector3d k, Base::Vector3d kr,      //k for bacK (rear)
                      Base::Vector3d t, Base::Vector3d tr, Base::Vector3d b, Base::Vector3d br);



    void rotateUp(double angle = 90.0);
    void rotateDown(double angle = 90.0);
    void rotateRight(double angle = 90.0);
    void rotateLeft (double angle = 90.0);
    void spinCCW(double angle = 90.0);
    void spinCW(double angle = 90.0);
    void updateIsoDirs();

    //viewDirection getters
    Base::Vector3d getViewDir(std::string name);
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

    //rotation getters
    Base::Vector3d getRotationDir(std::string name);
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
    
    void dump(char * title);
    void dumpISO(char * title);
    
    std::vector<Base::Vector3d> getAllDirs(void);
    std::vector<Base::Vector3d> getAllRots(void);
    void setAllDirs(std::vector<Base::Vector3d> dirs);
    void setAllRots(std::vector<Base::Vector3d> rots);

private:
    //the current state of the "cube"
    //the "cube" is always full - entries for every ortho position (6 total)
    std::map<std::string, Base::Vector3d> m_mapCubeDir;
    std::map<std::string, Base::Vector3d> m_mapCubeRot;

    static const std::map<std::string,Base::Vector3d> m_viewDefault;
    static const std::map<std::string,Base::Vector3d> m_rotDefault;

//should be in drawutil?
    Base::Vector3d rodrigues(Base::Vector3d v,
                             double angle,
                             Base::Vector3d axis);

};

}
#endif


