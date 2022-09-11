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

//! LineGroup - Classes related to processing LineGroup definition CSV files

#ifndef TechDraw_LINEGROUP_H_
#define TechDraw_LINEGROUP_H_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <string>

namespace TechDraw
{

class TechDrawExport LineGroup
{
public:
    LineGroup();
    LineGroup(std::string groupName);
    ~LineGroup();
    double getWeight(std::string s);
    void setWeight(std::string s, double weight);
//    void setWeight(const char* s, double weight);
    void dump(const char* title);
    std::string getName(void) { return m_name; }
    void setName(std::string s) { m_name = s; }

    //static support function: split comma separated string of values into vector of numbers
    static std::vector<double> split(std::string line);

    //static support function: find group defn in file
    static std::string getRecordFromFile(std::string parmFile, int groupNumber);

    //static LineGroup maker
    static LineGroup* lineGroupFactory(int groupNumber);

    static double getDefaultWidth(std::string weightName, int groupNumber = -1);

    static std::string getGroupNamesFromFile(std::string FileName);


protected:
    void init(void);

    std::string m_name;
    double      m_thin;
    double      m_graphic;
    double      m_thick;
    double      m_extra;
};

}
#endif
