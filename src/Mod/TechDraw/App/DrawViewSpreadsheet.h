/***************************************************************************
 *   Copyright (c) 2016 WandererFan    (wandererfan@gmail.com)             *
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




#ifndef _DrawViewSpreadsheet_h_
#define _DrawViewSpreadsheet_h_


#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>
#include <App/FeaturePython.h>

#include "DrawViewSymbol.h"

namespace TechDraw
{


class TechDrawExport DrawViewSpreadsheet : public TechDraw::DrawViewSymbol
{
    PROPERTY_HEADER(TechDraw::DrawViewSpreadsheet);

public:
    DrawViewSpreadsheet(void);
    virtual ~DrawViewSpreadsheet();
    App::PropertyLink         Source;
    App::PropertyString       CellStart;
    App::PropertyString       CellEnd;
    App::PropertyFont         Font;
    App::PropertyColor        TextColor;
    App::PropertyFloat        LineWidth;
    App::PropertyFloat        TextSize;


    virtual App::DocumentObjectExecReturn *execute(void);
    virtual short mustExecute() const;
    std::string getSheetImage(void);

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderSpreadsheet";
    }

protected:
    virtual void onChanged(const App::Property* prop);
    std::vector<std::string> getAvailColumns(void);
    std::string getSVGHead(void);
    std::string getSVGTail(void);
    int colInList(const std::vector<std::string>& list,
                   const std::string& toFind);

private:
};

typedef App::FeaturePythonT<DrawViewSpreadsheet> DrawViewSpreadsheetPython;


} //namespace TechDraw


#endif
