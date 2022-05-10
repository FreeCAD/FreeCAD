/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef DRAWINGGUI_VIEWPROVIDERTILE_H
#define DRAWINGGUI_VIEWPROVIDERTILE_H

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/TechDraw/App/DrawTile.h>

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderTile : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderTile);

public:
    /// constructor
    ViewProviderTile();
    /// destructor
    virtual ~ViewProviderTile();


    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    virtual bool canDelete(App::DocumentObject *obj) const;

/*    virtual TechDraw::DrawTile* getViewObject() const;*/
    virtual TechDraw::DrawTile* getFeature() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERTILE_H
