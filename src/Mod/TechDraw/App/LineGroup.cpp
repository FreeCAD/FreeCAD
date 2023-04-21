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
# include <fstream>
# include <iomanip>
# include <sstream>
# include <stdexcept>
#endif

#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "LineGroup.h"
#include "Preferences.h"


using namespace TechDraw;

LineGroup::LineGroup()
{
    init();
}

LineGroup::LineGroup(std::string groupName)
{
    init();
    m_name = groupName;
}


LineGroup::~LineGroup()
{
}

void LineGroup::init()
{
    m_name    = "Default";
    m_thin    = 0.35;
    m_graphic = 0.50;
    m_thick   = 0.70;
    m_extra   = 1.40;
}

double LineGroup::getWeight(std::string s)
{
    if (s == "Thin") {
       return m_thin;
    } else if (s == "Graphic") {
       return m_graphic;
    } else if (s == "Thick") {
       return m_thick;
    } else if (s == "Extra") {
       return m_extra;
    }
    return 0.55;
}

void LineGroup::setWeight(std::string s, double weight)
{
    if (s == "Thin") {
       m_thin = weight;
    } else if (s == "Graphic") {
       m_graphic = weight;
    } else if (s == "Thick") {
       m_thick = weight;
    } else if (s == "Extra") {
       m_extra = weight;
    }
}

void LineGroup::dump(const char* title)
{
    Base::Console().Message( "DUMP: %s\n", title);
    Base::Console().Message( "Name: %s\n", m_name.c_str());
    Base::Console().Message( "Thin: %.3f\n", m_thin);
    Base::Console().Message( "Graphic: %.3f\n", m_graphic);
    Base::Console().Message( "Thick: %.3f\n", m_thick);
    Base::Console().Message( "Extra: %.3f\n", m_extra);
}

//static support function: split comma separated string of values into vector of numbers
std::vector<double> LineGroup::split(std::string line)
{
    std::vector<double>   result;
    std::stringstream     lineStream(line);
    std::string           cell;
    bool nameCell = true;

    while(std::getline(lineStream, cell, ','))
    {
        if (nameCell) {
            nameCell = false;
            continue;
        }
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

//static support function: find group definition in file
std::string LineGroup::getRecordFromFile(std::string parmFile, int groupNumber)
{
    std::string record;
    Base::FileInfo fi(parmFile);
    Base::ifstream inFile(fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().Message( "Cannot open LineGroup file: %s\n", parmFile.c_str());
        return record;
    }
    // parse file to get the groupNumber'th line
    int counter = 0; // the combobox enums begin with 0
    while ( inFile.good() ){
         std::string line;
         std::getline(inFile, line);
         std::string nameTag = line.substr(0, 1);
         if (nameTag == "*") { // we found a definition line
             if (counter == groupNumber) {
                 record = line;
                 return record;
             }
             ++counter;
        }
    }  //endwhile
    // nothing was found
    Base::Console().Error("LineGroup: the LineGroup file has only %s entries but entry number %s is set\n"
        , std::to_string(counter).c_str()
        , std::to_string(groupNumber).c_str());
    return std::string(); // return an empty string
}

//static LineGroup maker
LineGroup* LineGroup::lineGroupFactory(int groupNumber)
{
    LineGroup* lg = new LineGroup();

    std::string lgFileName = Preferences::lineGroupFile();

    std::string lgRecord = LineGroup::getRecordFromFile(lgFileName, groupNumber);

    std::vector<double> values = LineGroup::split(lgRecord);
    if (values.size() < 4) {
        Base::Console().Error( "LineGroup::invalid entry in %s\n", lgFileName.c_str() );
    } else {
        lg->setWeight("Thin", values[0]);
        lg->setWeight("Graphic", values[1]);
        lg->setWeight("Thick", values[2]);
        lg->setWeight("Extra", values[3]);
    }
    return lg;
}

/**
 * @brief Returns the default line width given a weight name and group number.
 *
 * @param weightName can be "Thick", "Thin", "Graphic" or "Extra"
 * @param lineGroupNumber if -1 will then use groupNumber in preferences. Default value is -1.
 * @return the default line width.
 */
double LineGroup::getDefaultWidth(std::string weightName, int lineGroupNumber)
{
    if (lineGroupNumber == -1) {
        lineGroupNumber = Preferences::lineGroup();
    }
    auto lg = TechDraw::LineGroup::lineGroupFactory(lineGroupNumber);

    double weight = lg->getWeight(weightName);
    delete lg;
    return weight;
}

//static support function: find group definition in file
std::string LineGroup::getGroupNamesFromFile(std::string FileName)
{
    std::string record;
    Base::FileInfo fi(FileName);
    Base::ifstream inFile(fi, std::ifstream::in);
    if (!inFile.is_open()) {
        Base::Console().Message("Cannot open LineGroup file: %s\n", FileName.c_str());
        return record;
    }
    // parse the file
    while (inFile.good()) {
        std::string line;
        std::getline(inFile, line);
        std::string nameTag = line.substr(0, 1);
        std::string found;
        std::size_t commaPos;
        if (nameTag == "*") {
            commaPos = line.find(',', 1);
            if (commaPos != std::string::npos) {
                found = line.substr(1, commaPos - 1);
                record = record + found + ',';
            }
        }
    }  //endwhile
    if (record.empty()) {
        Base::Console().Message("LineGroup error: no group found in file %s\n", FileName.c_str());
    }
    return record;
}
