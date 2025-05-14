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

//! a class for handling standard ISO128, ANSI Y14.2 line types and their dash
//! patterns.  Additional standards can be added.
//! ISO standard lines are defined by a sequence of graphical elements as in
//! the dotted line (line type 7): DOT, GAP
//! each graphical element (DOT, GAP, DASH, etc) has a standard length in units
//! of the line's width.
//! the graphical elements and line definitions are stored in csv files.
//! ANSI lines standards are not numbered, but we assign a number as a convenient
//! reference.
//! ANSI standard lines are defined similarly to ISO, but the element lengths
//! are defined in mm, and do not vary with pen width.

//! the graphical elements and line definitions are stored in csv files.
//! these values only change if ISO128.20 or ANSI Y14.2 change

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <QVector>

#include <Base/Console.h>
#include <Base/Stream.h>

#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/LineNameEnum.h>

#include "Preferences.h"
#include "LineGenerator.h"

using namespace TechDraw;
using DU = DrawUtil;

LineGenerator::LineGenerator()
{
    reloadDescriptions();
}

void LineGenerator::reloadDescriptions()
{
    m_elementDefs = loadElements();
    m_lineDefs = getLineDefinitions();
    m_lineDescs = getLineDescriptions();
}

//! figure out an appropriate QPen from an iso line number and a Qt PenStyle
//! we prefer to use the ISO Line Number if available.
QPen LineGenerator::getBestPen(size_t isoNumber, Qt::PenStyle qtStyle, double width)
{
    // TODO: use TechDraw::LineFormat::InvalidLine here
    if (isoNumber > 0 &&
        isoNumber < m_lineDefs.size()) {
        // we have a valid line number, so use it
        return getLinePen(isoNumber, width);
    }
    int qtline = fromQtStyle(qtStyle);
    if (qtline > 0) {
        // we have a reasonable approximation of a Qt Style
        return getLinePen(qtline, width);
    }
    // no valid line and the qtStyle doesn't convert to a line numb45r
    // so we'll just make it continuous.
    return getLinePen(1, width);
}

//! create a QPen for a given line number and line width.  ISO lines are numbered
//! 1-15 and ANSI lines are 1-4(?)  The line width is the nominal width in mm.
QPen LineGenerator::getLinePen(size_t lineNumber, double nominalLineWidth)
{
//    Base::Console().message("LG::getLinePen(%d, %.3f)\n",
//                             lineNumber, nominalLineWidth);
    QPen linePen;
    linePen.setWidthF(nominalLineWidth);

    // Note: if the cap style is Round or Square, the lengths of the lines, or
    // dots/dashes within the line, will be wrong by 1 pen width.  To get the
    // exact line lengths or dash pattern, you must use Flat caps.  Flat caps
    // look terrible at the corners.
    linePen.setCapStyle((Qt::PenCapStyle)Preferences::LineCapStyle());

    double proportionalAdjust{1.0};
    if (!isCurrentProportional()) {
        // ANSI.Y14.2M.1992 lines are specified in actual element lengths, but Qt will draw
        // them as proportional to the line width.
        proportionalAdjust = nominalLineWidth;
    }

    // valid line numbers are [1, number of line definitions]
    // line 1 is always (?) continuous
    // 0 substitutes for LineFormat::InvalidLine here
    if (lineNumber < 2 ||
        lineNumber > m_lineDefs.size()) {
        // plain boring solid line (or possibly an invalid line number)
        linePen.setStyle(Qt::SolidLine);
        return linePen;
    }

    int lineIndex = lineNumber - 1;
    std::vector<std::string> elements = m_lineDefs.at(lineIndex);

    // there are some lines with numbers >1 that are also continuous, and
    // a dash pattern is not applicable.
    std::string naToken{"n/a"};
    if (elements.empty() || elements.front() == naToken) {
        // plain boring solid line (or possibly an invalid line number)
        linePen.setStyle(Qt::SolidLine);
        return linePen;
    }

    // there is at least one line style (ASME #11 Other) that is "invisible"
    std::string noLineToken{"noline"};
    if (elements.front() == noLineToken) {
        linePen.setStyle(Qt::NoPen);
        return linePen;
    }

    // interesting line styles
    linePen.setStyle(Qt::CustomDashLine);
    std::vector<double> dashPattern;
    bool firstElement(true);
    for (auto& entry : elements) {
        if (firstElement &&
            (entry == "Gap" || entry == "Space") ) {
            // some dash patterns MAY begin with a gap/space, but Qt dash patterns are always
            // "mark, space, mark, space", so we handle this by offsetting the pattern
            // and skipping the first element.
            linePen.setDashOffset(static_cast< double >(m_elementDefs[entry]) / proportionalAdjust);
            firstElement = false;
            continue;
        }
        firstElement = false;
        dashPattern.push_back(static_cast< double >(m_elementDefs[entry]) / proportionalAdjust);
    }

    QVector<double> qDashPattern(dashPattern.begin(), dashPattern.end());

    linePen.setDashPattern(qDashPattern);
    linePen.setWidthF(nominalLineWidth);
    return linePen;
}


//! convert Qt line style to closest ISO line number
int LineGenerator::fromQtStyle(Qt::PenStyle style)
{
    // the 4 standard Qt::PenStyles and ISO128 equivalents
    int dashed = 2;
    int dotted = 7;
    int dashDot = 10;
    int dashDotDot = 12;
    if (Preferences::lineStandard() == ANSI) {
        dashed = 2;
        dotted = 2;  // no dotted line in Ansi Y14.2?
        dashDot = 2;
        dashDotDot = 2;
    }
    if (Preferences::lineStandard() == ASME) {
        dashed = 2;
        dotted = 16;
        dashDot = 17;
        dashDotDot = 14;
    }

    switch (style) {
        case Qt::NoPen:
        case Qt::SolidLine:
            return 1;
        case Qt::DashLine:
            return dashed;
        case Qt::DotLine:
            return dotted;
        case Qt::DashDotLine:
            return dashDot;
        case Qt::DashDotDotLine:
            return dashDotDot;
        case Qt::CustomDashLine:
            // not sure what to do here.  we would have to match the custom pattern
            // to the patterns of the ISO lines and set the dash pattern accordingly.
            return 2;
        default:
            return 0;
    }
}


//! return the standard lengths for the various graphical elements described in ISO128,
//! ANSI Y14.2 standards file
std::map<std::string, int> LineGenerator::loadElements()
{
    std::map<std::string, int> result;
    // open file, for each record, parse element name and length, then insert into
    // the output map.

    std::string parmFile = Preferences::currentElementDefFile();
    std::string record;
    Base::FileInfo fi(parmFile);
    Base::ifstream inFile(fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().message( "Cannot open line element def file: %s\n", parmFile.c_str());
        return result;
    }
    std::string line;
    while ( std::getline(inFile, line) ){
        if ( line.empty() || line.at(0) == '#' ) {
            // this is a comment or a blank line, ignore it
            continue;
        }
        std::vector<std::string> tokens = DU::tokenize(line, ",");
        // should be 2 tokens: elementName, elementLength
        result[tokens.front()] = std::stoi(tokens.back(), nullptr);
    }
    inFile.close();
    return result;
}


//! load the line definition file into memory
std::vector< std::vector<std::string> > LineGenerator::getLineDefinitions()
{
    std::vector< std::vector<std::string> > lineDefs;
    std::string record;
    Base::FileInfo fi(Preferences::currentLineDefFile());
    Base::ifstream inFile(fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().message( "Cannot open line def file: %s\n", fi.filePath().c_str());
        return lineDefs;
    }

    std::string line;
    while ( std::getline(inFile, line) ) {
        if (line.empty() ||
            line.at(0) == '#') {
            // this is a comment or a blank line, ignore it
            continue;
        }
        // strip out any null tokens that may be caused by trailing ',' in the input
        std::vector<std::string> validTokens;
        for (auto& token : DU::tokenize(line, ",")) {
            if (!token.empty()) {
                validTokens.emplace_back(token);
            }
        }
        std::vector<std::string> lineDefRow;
        lineDefRow.insert(lineDefRow.end(), validTokens.begin()+2, validTokens.end());
        lineDefs.push_back(lineDefRow);
    }

    inFile.close();
    return lineDefs;
}

//! retrieve a sorted list of available line definition files.
std::vector<std::string> LineGenerator::getAvailableLineStandards()
{
    std::vector<std::string> result;
    std::string lineDefToken{"LineDef"};
    Base::FileInfo fi(Preferences::lineDefinitionLocation());
    auto fiAll = fi.getDirectoryContent();
    for (auto& entry : fiAll) {
        if (!entry.isFile()) {
            continue;
        }
        auto fileName = entry.fileNamePure();
        size_t position = fileName.find(lineDefToken);
        if (position != std::string::npos) {
            result.emplace_back(fileName.substr(0, position - 1));
        }
    }
    std::sort(result.begin(),result.end());
    return result;
}


//! get the descriptions of the lines already loaded into this LineGenerator object
std::vector<std::string> LineGenerator::getLoadedDescriptions()
{
    return m_lineDescs;
}


//! extract the line description fields from the current definition file
std::vector<std::string> LineGenerator::getLineDescriptions()
{
    std::vector<std::string> lineDescs;
    std::string record;
    Base::FileInfo fi(Preferences::currentLineDefFile());
    Base::ifstream inFile(fi, std::ifstream::in);
    if(!inFile.is_open()) {
        Base::Console().message( "Cannot open line def file: %s\n", fi.filePath().c_str());
        return lineDescs;
    }

    std::string line;
    while ( std::getline(inFile, line) ) {
        if (line.empty() ||
            line.at(0) == '#') {
            // this is a comment or a blank line, ignore it
            continue;
        }
        // strip out any null tokens that may be caused by trailing ',' in the input
        std::vector<std::string> validTokens;
        for (auto& token : DU::tokenize(line, ",")) {
            if (!token.empty()) {
                validTokens.emplace_back(token);
            }
        }
        lineDescs.push_back(validTokens.at(1));
    }

    inFile.close();
    return lineDescs;
}


//! returns a string identifying the standards body which issued the active line
//! standard
std::string  LineGenerator::getLineStandardsBody()
{
    int activeStandard = Preferences::lineStandard();
    std::vector<std::string> choices = getAvailableLineStandards();
    if (activeStandard < 0 ||
        (size_t) activeStandard >= choices.size()) {
        // there is a condition where the LineStandard parameter exists, but is -1 (the
        // qt value for no current index in a combobox).  This is likely caused by an old
        // development version writing an unvalidated value.  In this case, the existing but
        // invalid value will be returned.  This is a temporary fix and can be removed for
        // production.
        // Preferences::lineStandard() will print a message about this every time it is called
        // (lots of messages!).
        activeStandard = 0;
        }
    return getBodyFromString(choices.at(activeStandard));
}


//! returns true if line elements of the current standard are proportional
//! to line width (as in ISO), or false if the elements have a constant length
//! (as in ANSI)
bool LineGenerator::isCurrentProportional()
{
    return isProportional(Preferences::lineStandard());
}


//! returns true if line elements of the specified standard are proportional
//! to line width (as in ISO), or false if the elements have a constant length
//! (as in ANSI)
bool LineGenerator::isProportional(size_t standardIndex)
{
    std::vector<std::string> choices = getAvailableLineStandards();
    if (standardIndex > choices.size()) {
        // we don't have a standard for the specified index.
        return true;
    }
    std::string bodyName = getBodyFromString(choices.at(standardIndex));
    if (bodyName == "ANSI") {
        return false;
    }
    return true;
}


//! returns the standards body name from a standard name in the form
//! body.standard.section.revision
std::string LineGenerator::getBodyFromString(std::string inString)
{
    size_t firstDot = inString.find(".");
    if (firstDot == std::string::npos) {
        // something has gone very wrong if an entry in choices does not contain a dot.
        throw Base::RuntimeError("Malformed standard name found.  Could not determine standards body.");
    }
    return inString.substr(0, firstDot);
}
