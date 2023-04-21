/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef TECHDRAWGUI_VIEWPROVIDERVIEWSECTION_H
#define TECHDRAWGUI_VIEWPROVIDERVIEWSECTION_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Mod/TechDraw/App/DrawViewSection.h>

#include "ViewProviderViewPart.h"


namespace TechDrawGui {

class TechDrawGuiExport ViewProviderViewSection : public ViewProviderViewPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderViewSection);

public:
    /// constructor
    ViewProviderViewSection();
    /// destructor
    ~ViewProviderViewSection() override;

    App::PropertyBool   ShowCutSurface;        //obsolete - use CutSurfaceDisplay
    App::PropertyColor  CutSurfaceColor;
    App::PropertyBool   HatchCutSurface;       //obsolete - use CutSurfaceDisplay
    App::PropertyColor  HatchColor;
    App::PropertyColor  GeomHatchColor;
    App::PropertyFloat  WeightPattern;

    void updateData(const App::Property*) override;
    void onChanged(const App::Property *prop) override;
    bool setEdit(int ModNum) override;
    bool doubleClicked() override;

    void updateGraphic();
    void getParameters();
    bool canDelete(App::DocumentObject* obj) const override;

    TechDraw::DrawViewSection* getViewObject() const override;
};

} // namespace TechDrawGui


#endif // TECHDRAWGUI_VIEWPROVIDERVIEWSECTION_H
