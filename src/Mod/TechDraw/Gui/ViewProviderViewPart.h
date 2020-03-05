/***************************************************************************
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


#ifndef DRAWINGGUI_VIEWPROVIDERVIEWPART_H
#define DRAWINGGUI_VIEWPROVIDERVIEWPART_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include "ViewProviderDrawingView.h"
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderViewPart : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderViewPart);

public:
    /// constructor
    ViewProviderViewPart();
    /// destructor
    virtual ~ViewProviderViewPart();

    App::PropertyLength LineWidth;
    App::PropertyLength HiddenWidth;
    App::PropertyLength IsoWidth;
    App::PropertyLength ExtraWidth;
    App::PropertyBool   ArcCenterMarks;
    App::PropertyFloat  CenterScale;
    App::PropertyBool   HorizCenterLine;
    App::PropertyBool   VertCenterLine;
    App::PropertyBool   ShowSectionLine;
    App::PropertyFloat  HighlightAdjust;
    App::PropertyBool   ShowAllEdges;

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual bool onDelete(const std::vector<std::string> &);
    virtual bool canDelete(App::DocumentObject* obj) const;

public:
    virtual void onChanged(const App::Property *prop);
    virtual void updateData(const App::Property*);
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop);


    virtual std::vector<App::DocumentObject*> claimChildren(void) const;

    virtual TechDraw::DrawViewPart* getViewObject() const;
    TechDraw::DrawViewPart* getViewPart() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERVIEWPART_H
