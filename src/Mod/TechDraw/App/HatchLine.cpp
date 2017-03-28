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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <sstream>
#include <iomanip>
#include <QFile>
#include <QFileInfo>
#include <stdexcept>
#endif

#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>

#include <Base/Console.h>
#include <Base/Vector3D.h>

#include "Geometry.h"

#include "DrawUtil.h"
#include "HatchLine.h"

using namespace TechDraw;

double LineSet::getMinX(void)
{
    double xMin,yMin,zMin,xMax,yMax,zMax;
    m_box.Get(xMin,yMin,zMin,xMax,yMax,zMax);
    return xMin;
}

double LineSet::getMinY(void)
{
    double xMin,yMin,zMin,xMax,yMax,zMax;
    m_box.Get(xMin,yMin,zMin,xMax,yMax,zMax);
    return yMin;
}

double LineSet::getMaxX(void)
{
    double xMin,yMin,zMin,xMax,yMax,zMax;
    m_box.Get(xMin,yMin,zMin,xMax,yMax,zMax);
    return xMax;
}

double LineSet::getMaxY(void)
{
    double xMin,yMin,zMin,xMax,yMax,zMax;
    m_box.Get(xMin,yMin,zMin,xMax,yMax,zMax);
    return yMax;
}

bool LineSet::isDashed(void)
{
    bool result = m_hatchLine.isDashed();
    return result;
}

//TODO: needs to incorporate deltaX parameter
//! calculates the apparent start point for dashed lines
Base::Vector3d LineSet::calcApparentStart(TechDrawGeometry::BaseGeom* g)
{
    Base::Vector3d result;
    Base::Vector3d start(g->getStartPoint().x,g->getStartPoint().y,0.0);
    double angle = getPATLineSpec().getAngle();
    if (angle == 0.0) {             //horizontal 
        result = Base::Vector3d(getMinX(),start.y,0.0);
    } else if ((angle == 90.0) ||
               (angle == -90.0)) {  //vertical
        result = Base::Vector3d(start.x,getMinY(),0.0);
    } else {               
        double slope = getPATLineSpec().getSlope();
        double y     = getMinY();
        double x = ((y - start.y) / slope) + start.x;
        result = Base::Vector3d(x,y,0);
    }
    return result;
}


//*******************************************
PATLineSpec::PATLineSpec()
{
    init();
}

PATLineSpec::PATLineSpec(std::string& lineSpec)
{
    init();
    load(lineSpec);
}


PATLineSpec::~PATLineSpec()
{
}

void PATLineSpec::init(void)
{
    m_angle = 0.0;
    m_origin = Base::Vector3d(0.0,0.0,0.0);
    m_interval = 1.0;
    m_offset = 0.0;
}

void PATLineSpec::load(std::string& lineSpec)
{
    std::vector<double> values = split(lineSpec);
    if (values.size() < 5) {
        Base::Console().Message( "PATLineSpec::load(%s) invalid entry in pattern\n",lineSpec.c_str() );
        return;
    }
    m_angle    = values[0];
    m_origin = Base::Vector3d(values[1],values[2],0.0);
    m_offset   = values[3];
    m_interval = values[4];
    if (values.size() > 5) {
        std::vector<double> dash;
        dash.insert(std::end(dash), std::begin(values) + 5, std::end(values));
        m_dashParms = DashSpec(dash);
    }
}

std::vector<double> PATLineSpec::split(std::string line)
{
    std::vector<double>   result;
    std::stringstream     lineStream(line);
    std::string           cell;

    while(std::getline(lineStream,cell, ','))
    {
        try {
            result.push_back(std::stod(cell));
        }
        catch (const std::invalid_argument& ia) {
            Base::Console().Warning("Invalid number in cell: %s (%s) \n",cell.c_str(),ia.what());
            result.push_back(0.0);
        }
    }
    return result;
}

void PATLineSpec::dump(char* title)
{
    Base::Console().Message( "DUMP: %s\n",title);
    Base::Console().Message( "Angle: %.3f\n", m_angle);
    Base::Console().Message( "Origin: %s\n",DrawUtil::formatVector(m_origin).c_str());
    Base::Console().Message( "Offset: %.3f\n",m_offset);
    Base::Console().Message( "Interval: %.3f\n",m_interval);
//    std::stringstream ss;
//    for (auto& d: m_dashParms) {
//        ss << d << ", ";
//    }
//    ss << "end";
//    Base::Console().Message( "DashSpec: %s\n",ss.str().c_str());
    m_dashParms.dump("dashspec");
}

//static class methods
std::vector<PATLineSpec> PATLineSpec::getSpecsForPattern(std::string& parmFile, std::string& parmName)
{
    std::vector<PATLineSpec> result;
    std::vector<std::string> lineSpecs;
    std::ifstream inFile;
    inFile.open (parmFile, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().Message( "Cannot open input file.\n");
        return result;
    }

    //get all the definition lines for this pattern
    bool status = findPatternStart(inFile, parmName);
    if (status) {
        lineSpecs = loadPatternDef(inFile);
    } else {
        Base::Console().Message( "Could not find pattern: %s\n",parmName.c_str() );
        return result;
    }
    
    //decode definition lines into PATLineSpec objects
    for (auto& l: lineSpecs) {
        PATLineSpec hl(l);
        result.push_back(hl);
    }
    return result;
}

bool  PATLineSpec::findPatternStart(std::ifstream& inFile, std::string& parmName)
{
    bool result = false;
    while ( inFile.good() ){
         std::string line;
         std::getline(inFile,line);
         std::string nameTag = line.substr(0,1);
         std::string patternName;
         unsigned long int commaPos;
         if ((nameTag == ";")  ||
             (nameTag == " ")  ||
             (line.empty()) )  {           //is cr/lf empty?
             continue;
         } else if (nameTag == "*") {
             commaPos = line.find(",",1);
             if (commaPos != std::string::npos) {
                  patternName = line.substr(1,commaPos-1);
             } else {
                  patternName = line.substr(1);
             }
             if (patternName == parmName) {
                 //this is our pattern
                 result = true;
                 break;
             }
        }
    }  //endwhile
    return result;
}

//get the definition lines for this pattern
std::vector<std::string> PATLineSpec::loadPatternDef(std::ifstream& inFile)
{
    std::vector<std::string> result;
    while ( inFile.good() ){
        std::string line;
        std::getline(inFile,line);
        std::string nameTag = line.substr(0,1);
        if ((nameTag == ";")  ||
             (nameTag == " ") ||
             (line.empty()) )  {           //is cr/lf empty?
            continue;
        } else if (nameTag == "*") {
            break;
        } else {             //dataline
            result.push_back(line);
        }
    }
    return result;
}

std::vector<std::string> PATLineSpec::getPatternList(std::string& parmFile)
{
    std::vector<std::string> result;
    std::ifstream inFile;
    inFile.open (parmFile, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().Message( "Cannot open input file.\n");
        return result;
    }

    while ( inFile.good() ){
        std::string line;
        std::getline(inFile,line);
        std::string nameTag = line.substr(0,1);               //dupl code here
        unsigned long int commaPos;
        if (nameTag == "*") {  //found a pattern
            commaPos = line.find(",",1);
            std::string patternName;
            if (commaPos != std::string::npos) {
                 patternName = line.substr(1,commaPos-1);
            } else {
                  patternName = line.substr(1);
            }
            result.push_back(patternName);
        }
    }
    return result;
}

double PATLineSpec::getSlope(void)
{
    double angle = getAngle();

    //only dealing with angles -180:180 for now
    if (angle > 90.0) {
         angle = -(180.0 - angle);
    } else if (angle < -90.0) {
        angle = (180 + angle);
    }
    double slope = tan(angle * M_PI/180.0);
    return slope;
}

bool PATLineSpec::isDashed(void)
{
    bool result = !m_dashParms.empty();
    return result;
}

//********************************************************
void DashSpec::dump(char* title)
{
    std::stringstream ss;
    ss << title << ": " ;
    for (auto& p: m_parms) {
        ss << p << ", ";
    }
    Base::Console().Message("DUMP - DashSpec - %s\n",ss.str().c_str());
}
 




