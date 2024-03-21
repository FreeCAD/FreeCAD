/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2015     *
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
#include <iomanip>
#include <sstream>
#endif

#include <App/PropertyUnits.h>
#include <Base/Exception.h>
#include <Mod/Spreadsheet/App/Cell.h>
#include <Mod/Spreadsheet/App/Sheet.h>

#include "FeatureViewSpreadsheet.h"


using namespace Drawing;

//===========================================================================
// FeatureViewSpreadsheet
//===========================================================================

PROPERTY_SOURCE(Drawing::FeatureViewSpreadsheet, Drawing::FeatureView)


FeatureViewSpreadsheet::FeatureViewSpreadsheet(void)
{
    static const char* vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(CellStart,
                      ("A1"),
                      vgroup,
                      App::Prop_None,
                      "The top left cell of the range to display");
    ADD_PROPERTY_TYPE(CellEnd,
                      ("B2"),
                      vgroup,
                      App::Prop_None,
                      "The bottom right cell of the range to display");
    ADD_PROPERTY_TYPE(Font, ("Sans"), vgroup, App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(Color,
                      (0.0f, 0.0f, 0.0f),
                      vgroup,
                      App::Prop_None,
                      "The default color of the text and lines");
    ADD_PROPERTY_TYPE(Source, (nullptr), vgroup, App::Prop_None, "Spreadsheet to view");
    ADD_PROPERTY_TYPE(LineWidth, (0.35), vgroup, App::Prop_None, "The thickness of the cell lines");
    ADD_PROPERTY_TYPE(FontSize, (12.0), vgroup, App::Prop_None, "The size of the text");
}


FeatureViewSpreadsheet::~FeatureViewSpreadsheet()
{}

App::DocumentObjectExecReturn* FeatureViewSpreadsheet::execute(void)
{
    // quick tests
    App::DocumentObject* link = Source.getValue();
    std::string scellstart = CellStart.getValue();
    std::string scellend = CellEnd.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No spreadsheet linked");
    }
    if (!link->isDerivedFrom<Spreadsheet::Sheet>()) {
        return new App::DocumentObjectExecReturn("The linked object is not a spreadsheet");
    }
    if ((scellstart.empty()) || (scellend.empty())) {
        return new App::DocumentObjectExecReturn("Empty cell value");
    }

    // build a list of available columns: A, B, C, ... AA, AB, ... ZY, ZZ.
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::vector<std::string> availcolumns;
    for (int i = 0; i < 26; ++i) {
        std::stringstream s;
        s << alphabet[i];
        availcolumns.push_back(s.str());
    }
    for (int i = 0; i < 26; ++i) {
        for (int j = 0; i < 26; ++i) {
            std::stringstream s;
            s << alphabet[i] << alphabet[j];
            availcolumns.push_back(s.str());
        }
    }

    // build rows range and columns range
    std::vector<std::string> columns;
    std::vector<int> rows;
    try {
        for (unsigned int i = 0; i < scellstart.length(); ++i) {
            if (isdigit(scellstart[i])) {
                columns.push_back(scellstart.substr(0, i));
                rows.push_back(std::atoi(scellstart.substr(i, scellstart.length() - 1).c_str()));
            }
        }
        for (unsigned int i = 0; i < scellend.length(); ++i) {
            if (isdigit(scellend[i])) {
                std::string startcol = columns.back();
                std::string endcol = scellend.substr(0, i);
                bool valid = false;
                for (std::vector<std::string>::const_iterator j = availcolumns.begin();
                     j != availcolumns.end();
                     ++j) {
                    if ((*j) == startcol) {
                        if ((*j) != endcol) {
                            valid = true;
                        }
                    }
                    else {
                        if (valid) {
                            if ((*j) == endcol) {
                                columns.push_back((*j));
                                valid = false;
                            }
                            else {
                                columns.push_back((*j));
                            }
                        }
                    }
                }
                int endrow = std::atoi(scellend.substr(i, scellend.length() - 1).c_str());
                for (int j = rows.back() + 1; j <= endrow; ++j) {
                    rows.push_back(j);
                }
            }
        }
    }
    catch (std::exception&) {
        return new App::DocumentObjectExecReturn("Invalid cell range");
    }

    // create the containing group
    std::string ViewName = Label.getValue();
    std::stringstream result, hr, hg, hb;
    const App::Color& c = Color.getValue();
    hr << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * c.r);
    hg << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * c.g);
    hb << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * c.b);
    result << "<g id=\"" << ViewName << "\" transform=\"translate(" << X.getValue() << ","
           << Y.getValue() << ")" << " rotate(" << Rotation.getValue() << ")" << " scale("
           << Scale.getValue() << ")\">" << std::endl;

    // fill the cells
    float rowoffset = 0.0;
    float coloffset = 0.0;
    float cellheight = 100;
    float cellwidth = 100;
    std::string celltext;
    Spreadsheet::Sheet* sheet = static_cast<Spreadsheet::Sheet*>(link);
    std::vector<std::string> skiplist;
    for (std::vector<std::string>::const_iterator col = columns.begin(); col != columns.end();
         ++col) {
        // create a group for each column
        result << "  <g id=\"" << ViewName << "_col" << (*col) << "\">" << std::endl;
        for (std::vector<int>::const_iterator row = rows.begin(); row != rows.end(); ++row) {
            // get cell size
            std::stringstream srow;
            srow << (*row);
            App::CellAddress address((*col) + srow.str());
            cellwidth = sheet->getColumnWidth(address.col());
            cellheight = sheet->getRowHeight(address.row());
            celltext = "";
            // get the text
            App::Property* prop = sheet->getPropertyByName(address.toString().c_str());
            std::stringstream field;
            if (prop) {
                if (prop->isDerivedFrom((App::PropertyQuantity::getClassTypeId()))) {
                    field << static_cast<App::PropertyQuantity*>(prop)->getValue();
                }
                else if (prop->isDerivedFrom((App::PropertyFloat::getClassTypeId()))) {
                    field << static_cast<App::PropertyFloat*>(prop)->getValue();
                }
                else if (prop->isDerivedFrom((App::PropertyString::getClassTypeId()))) {
                    field << static_cast<App::PropertyString*>(prop)->getValue();
                }
                else {
                    assert(0);
                }
                celltext = field.str();
            }
            // get colors, style, alignment and span
            int alignment = 0;
            std::string bcolor = "none";
            std::string fcolor = "#" + hr.str() + hg.str() + hb.str();
            std::string textstyle = "";
            Spreadsheet::Cell* cell = sheet->getCell(address);
            if (cell) {
                App::Color f, b;
                std::set<std::string> st;
                int colspan, rowspan;
                if (cell->getBackground(b)) {
                    std::stringstream br, bg, bb;
                    br << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * b.r);
                    bg << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * b.g);
                    bb << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * b.b);
                    bcolor = "#" + br.str() + bg.str() + bb.str();
                }
                if (cell->getForeground(f)) {
                    std::stringstream fr, fg, fb;
                    fr << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * f.r);
                    fg << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * f.g);
                    fb << std::hex << std::setfill('0') << std::setw(2) << (int)(255.0 * f.b);
                    fcolor = "#" + fr.str() + fg.str() + fb.str();
                }
                if (cell->getStyle(st)) {
                    for (std::set<std::string>::const_iterator i = st.begin(); i != st.end(); ++i) {
                        if ((*i) == "bold") {
                            textstyle = textstyle + "font-weight: bold; ";
                        }
                        else if ((*i) == "italic") {
                            textstyle = textstyle + "font-style: italic; ";
                        }
                        else if ((*i) == "underline") {
                            textstyle = textstyle + "text-decoration: underline; ";
                        }
                    }
                }
                if (cell->getSpans(rowspan, colspan)) {
                    for (int i = 0; i < colspan; ++i) {
                        for (int j = 0; j < rowspan; ++j) {
                            App::CellAddress nextcell(address.row() + j, address.col() + i);
                            if (i > 0) {
                                cellwidth = cellwidth + sheet->getColumnWidth(nextcell.col());
                            }
                            if (j > 0) {
                                cellheight = cellheight + sheet->getRowHeight(nextcell.row());
                            }
                            if ((i > 0) || (j > 0)) {
                                skiplist.push_back(nextcell.toString());
                            }
                        }
                    }
                }
                cell->getAlignment(alignment);
            }
            // skip cell if found in skiplist
            if (std::find(skiplist.begin(), skiplist.end(), address.toString()) == skiplist.end()) {
                result << "    <rect x=\"" << coloffset << "\" y=\"" << rowoffset << "\" width=\""
                       << cellwidth << "\" height=\"" << cellheight << "\" style=\"fill:" << bcolor
                       << ";stroke-width:" << LineWidth.getValue() / Scale.getValue() << ";stroke:#"
                       << hr.str() << hg.str() << hb.str() << ";\" />" << std::endl;
                if (alignment & Spreadsheet::Cell::ALIGNMENT_LEFT) {
                    result << "    <text style=\"" << textstyle << "\" x=\""
                           << coloffset + FontSize.getValue() / 2 << "\" y=\""
                           << rowoffset + 0.75 * cellheight << "\" font-family=\"";
                }
                if (alignment & Spreadsheet::Cell::ALIGNMENT_HCENTER) {
                    result << "    <text text-anchor=\"middle\" style=\"" << textstyle << "\" x=\""
                           << coloffset + cellwidth / 2 << "\" y=\""
                           << rowoffset + 0.75 * cellheight << "\" font-family=\"";
                }
                if (alignment & Spreadsheet::Cell::ALIGNMENT_RIGHT) {
                    result << "    <text text-anchor=\"end\" style=\"" << textstyle << "\" x=\""
                           << coloffset + (cellwidth - FontSize.getValue() / 2) << "\" y=\""
                           << rowoffset + 0.75 * cellheight << "\" font-family=\"";
                }
                result << Font.getValue() << "\"" << " font-size=\"" << FontSize.getValue() << "\""
                       << " fill=\"" << fcolor << "\">" << celltext << "</text>" << std::endl;
            }
            rowoffset = rowoffset + cellheight;
        }
        result << "  </g>" << std::endl;
        rowoffset = 0.0;
        coloffset = coloffset + cellwidth;
    }

    // close the containing group
    result << "</g>" << std::endl;

    // Apply the resulting fragment
    ViewResult.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}
