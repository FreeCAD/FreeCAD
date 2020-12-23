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
 
//! HatchLine - Classes related to processing PAT files

#ifndef _TechDraw_HATCHLINE_H_
#define _TechDraw_HATCHLINE_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

#include <TopoDS_Edge.hxx>
#include <Bnd_Box.hxx>
#include <Base/Vector3D.h>

//class TopoDS_Edge;
//class Bnd_Box;

namespace TechDraw
{
class BaseGeom;
}

namespace TechDraw
{
class DrawViewPart;
class DrawUtil;

//DashSpec is the parsed portion of a PATLineSpec related to mark/space/dot
class TechDrawExport DashSpec
{
public: 
      DashSpec() {}
      DashSpec(std::vector<double> p) { m_parms = p; }
      ~DashSpec() {}
      
      double              get(int i)  {return m_parms.at(i); }
      std::vector<double> get(void)   {return m_parms;}
      bool                empty(void) {return m_parms.empty();}
      int                 size(void)  {return m_parms.size();}
      double              length(void);
      void                dump(char* title);
      DashSpec            reversed(void);
      
private:
    std::vector<double> m_parms;
};

//! PATLineSpec is the result of parsing a singleline from PAT file into accessible parameters
class TechDrawExport PATLineSpec
{
public:
    PATLineSpec();
    PATLineSpec(std::string& lineSpec);
    ~PATLineSpec();
    
    void load(std::string& lineSpec);

    double getAngle(void)  {return m_angle;}
    Base::Vector3d getOrigin(void) {return m_origin;}
    double getInterval(void) {return m_interval;}
    double getIntervalX(void);
    double getIntervalY(void);
    double getOffset(void)  {return m_offset;}
    double getSlope(void);
    double getLength(void) {return m_dashParms.length(); }
    DashSpec getDashParms(void) {return m_dashParms;}

    static std::vector<PATLineSpec> getSpecsForPattern(std::string& parmFile, std::string& parmName);
    static bool  findPatternStart(std::ifstream& inFile, std::string& parmName);
    static std::vector<std::string> loadPatternDef(std::ifstream& inFile);
    static std::vector<std::string> getPatternList(std::string& parmFile);
    
    bool isDashed(void);

    void dump(char* title);

private: 
    void init(void);
    std::vector<double> split(std::string line);
    //PAT line extracted tokens
    double m_angle;
    Base::Vector3d m_origin;
    double m_interval;
    double m_offset;
    DashSpec m_dashParms;
};

//! a LineSet is all the generated edges for 1 PATLineSpec for 1 Face
class TechDrawExport LineSet
{
public:
    LineSet() {}
    ~LineSet() {}
    
    void setPATLineSpec(PATLineSpec s) { m_hatchLine = s; }
    void setEdges(std::vector<TopoDS_Edge> e) {m_edges = e;}
    void setGeoms(std::vector<TechDraw::BaseGeom*>  g) {m_geoms = g;}
    void setBBox(Bnd_Box bb) {m_box = bb;}

    std::vector<TopoDS_Edge>    getEdges(void) { return m_edges; }
    TopoDS_Edge                 getEdge(int i) {return m_edges.at(i);}
    std::vector<TechDraw::BaseGeom*> getGeoms(void) { return m_geoms; }

    PATLineSpec       getPATLineSpec(void) { return m_hatchLine; }
    double            getOffset(void) { return m_hatchLine.getOffset(); }      //delta X offset
    double            getAngle(void)  { return m_hatchLine.getAngle(); }
    Base::Vector3d    getOrigin(void) { return m_hatchLine.getOrigin(); }      
    double            getInterval(void) {return m_hatchLine.getInterval(); }   //space between lines
    double            getIntervalX(void) { return m_hatchLine.getIntervalX(); }      //interval X-component
    double            getIntervalY(void) { return m_hatchLine.getIntervalY(); }      //interval Y-component
    double            getSlope(void)  { return m_hatchLine.getSlope(); }
    double            getPatternLength(void) { return m_hatchLine.getLength(); }
    Base::Vector3d    getUnitDir(void);
    Base::Vector3d    getUnitOrtho(void);
    DashSpec          getDashSpec(void) { return m_hatchLine.getDashParms();} 
    Base::Vector3d    calcApparentStart(TechDraw::BaseGeom* g);
    Base::Vector3d    findAtomStart(void);
    Base::Vector3d    getLineOrigin(void);                              //point corresponding to pattern origin for this line (O + n*intervalX)
    Base::Vector3d    getPatternStartPoint(TechDraw::BaseGeom* g, double &offset, double scale = 1.0);

    Bnd_Box getBBox(void) {return m_box;}
    double getMinX(void);
    double getMaxX(void);
    double getMinY(void);
    double getMaxY(void);

    bool isDashed(void);
    
private:
    std::vector<TopoDS_Edge> m_edges;
    std::vector<TechDraw::BaseGeom*> m_geoms;
    PATLineSpec m_hatchLine;
    Bnd_Box m_box;
};


} //end namespace

#endif
