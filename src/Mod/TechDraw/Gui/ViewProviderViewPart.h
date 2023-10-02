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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/PropertyUnits.h>

#include <Mod/TechDraw/App/DrawViewPart.h>

#include "ViewProviderDrawingView.h"


namespace TechDrawGui {

class TechDrawGuiExport ViewProviderViewPart : public ViewProviderDrawingView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderViewPart);

public:
    /// constructor
    ViewProviderViewPart();
    /// destructor
    ~ViewProviderViewPart() override;

    App::PropertyLength LineWidth;
    App::PropertyLength HiddenWidth;
    App::PropertyLength IsoWidth;
    App::PropertyLength ExtraWidth;
    App::PropertyBool   ArcCenterMarks;
    App::PropertyFloat  CenterScale;
    App::PropertyBool   HorizCenterLine;
    App::PropertyBool   VertCenterLine;
    App::PropertyBool   ShowSectionLine;
    App::PropertyEnumeration   SectionLineStyle;
    App::PropertyColor  SectionLineColor;
    App::PropertyBool   SectionLineMarks;
    App::PropertyEnumeration   HighlightLineStyle;
    App::PropertyColor  HighlightLineColor;
    App::PropertyFloat  HighlightAdjust;
    App::PropertyBool   ShowAllEdges;

    static const char* LineStyleEnums[];

    void attach(App::DocumentObject *) override;
    bool useNewSelectionModel(void) const override {return false;}
    bool onDelete(const std::vector<std::string> &) override;
    bool canDelete(App::DocumentObject* obj) const override;
    bool setEdit(int ModNum) override;
    bool doubleClicked(void) override;
    void onChanged(const App::Property *prop) override;
    void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop) override;
    App::Color prefSectionColor(void);
    App::Color prefHighlightColor(void);
    int prefHighlightStyle(void);

    std::vector<App::DocumentObject*> claimChildren(void) const override;
    void fixSceneDependencies();

    std::vector<std::string> getSelectedCosmetics(std::vector<std::string> subNames);
    void deleteCosmeticElements(std::vector<std::string> removables);

    TechDraw::DrawViewPart* getViewObject() const override;
    TechDraw::DrawViewPart* getViewPart() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERVIEWPART_H
