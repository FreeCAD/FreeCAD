/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TechDraw_HATCHLINE_H_
#define TechDraw_HATCHLINE_H_

#include <string>
#include <vector>

#include <Bnd_Box.hxx>
#include <TopoDS_Edge.hxx>

#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "Geometry.h"


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
      explicit DashSpec(std::vector<double> p) { m_parms = p; }
      ~DashSpec() = default;

      double              get(int i)  {return m_parms.at(i); }
      std::vector<double> get()   {return m_parms;}
      bool                empty() {return m_parms.empty();}
      int                 size()  {return m_parms.size();}
      double              length();
      void                dump(const char* title);
      DashSpec            reversed();

private:
    std::vector<double> m_parms;
};

//! PATLineSpec is the result of parsing a singleline from PAT file into accessible parameters
class TechDrawExport PATLineSpec
{
public:
    PATLineSpec();
    explicit PATLineSpec(std::string& lineSpec);
    ~PATLineSpec();

    void load(std::string& lineSpec);

    double getAngle()  {return m_angle;}
    Base::Vector3d getOrigin() {return m_origin;}
    double getInterval() {return m_interval;}
    double getIntervalX();
    double getIntervalY();
    double getOffset()  {return m_offset;}
    double getSlope();
    double getLength() {return m_dashParms.length(); }
    DashSpec getDashParms() {return m_dashParms;}

    static std::vector<PATLineSpec> getSpecsForPattern(std::string& parmFile, std::string& parmName);
    static bool  findPatternStart(std::ifstream& inFile, std::string& parmName);
    static std::vector<std::string> loadPatternDef(std::ifstream& inFile);
    static std::vector<std::string> getPatternList(std::string& parmFile);

    bool isDashed();

    void dump(const char* title);

private:
    void init();
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
    ~LineSet() = default;

    void setPATLineSpec(const PATLineSpec& s) { m_hatchLine = s; }
    void setEdges(std::vector<TopoDS_Edge> e) {m_edges = e;}
    void setGeoms(std::vector<TechDraw::BaseGeomPtr>  g) {m_geoms = g;}
    void setBBox(const Bnd_Box& bb) {m_box = bb;}

    std::vector<TopoDS_Edge>    getEdges() { return m_edges; }
    TopoDS_Edge                 getEdge(int i) {return m_edges.at(i);}
    std::vector<TechDraw::BaseGeomPtr> getGeoms() { return m_geoms; }

    PATLineSpec       getPATLineSpec() { return m_hatchLine; }
    double            getOffset() { return m_hatchLine.getOffset(); }      //delta X offset
    double            getAngle()  { return m_hatchLine.getAngle(); }
    Base::Vector3d    getOrigin() { return m_hatchLine.getOrigin(); }
    double            getInterval() {return m_hatchLine.getInterval(); }   //space between lines
    double            getIntervalX() { return m_hatchLine.getIntervalX(); }      //interval X-component
    double            getIntervalY() { return m_hatchLine.getIntervalY(); }      //interval Y-component
    double            getSlope()  { return m_hatchLine.getSlope(); }
    double            getPatternLength() { return m_hatchLine.getLength(); }
    Base::Vector3d    getUnitDir();
    Base::Vector3d    getUnitOrtho();
    DashSpec          getDashSpec() { return m_hatchLine.getDashParms();}
    Base::Vector3d    calcApparentStart(TechDraw::BaseGeomPtr g);
    Base::Vector3d    findAtomStart();
    Base::Vector3d    getLineOrigin();                              //point corresponding to pattern origin for this line (O + n*intervalX)
    Base::Vector3d    getPatternStartPoint(TechDraw::BaseGeomPtr g, double &offset, double scale = 1.0);

    Bnd_Box getBBox() {return m_box;}
    double getMinX();
    double getMaxX();
    double getMinY();
    double getMaxY();

    bool isDashed();

private:
    std::vector<TopoDS_Edge> m_edges;
    std::vector<TechDraw::BaseGeomPtr> m_geoms;
    PATLineSpec m_hatchLine;
    Bnd_Box m_box;
};


} //end namespace

#endif
