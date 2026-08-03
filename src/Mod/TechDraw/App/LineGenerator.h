/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

//! a class for handling standard ISO128 and ANSI Y14.2 line types and their dash
//! patterns
//! ISO standard lines are defined by a sequence of graphical elements as in
//! the dotted line (line type 7): DOT, GAP
//! each graphical element (DOT, GAP, DASH, etc) has a standard length in units
//! of the line's width.
//! the graphical elements and line definitions are stored in csv files.
//! ANSI lines standards are not numbered, but we assign a number as a convenient
//! reference.
//! ANSI standard lines are defined similarly to ISO, but the element lengths
//! are defined in mm, and do not vary with pen width.
//! ASME standard lines do not specify the element lengths at all, so we have
//! chosen values generally equal to those for ISO128


#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <map>
#include <QPen>

namespace TechDraw {

class TechDrawExport LineGenerator {
public:
    LineGenerator();
    ~LineGenerator() = default;

    enum LINESTANDARD
    {
        ANSI,
        ISO,
        ASME
    };

    QPen getBestPen(size_t lineNumber, Qt::PenStyle qtStyle, double width);
    QPen getLinePen(size_t lineNumber, double nominalLineWidth);

    static int fromQtStyle(Qt::PenStyle style);
    static std::vector<std::string> getAvailableLineStandards();
    static std::string getLineStandardsBody();

    //! if the line standard changes during a lineGenerator's life time
    //! then the elements and line descriptions need to be reloaded using the
    //! new standard.
    void reloadDescriptions();
    //! get line descriptions from memory
    std::vector<std::string> getLoadedDescriptions();
    //! get line descriptions from file
    static std::vector<std::string> getLineDescriptions();

    static bool isProportional(size_t standardIndex);
    bool isCurrentProportional();
    static std::string getBodyFromString(std::string inString);

private:
    static std::map<std::string, int> loadElements();
    static std::vector< std::vector<std::string> > getLineDefinitions();

    std::map<std::string, int> m_elementDefs;
    std::vector< std::vector<std::string> > m_lineDefs;
    std::vector< std::string > m_lineDescs;
};

}