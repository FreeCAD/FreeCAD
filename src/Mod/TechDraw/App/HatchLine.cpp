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

#include <TopoDS_Edge.hxx>

#include <Base/Console.h>
#include <Base/Vector3D.h>

#include "Geometry.h"

#include "DrawUtil.h"
#include "HatchLine.h"

using namespace TechDraw;

HatchLine::HatchLine()
{
    init();
}

HatchLine::HatchLine(std::string& lineSpec)
{
    init();
    load(lineSpec);
}


HatchLine::~HatchLine()
{
}

void HatchLine::init(void)
{
    m_angle = 0.0;
    m_origin = Base::Vector3d(0.0,0.0,0.0);
    m_interval = 1.0;
    m_offset = 0.0;
}

void HatchLine::load(std::string& lineSpec)
{
    std::vector<double> values = split(lineSpec);
    if (values.size() < 5) {
        Base::Console().Message( "HatchLine::load(%s) invalid entry in pattern\n",lineSpec.c_str() );
        return;
    }
    m_angle    = values[0];
    m_origin = Base::Vector3d(values[1],values[2],0.0);
    m_offset   = values[3];
    m_interval = values[4];
    if (values.size() > 5) {
        m_dashParms.insert(std::end(m_dashParms), std::begin(values) + 5, std::end(values));
    }
}

std::vector<double> HatchLine::split(std::string line)
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

void HatchLine::dump(char* title)
{
    Base::Console().Message( "DUMP: %s\n",title);
    Base::Console().Message( "Angle: %.3f\n", m_angle);
    Base::Console().Message( "Origin: %s\n",DrawUtil::formatVector(m_origin).c_str());
    Base::Console().Message( "Offset: %.3f\n",m_offset);
    Base::Console().Message( "Interval: %.3f\n",m_interval);
    std::stringstream ss;
    for (auto& d: m_dashParms) {
        ss << d << ", ";
    }
    ss << "end";
    Base::Console().Message( "DashSpec: %s\n",ss.str().c_str());
}

//static class methods
std::vector<HatchLine> HatchLine::getSpecsForPattern(std::string& parmFile, std::string& parmName)
{
    std::vector<HatchLine> result;
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
    
    //decode definition lines into HatchLine objects
    for (auto& l: lineSpecs) {
        HatchLine hl(l);
        result.push_back(hl);
    }
    return result;
}

bool  HatchLine::findPatternStart(std::ifstream& inFile, std::string& parmName)
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
std::vector<std::string> HatchLine::loadPatternDef(std::ifstream& inFile)
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

std::vector<std::string> HatchLine::getPatternList(std::string& parmFile)
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
 




