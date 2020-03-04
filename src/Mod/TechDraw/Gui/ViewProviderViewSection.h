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


#ifndef DRAWINGGUI_VIEWPROVIDERVIEWSECTION_H
#define DRAWINGGUI_VIEWPROVIDERVIEWSECTION_H

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

#include "ViewProviderViewPart.h"

namespace TechDrawGui {


class TechDrawGuiExport ViewProviderViewSection : public ViewProviderViewPart
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderViewSection);

public:
    /// constructor
    ViewProviderViewSection();
    /// destructor
    virtual ~ViewProviderViewSection();

    App::PropertyBool   ShowCutSurface;        //obsolete - use CutSurfaceDisplay
    App::PropertyColor  CutSurfaceColor;
    App::PropertyBool   HatchCutSurface;       //obsolete - use CutSurfaceDisplay
    App::PropertyColor  HatchColor;
    App::PropertyFloat  WeightPattern;


    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property *prop);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);

    virtual std::vector<App::DocumentObject*> claimChildren(void) const;

    void updateGraphic(void);
    void getParameters(void);
    virtual bool canDelete(App::DocumentObject* obj) const;

    virtual TechDraw::DrawViewSection* getViewObject() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERVIEW_H
