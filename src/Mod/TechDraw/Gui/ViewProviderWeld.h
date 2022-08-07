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

#ifndef DRAWINGGUI_VIEWPROVIDERWELD_H
#define DRAWINGGUI_VIEWPROVIDERWELD_H

#include <Gui/ViewProviderFeature.h>

#include "ViewProviderDrawingView.h"
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>


namespace TechDrawGui {

class TechDrawGuiExport ViewProviderWeld : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderWeld);

public:
    /// constructor
    ViewProviderWeld();
    /// destructor
    virtual ~ViewProviderWeld();

    App::PropertyString      Font;
    App::PropertyLength      FontSize;
    App::PropertyLength      TileFontSize;

    virtual bool useNewSelectionModel() const {return false;}
    virtual void onChanged(const App::Property* p);
    virtual std::vector<App::DocumentObject*> claimChildren() const;
    virtual bool setEdit(int ModNum);
    virtual bool doubleClicked();

    virtual TechDraw::DrawWeldSymbol* getViewObject() const;
    virtual TechDraw::DrawWeldSymbol* getFeature() const;

    std::string prefFontName();
    double prefFontSize();
    double prefTileTextAdjust();
    virtual bool onDelete(const std::vector<std::string> &);
    virtual bool canDelete(App::DocumentObject* obj) const;
    
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERWELD_H
