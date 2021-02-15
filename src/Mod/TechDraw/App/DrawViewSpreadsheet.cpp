/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <sstream>
#endif

#include <iomanip>

#include <boost/regex.hpp>

#include <App/Application.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "Preferences.h"
#include "DrawViewSpreadsheet.h"

#include <Mod/Spreadsheet/App/Cell.h>
#include <Mod/Spreadsheet/App/Sheet.h>

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewSpreadsheet
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSpreadsheet, TechDraw::DrawViewSymbol)

DrawViewSpreadsheet::DrawViewSpreadsheet(void)
{
    static const char *vgroup = "Spreadsheet";

    ADD_PROPERTY_TYPE(Source ,(0),vgroup,App::Prop_None,"Spreadsheet to view");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(CellStart ,("A1"),vgroup,App::Prop_None,"The top left cell of the range to display");
    ADD_PROPERTY_TYPE(CellEnd ,("B2"),vgroup,App::Prop_None,"The bottom right cell of the range to display");
    ADD_PROPERTY_TYPE(Font ,(Preferences::labelFont().c_str()),
                                                         vgroup,App::Prop_None,"The name of the font to use");
    ADD_PROPERTY_TYPE(TextColor,(0.0f,0.0f,0.0f),vgroup,App::Prop_None,"The default color of the text and lines");
    ADD_PROPERTY_TYPE(TextSize,(12.0),vgroup,App::Prop_None,"The size of the text");
    ADD_PROPERTY_TYPE(LineWidth,(0.35),vgroup,App::Prop_None,"The thickness of the cell lines");

    EditableTexts.setStatus(App::Property::Hidden,true);

}

DrawViewSpreadsheet::~DrawViewSpreadsheet()
{
}

short DrawViewSpreadsheet::mustExecute() const
{
    short result = 0;
    if (!isRestoring()) {
        result  =  (Source.isTouched()  ||
                    CellStart.isTouched() ||
                    CellEnd.isTouched() ||
                    Font.isTouched() ||
                    TextSize.isTouched() ||
                    TextColor.isTouched() ||
                    LineWidth.isTouched() );
    }
    if (result) {
        return result;
    }
    return TechDraw::DrawView::mustExecute();
}

void DrawViewSpreadsheet::onChanged(const App::Property* prop)
{
    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSpreadsheet::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    std::string scellstart = CellStart.getValue();
    std::string scellend = CellEnd.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No spreadsheet linked");
    if (!link->getTypeId().isDerivedFrom(Spreadsheet::Sheet::getClassTypeId()))
        return new App::DocumentObjectExecReturn("The linked object is not a spreadsheet");
    if ( (scellstart.empty()) || (scellend.empty()) )
        return new App::DocumentObjectExecReturn("Empty cell value");

    Symbol.setValue(getSheetImage());

    return TechDraw::DrawView::execute();
}

std::vector<std::string> DrawViewSpreadsheet::getAvailColumns(void)
{
    // build a list of available columns: A, B, C, ... AA, AB, ... ZY, ZZ.
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::vector<std::string> availcolumns;
    for (int i=0; i<26; ++i) {              //A:Z
        std::stringstream s;
        s << alphabet[i];
        availcolumns.push_back(s.str());
    }
    for (int i=0; i<26; ++i) {             //AA:ZZ
        for (int j=0; j<26; ++j) {
            std::stringstream s;
            s << alphabet[i] << alphabet[j];
            availcolumns.push_back(s.str());
        }
    }
    return availcolumns;
}

std::string DrawViewSpreadsheet::getSVGHead(void)
{
    std::string head = std::string("<svg\n") +
                       std::string("	xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"\n") +
                       std::string("	xmlns:freecad=\"http://www.freecadweb.org/wiki/index.php?title=Svg_Namespace\">\n");
    return head;
}

std::string DrawViewSpreadsheet::getSVGTail(void)
{
    std::string tail = "\n</svg>";
    return tail;
}

std::string DrawViewSpreadsheet::getSheetImage(void)
{
    std::stringstream result;

    App::DocumentObject* link = Source.getValue();
    link->recomputeFeature();   //make sure s/s is up to date

    std::string scellstart = CellStart.getValue();
    std::string scellend = CellEnd.getValue();

    //s/s columns are A,B,C, ... ZX,ZY,ZZ 
    //lower case characters are not valid
    transform(scellstart.begin(), scellstart.end(), scellstart.begin(), ::toupper); 
    transform(scellend.begin(), scellend.end(), scellend.begin(), ::toupper); 

    std::string colPart;
    std::string rowPart;
    boost::regex re{"([A-Z]*)([0-9]*)"};
    boost::smatch what;
    int iRowStart = 0, iRowEnd = 0;
    std::string sColStart, sColEnd;
    if (boost::regex_search(scellstart, what, re)) {
        if (what.size() < 3) {
            Base::Console().Error("%s - start cell (%s) is invalid\n",getNameInDocument(),CellStart.getValue());
            return result.str();
        } else {
            colPart = what[1];
            sColStart = colPart;
            rowPart = what[2];
            try {
                iRowStart = std::stoi(rowPart);
            }
            catch (...) {
                Base::Console().Error("%s - start cell (%s) invalid row\n",
                                      getNameInDocument(), rowPart.c_str());
                return result.str();
            }
        }
    }

    if (boost::regex_search(scellend, what, re)) {
        if (what.size() < 3) {
            Base::Console().Error("%s - end cell (%s) is invalid\n",getNameInDocument(),CellEnd.getValue());
        } else {
            colPart = what[1];
            sColEnd = colPart;
            rowPart = what[2];
            try {
                iRowEnd = std::stoi(rowPart);
            }
            catch (...) {
                Base::Console().Error("%s - end cell (%s) invalid row\n",
                                      getNameInDocument(), rowPart.c_str());
                return result.str();
            }
        }
    }

    std::vector<std::string> availcolumns = getAvailColumns();

    //validate range start column in sheet's available columns
    int iAvailColStart = colInList(availcolumns, sColStart);
    if (iAvailColStart < 0) {               //not found range start column in availcolumns list
        Base::Console().Error("DVS - %s - start Column (%s) is invalid\n",
                               getNameInDocument(), sColStart.c_str());
        return result.str();
    }

    //validate range end column in sheet's available columns
    int iAvailColEnd = colInList(availcolumns,sColEnd);
    if (iAvailColEnd < 0) {
        Base::Console().Error("DVS - %s - end Column (%s) is invalid\n",
                              getNameInDocument(), sColEnd.c_str());
        return result.str();
    }

    //check for logical range
    if ( (iAvailColStart > iAvailColEnd) ||
         (iRowStart > iRowEnd) ) {
        Base::Console().Error("%s - cell range is illogical\n",getNameInDocument());
        return result.str();
    }

    // build row and column ranges
    std::vector<std::string> validColNames;
    std::vector<int> validRowNumbers;

    int iCol = iAvailColStart;
    for (; iCol <= iAvailColEnd; iCol++) {
        validColNames.push_back(availcolumns.at(iCol));
    }
    
    int iRow = iRowStart;
    for ( ; iRow <= iRowEnd ; iRow++) {
        validRowNumbers.push_back(iRow);
    }

    // create the Svg code
    std::string ViewName = Label.getValue();

    result << getSVGHead();

    App::Color c = TextColor.getValue();
    result  << "<g id=\"" << ViewName << "\">" << endl;

    // fill the cells
    float rowoffset = 0.0;
    float coloffset = 0.0;
    float cellheight = 100;
    float cellwidth = 100;
    std::string celltext;
    Spreadsheet::Sheet* sheet = static_cast<Spreadsheet::Sheet*>(link);
    std::vector<std::string> skiplist;

    for (std::vector<std::string>::const_iterator col = validColNames.begin(); col != validColNames.end(); ++col) {
        // create a group for each column
        result << "  <g id=\"" << ViewName << "_col" << (*col) << "\">" << endl;
        for (std::vector<int>::const_iterator row = validRowNumbers.begin(); row != validRowNumbers.end(); ++row) {
            // get cell size
            std::stringstream srow;
            srow << (*row);
            App::CellAddress address((*col) + srow.str());
            cellwidth = sheet->getColumnWidth(address.col());
            cellheight = sheet->getRowHeight(address.row());
            celltext = "";
            Spreadsheet::Cell* cell = sheet->getCell(address);
            // get the text
            App::Property* prop = sheet->getPropertyByName(address.toString().c_str());
            std::stringstream field;
            if (prop && cell) {
                if (prop->isDerivedFrom((App::PropertyQuantity::getClassTypeId()))) {
                    field << cell->getFormattedQuantity();
                } else if (prop->isDerivedFrom((App::PropertyFloat::getClassTypeId()))) {
                    field << cell->getFormattedQuantity();
                } else if (prop->isDerivedFrom((App::PropertyInteger::getClassTypeId()))) {
                    field << cell->getFormattedQuantity();
                } else if (prop->isDerivedFrom((App::PropertyString::getClassTypeId()))) {
                    field << static_cast<App::PropertyString*>(prop)->getValue();
                } else {
                    Base::Console().Error("DVSS: Unknown property type\n");
                    celltext = "???";
//                    assert(0);
                }
                celltext = field.str();
            }
            // get colors, style, alignment and span
            int alignment = 0;
            std::string bcolor = "none";
            std::string fcolor = c.asCSSString();
            std::string textstyle = "";
            if (cell) {
                App::Color f,b;
                std::set<std::string> st;
                int colspan, rowspan;
                if (cell->getBackground(b)) {
                    bcolor = b.asCSSString();
                }
                if (cell->getForeground(f)) {
                    fcolor = f.asCSSString();
                }
                if (cell->getStyle(st)) {
                    for (std::set<std::string>::const_iterator i = st.begin(); i != st.end(); ++i) {
                         if ((*i) == "bold")
                            textstyle = textstyle + "font-weight: bold; ";
                        else if ((*i) == "italic")
                            textstyle = textstyle + "font-style: italic; ";
                        else if ((*i) == "underline")
                            textstyle = textstyle + "text-decoration: underline; ";
                    }
                }
                if (cell->getSpans(rowspan,colspan)) {
                    for (int i=0; i<colspan; ++i) {
                        for (int j=0; j<rowspan; ++j) {
                            App::CellAddress nextcell(address.row()+j,address.col()+i);
                            if (i > 0)
                                cellwidth = cellwidth + sheet->getColumnWidth(nextcell.col());
                            if (j > 0)
                                cellheight = cellheight + sheet->getRowHeight(nextcell.row());
                            if ( (i > 0) || (j > 0) )
                                skiplist.push_back(nextcell.toString());
                        }
                    }
                }
                cell->getAlignment(alignment);
            }
            // skip cell if found in skiplist
            if (std::find(skiplist.begin(), skiplist.end(), address.toString()) == skiplist.end()) {
                result << "    <rect x=\"" << coloffset << "\" y=\"" << rowoffset << "\" width=\"" << cellwidth
                       << "\" height=\"" << cellheight << "\" style=\"fill:" << bcolor << ";stroke-width:"
                       << LineWidth.getValue()/getScale() << ";stroke:" << c.asCSSString() << ";\" />" << endl;
                if (alignment & Spreadsheet::Cell::ALIGNMENT_LEFT)
                    result << "    <text style=\"" << textstyle << "\" x=\"" << coloffset + TextSize.getValue()/2 << "\" y=\"" << rowoffset + 0.75 * cellheight << "\" font-family=\"" ;
                if (alignment & Spreadsheet::Cell::ALIGNMENT_HCENTER)
                    result << "    <text text-anchor=\"middle\" style=\"" << textstyle << "\" x=\"" << coloffset + cellwidth/2 << "\" y=\"" << rowoffset + 0.75 * cellheight << "\" font-family=\"" ;
                if (alignment & Spreadsheet::Cell::ALIGNMENT_RIGHT)
                    result << "    <text text-anchor=\"end\" style=\"" << textstyle << "\" x=\"" << coloffset + (cellwidth - TextSize.getValue()/2) << "\" y=\"" << rowoffset + 0.75 * cellheight << "\" font-family=\"" ;
                if ((alignment & Spreadsheet::Cell::ALIGNMENT_LEFT) ||
                    (alignment & Spreadsheet::Cell::ALIGNMENT_HCENTER) ||
                    (alignment & Spreadsheet::Cell::ALIGNMENT_RIGHT)) {
                    result << Font.getValue() << "\"" << " font-size=\"" << TextSize.getValue() << "\""
                           << " fill=\"" << fcolor << "\">" << celltext << "</text>" << endl;
                }
            }
            rowoffset = rowoffset + cellheight;
        }
        result << "  </g>" << endl;
        rowoffset = 0.0;
        coloffset = coloffset + cellwidth;
    }

    // close the containing group
    result << "</g>" << endl;

    result << getSVGTail();

    return result.str();

}

//find index of column name "toFind" in "list" of column names
int DrawViewSpreadsheet::colInList(const std::vector<std::string>& list,
                                   const std::string& toFind)
{
    int result = -1;
    auto match = std::find(std::begin(list), std::end(list), toFind);
    if (match != std::end(list)) {
        result = match - std::begin(list);
    }
    return result;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSpreadsheetPython, TechDraw::DrawViewSpreadsheet)
template<> const char* TechDraw::DrawViewSpreadsheetPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderSpreadsheet";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSpreadsheet>;
}
