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
#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>

#include "ViewProviderDrawingView.h"


namespace TechDrawGui {

class TechDrawGuiExport ViewProviderDimension : public ViewProviderDrawingView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderDimension);

public:
    /// constructor
    ViewProviderDimension();
    /// destructor
    ~ViewProviderDimension() override;

    App::PropertyFont   Font;
    App::PropertyLength Fontsize;
    App::PropertyLength Arrowsize;
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

    App::PropertyFloat GapFactorISO;
    App::PropertyFloat GapFactorASME;
    App::PropertyFloat LineSpacingFactorISO;

    void attach(App::DocumentObject *) override;
    bool useNewSelectionModel() const override {return false;}
    void updateData(const App::Property*) override;
    void onChanged(const App::Property* p) override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool setEdit(int ModNum) override;
    bool doubleClicked() override;
    bool onDelete(const std::vector<std::string> & parms) override;


    TechDraw::DrawViewDimension* getViewObject() const override;

    App::Color prefColor() const;
    std::string prefFont() const;
    double prefFontSize() const;
    double prefArrowSize() const;
    double prefWeight() const;
    int prefStandardAndStyle() const;
    bool canDelete(App::DocumentObject* obj) const override;
    void setPixmapForType();

protected:
    void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop) override;

private:
    static const char *StandardAndStyleEnums[];
    static const char *RenderingExtentEnums[];

};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERDIMENSION_H
