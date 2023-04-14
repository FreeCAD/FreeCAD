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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#endif

#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>

#include <Base/Console.h>
#include <Base/Stream.h>
#include <Base/Vector3D.h>

#include "HatchLine.h"
#include "DrawUtil.h"


using namespace TechDraw;

double LineSet::getMinX()
{
    double xMin, yMin, zMin, xMax, yMax, zMax;
    m_box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    return xMin;
}

double LineSet::getMinY()
{
    double xMin, yMin, zMin, xMax, yMax, zMax;
    m_box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    return yMin;
}

double LineSet::getMaxX()
{
    double xMin, yMin, zMin, xMax, yMax, zMax;
    m_box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    return xMax;
}

double LineSet::getMaxY()
{
    double xMin, yMin, zMin, xMax, yMax, zMax;
    m_box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    return yMax;
}

bool LineSet::isDashed()
{
    return m_hatchLine.isDashed();
}

//! calculates the apparent start point (ie start of overlay line) for dashed lines
Base::Vector3d LineSet::calcApparentStart(TechDraw::BaseGeomPtr g)
{
    Base::Vector3d result;
    Base::Vector3d start(g->getStartPoint().x, g->getStartPoint().y, 0.0);
    double angle = getPATLineSpec().getAngle();
    if (angle == 0.0) {             //horizontal
        result = Base::Vector3d(getMinX(), start.y, 0.0);
    } else if ((angle == 90.0) ||
               (angle == -90.0)) {  //vertical
        result = Base::Vector3d(start.x, getMinY(), 0.0);
    } else {
        double slope = getPATLineSpec().getSlope();
        double y     = getMinY();
        double x = ((y - start.y) / slope) + start.x;
        result = Base::Vector3d(x, y,0);
    }
    return result;
}

Base::Vector3d LineSet::getUnitDir()
{
    Base::Vector3d result;
    Base::Vector3d start(m_geoms.at(0)->getStartPoint().x,
                         m_geoms.at(0)->getStartPoint().y,
                         0.0);
    Base::Vector3d end(m_geoms.at(0)->getEndPoint().x,
                         m_geoms.at(0)->getEndPoint().y,
                         0.0);
    result = end - start;
    result.Normalize();
    return result;
}

Base::Vector3d LineSet::getUnitOrtho()
{
    Base::Vector3d result;
    Base::Vector3d unit = getUnitDir();
    Base::Vector3d X(1.0, 0.0, 0.0);
    Base::Vector3d Y(0.0, 1.0, 0.0);
    if (unit.IsEqual(X, 0.000001)) {
        result = Y;
    } else if (unit.IsEqual(Y, 0.000001)) {
        result = X;
    } else {
        double unitX = unit.x;
        double unitY = unit.y;
        result = Base::Vector3d(unitY, -unitX, 0.0);  //perpendicular
    }
    result.Normalize();   //probably redundant
    return result;
}


Base::Vector3d LineSet::findAtomStart()
{
    Base::Vector3d result;
    Base::Vector3d origin = getOrigin();
    double angle = getAngle();
    if (angle == 0.0) {
        result = Base::Vector3d(getMinX(), origin.y, 0.0);
    } else if ( (angle == 90.0) ||
                (angle == -90.0) ) {
        result = Base::Vector3d(origin.x, getMinY(), 0.0);
    } else {
        double minY = getMinY();
        double x = origin.x - (origin.y - minY)/getSlope();
        result = Base::Vector3d(x, minY, 0.0);
    }
    return result;
}

Base::Vector3d LineSet::getPatternStartPoint(TechDraw::BaseGeomPtr g, double &offset, double scale)
{
    Base::Vector3d result = getOrigin();
    Base::Vector3d atomStart = findAtomStart();
    Base::Vector3d thisStart = calcApparentStart(g);
    double angle = getAngle();
    double patternLength = scale * getPatternLength();
    Base::Vector3d lineOrigin;
    double distX, distY;
    int overlayIndex = 0;
    //interval & offset are not patternscaled

//figure out which line this is in the overlay
    if (angle == 0.0) {      //odd case - horizontal line
        distY = (thisStart.y - atomStart.y);                                       //this is patternscaled
        overlayIndex = (int) round(distY/(scale * getIntervalY()));                //this is +/-
        lineOrigin = getOrigin() + distY * Base::Vector3d(0.0, 1.0, 0.0);                    //move u/d
    } else {
        distX = (thisStart.x - atomStart.x);                                       //this is patternscaled
        overlayIndex = (int) round(distX/(scale * getIntervalX()));
        lineOrigin = getOrigin() + overlayIndex * (scale * getInterval()) * getUnitOrtho();
    }

    lineOrigin = lineOrigin + (overlayIndex * (scale * getOffset())) * getUnitDir();            //move along line

    //is lineOrigin on line of g? should be within fp error
    Base::Vector3d gStart(g->getStartPoint().x,
                          g->getStartPoint().y,
                          0.0);
    Base::Vector3d gEnd(g->getEndPoint().x,
                        g->getEndPoint().y,
                        0.0);
    double lenStartOrg = (gStart - lineOrigin).Length();
    double lenEndOrg = (gEnd   - lineOrigin).Length();
    double lenStartEnd = (gStart - gEnd).Length();
    if ( (lenStartOrg <= lenStartEnd) &&
         (lenEndOrg <= lenStartEnd) )  {                                //origin is in g
        result = lineOrigin;
        offset = 0.0;
    } else {
        //find a point where pattern repeats within g
        double patsStartOrg = lenStartOrg/patternLength;                   //# pattern repeats from lineOrigin to start
        double patsEndOrg = lenEndOrg/patternLength;
        if (lenStartOrg < lenEndOrg) {                                  //origin is before start
            double c = ceil(patsStartOrg);
            if (c <= patsEndOrg) {                                      //c is an integer pattern count in [patsStartOrg, patsEndOrg]
                result = lineOrigin + c*patternLength*getUnitDir();
                offset = 0.0;
            } else {
//                //ugly case - partial pattern
                result = gStart;
                offset = patsStartOrg - (int)patsStartOrg;        //fraction of a patternLength
                offset = offset * patternLength;
            }
        } else if (lenStartOrg > lenEndOrg) {                          //origin is after end
            double c = ceil(patsEndOrg);
            if (c <= patsStartOrg) {                      //c is an integer pattern count in [patsStartOrg, patsEndOrg]
                result = lineOrigin - c*patternLength*getUnitDir();
                offset = 0.0;
            } else {
                result = gStart;
                offset = ceil(patsStartOrg) - patsStartOrg;      //fraction of a patternLength patstartorg to repeat point
                offset = offset * patternLength;
            }
        }
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

void PATLineSpec::init()
{
    m_angle = 0.0;
    m_origin = Base::Vector3d(0.0, 0.0, 0.0);
    m_interval = 1.0;
    m_offset = 0.0;
}

void PATLineSpec::load(std::string& lineSpec)
{
    std::vector<double> values = split(lineSpec);
    if (values.size() < 5) {
        Base::Console().Message( "PATLineSpec::load(%s) invalid entry in pattern\n", lineSpec.c_str() );
        return;
    }
    m_angle    = values[0];
    m_origin = Base::Vector3d(values[1], values[2], 0.0);
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

    while(std::getline(lineStream, cell, ','))
    {
        try {
            result.push_back(std::stod(cell));
        }
        catch (const std::invalid_argument& ia) {
            Base::Console().Warning("Invalid number in cell: %s (%s) \n", cell.c_str(), ia.what());
            result.push_back(0.0);
        }
    }
    return result;
}

void PATLineSpec::dump(const char* title)
{
    Base::Console().Message( "DUMP: %s\n", title);
    Base::Console().Message( "Angle: %.3f\n", m_angle);
    Base::Console().Message( "Origin: %s\n", DrawUtil::formatVector(m_origin).c_str());
    Base::Console().Message( "Offset: %.3f\n", m_offset);
    Base::Console().Message( "Interval: %.3f\n", m_interval);
//    std::stringstream ss;
//    for (auto& d: m_dashParms) {
//        ss << d << ", ";
//    }
//    ss << "end";
//    Base::Console().Message( "DashSpec: %s\n", ss.str().c_str());
    m_dashParms.dump("dashspec");
}

//static class methods
std::vector<PATLineSpec> PATLineSpec::getSpecsForPattern(std::string& parmFile, std::string& parmName)
{
    std::vector<std::string> lineSpecs;
    Base::FileInfo fi(parmFile);
    Base::ifstream inFile;
    inFile.open(fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().Message("Cannot open input file.\n");
        return std::vector<PATLineSpec>();
    }

    //get all the definition lines for this pattern
    bool status = findPatternStart(inFile, parmName);
    if (!status) { // This can come up when changing PAT file or pattern name
        return std::vector<PATLineSpec>();
    }
    lineSpecs = loadPatternDef(inFile);

    //decode definition lines into PATLineSpec objects
    std::vector<PATLineSpec> result;
    for (auto& l: lineSpecs) {
        PATLineSpec hl(l);
        result.push_back(hl);
    }
    return result;
}

bool  PATLineSpec::findPatternStart(std::ifstream& inFile, std::string& parmName)
{
//    Base::Console().Message("HL::findPatternStart() - parmName: %s\n", parmName.c_str());
    while (inFile.good() ){
         std::string line;
         std::getline(inFile, line);
         std::string nameTag = line.substr(0, 1);
         std::string patternName;
         std::size_t commaPos;
         if (nameTag == ";" ||
             nameTag == " " ||
             line.empty()) {           //is cr/lf empty?
             continue;
         } else if (nameTag == "*") {
             commaPos = line.find(',',1);
             if (commaPos != std::string::npos) {
                  patternName = line.substr(1, commaPos-1);
             } else {
                  patternName = line.substr(1);
             }
             if (patternName == parmName) {
                 //this is our pattern
                 return true;
             }
        }
    }  //endwhile
    return false;
}

//get the definition lines for this pattern
std::vector<std::string> PATLineSpec::loadPatternDef(std::ifstream& inFile)
{
    std::vector<std::string> result;
    while ( inFile.good() ){
        std::string line;
        std::getline(inFile, line);
        std::string nameTag = line.substr(0, 1);
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
    Base::FileInfo fi(parmFile);
    Base::ifstream inFile;
    inFile.open (fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().Message( "Cannot open input file.\n");
        return result;
    }

    while ( inFile.good() ){
        std::string line;
        std::getline(inFile, line);
        std::string nameTag = line.substr(0, 1);               //dupl code here
        std::size_t commaPos;
        if (nameTag == "*") {  //found a pattern
            commaPos = line.find(',',1);
            std::string patternName;
            if (commaPos != std::string::npos) {
                 patternName = line.substr(1, commaPos-1);
            } else {
                  patternName = line.substr(1);
            }
            result.push_back(patternName);
        }
    }
    return result;
}

double PATLineSpec::getSlope()
{
    double angle = getAngle();

    //only dealing with angles -180:180 for now
    if (angle > 90.0) {
        angle = -(180.0 - angle);
    } else if (angle < -90.0) {
        angle = (180 + angle);
    }
    return tan(angle * M_PI/180.0);
}

bool PATLineSpec::isDashed()
{
    return !m_dashParms.empty();
}

//! X component of distance between lines
double PATLineSpec::getIntervalX()
{
    if (getAngle() == 0.0) {
        return 0.0;
    } else if ((getAngle() == 90.0) || (getAngle() == -90.0)) {
        return getInterval();
    } else {
        double perpAngle = fabs(getAngle() - 90.0);
        return fabs(getInterval() / cos(perpAngle * M_PI/180.0));
    }
}

//! Y component of distance between lines
double PATLineSpec::getIntervalY()
{
    if (getAngle() == 0.0) {
        return getInterval();
    } else if ((getAngle() == 90.0) || (getAngle() == -90.0)) {
        return 0.0;
    } else {
        double perpAngle = fabs(getAngle() - 90.0);
        return fabs(getInterval() * tan(perpAngle * M_PI/180.0));
    }
}


//********************************************************

double DashSpec::length()
{
    double result = 0.0;
    for (auto& c: get()) {
        result += fabs(c);
    }
    return result;
}

DashSpec DashSpec::reversed()
{
    std::vector<double> p = get();
    std::reverse(p.begin(), p.end());
    return DashSpec(p);
}

void DashSpec::dump(const char* title)
{
    std::stringstream ss;
    ss << title << ": " ;
    for (auto& p: m_parms) {
        ss << p << ", ";
    }
    Base::Console().Message("DUMP - DashSpec - %s\n", ss.str().c_str());
}





