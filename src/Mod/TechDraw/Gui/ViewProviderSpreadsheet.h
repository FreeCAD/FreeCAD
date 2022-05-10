/***************************************************************************
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

#ifndef DRAWINGGUI_VIEWPROVIDERSPREADSHEET_H
#define DRAWINGGUI_VIEWPROVIDERSPREADSHEET_H

#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>

#include "ViewProviderSymbol.h"


namespace TechDrawGui {


class TechDrawGuiExport ViewProviderSpreadsheet : public ViewProviderSymbol
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderSpreadsheet);

public:
    /// constructor
    ViewProviderSpreadsheet();
    /// destructor
    virtual ~ViewProviderSpreadsheet();


    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);

    virtual TechDraw::DrawViewSpreadsheet* getViewObject() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERSPREADSHEET_H
