/***************************************************************************
 *   Copyright (c) 2017 Wandererfan <wandererfan@gmail.com>                *
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

#ifndef _TechDraw_HATCHLINE_H_
#define _TechDraw_HATCHLINE_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

#include <Base/Vector3D.h>

class TopoDS_Edge;

namespace TechDrawGeometry
{
class BaseGeom;
}

namespace TechDraw
{
class DrawViewPart;
class DrawUtil;


// HatchLine is the result of parsing a line from PAT file into accessible parameters
// e /HatchLine/PATSpecLine/
class TechDrawExport HatchLine
{
public:
    HatchLine();
    HatchLine(std::string& lineSpec);
    ~HatchLine();
    
    void load(std::string& lineSpec);

    double getAngle(void)  {return m_angle;}
    Base::Vector3d getOrigin(void) {return m_origin;}
    double getInterval(void) {return m_interval;}
    double getOffset(void)  {return m_offset;}
    std::vector<double> getDashParms(void) {return m_dashParms;}

    static std::vector<HatchLine> getSpecsForPattern(std::string& parmFile, std::string& parmName);
    static bool  findPatternStart(std::ifstream& inFile, std::string& parmName);
    static std::vector<std::string> loadPatternDef(std::ifstream& inFile);
    static std::vector<std::string> getPatternList(std::string& parmFile);

    void dump(char* title);

private: 
    void init(void);
    std::vector<double> split(std::string line);
    //PAT line extracted tokens
    double m_angle;
    Base::Vector3d m_origin;
    double m_interval;
    double m_offset;
    std::vector<double> m_dashParms;   //why isn't this a DashSpec object?
};

// a LineSet is all the generated edges for 1 HatchLine for 1 Face
class TechDrawExport LineSet
{
public:
    LineSet() {}
    ~LineSet() {}
    
    void setHatchLine(HatchLine s) { m_hatchLine = s; }
    void setEdges(std::vector<TopoDS_Edge> e) {m_edges = e;}
    void setGeoms(std::vector<TechDrawGeometry::BaseGeom*>  g) {m_geoms = g;}

    HatchLine getHatchLine(void) { return m_hatchLine; }
    std::vector<double> getDashSpec(void) { return m_hatchLine.getDashParms();}
    std::vector<TopoDS_Edge> getEdges(void) { return m_edges; }
    std::vector<TechDrawGeometry::BaseGeom*> getGeoms(void) { return m_geoms; }
    //void clearGeom(void);
    
private:
    std::vector<TopoDS_Edge> m_edges;
    std::vector<TechDrawGeometry::BaseGeom*> m_geoms;
    HatchLine m_hatchLine;
};

class TechDrawExport DashSpec
{
public: 
      DashSpec() {}
      DashSpec(std::vector<double> p) { m_parms = p; }
      ~DashSpec() {}
      
      double get(int i) {return m_parms.at(i); }
      std::vector<double> get(void) {return m_parms;}
      bool empty(void) {return m_parms.empty();}
      int  size(void)  {return m_parms.size();}
      void dump(char* title);
      
private:
    std::vector<double> m_parms;
};

} //end namespace

#endif
