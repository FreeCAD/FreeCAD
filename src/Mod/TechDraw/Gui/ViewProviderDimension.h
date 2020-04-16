/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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


#ifndef DRAWINGGUI_VIEWPROVIDERDIMENSION_H
#define DRAWINGGUI_VIEWPROVIDERDIMENSION_H

#include <App/PropertyUnits.h>

#include "ViewProviderDrawingView.h"
#include <Mod/TechDraw/App/DrawViewDimension.h>


namespace TechDrawGui {


class TechDrawGuiExport ViewProviderDimension : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderDimension);

public:
    /// constructor
    ViewProviderDimension();
    /// destructor
    virtual ~ViewProviderDimension();

    App::PropertyFont   Font;
    App::PropertyLength Fontsize;
    App::PropertyLength LineWidth;
    App::PropertyColor  Color;

    static const int STD_STYLE_ISO_ORIENTED     = 0;
    static const int STD_STYLE_ISO_REFERENCING  = 1;
    static const int STD_STYLE_ASME_INLINED     = 2;
    static const int STD_STYLE_ASME_REFERENCING = 3;
    App::PropertyEnumeration StandardAndStyle;

    static const int REND_EXTENT_NONE     = 0;
    static const int REND_EXTENT_MINIMAL  = 1;
    static const int REND_EXTENT_CONFINED = 2;
    static const int REND_EXTENT_REDUCED  = 3;
    static const int REND_EXTENT_NORMAL   = 4;
    static const int REND_EXTENT_EXPANDED = 5;
    App::PropertyEnumeration RenderingExtent;

    App::PropertyBool        FlipArrowheads;

    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    virtual bool useNewSelectionModel(void) const {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property* p);

    virtual TechDraw::DrawViewDimension* getViewObject() const;

    App::Color prefColor() const;
    std::string prefFont() const;
    double prefFontSize() const;
    double prefWeight() const;
    int prefStandardAndStyle() const;
    virtual bool canDelete(App::DocumentObject* obj) const;

protected:
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop);

private:
    static const char *StandardAndStyleEnums[];
    static const char *RenderingExtentEnums[];

};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERDIMENSION_H
