/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef TECHDRAWGUI_VIEWPROVIDERRICHANNO_H
#define TECHDRAWGUI_VIEWPROVIDERRICHANNO_H

#include <App/PropertyUnits.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>

#include "ViewProviderDrawingView.h"


namespace TechDraw {
class DrawRichAnno;
}

namespace TechDrawGui {

class TechDrawGuiExport ViewProviderRichAnno : public ViewProviderDrawingView
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderRichAnno);

public:
    /// constructor
    ViewProviderRichAnno();
    /// destructor
    virtual ~ViewProviderRichAnno();

    App::PropertyLength      LineWidth;
    App::PropertyEnumeration LineStyle;
    App::PropertyColor       LineColor;

    virtual void attach(App::DocumentObject *);
    virtual bool useNewSelectionModel(void) const {return false;}
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property* p);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);
    virtual bool canDelete(App::DocumentObject* obj) const;

    static const char* LineStyleEnums[];

    virtual TechDraw::DrawRichAnno* getViewObject() const;
    TechDraw::DrawRichAnno* getFeature()  const;

protected:
    App::Color getDefLineColor(void);
    std::string getDefFont(void);
    double getDefFontSize(void);
    double getDefLineWeight(void);
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop);

private:
    static App::PropertyIntegerConstraint::Constraints LineStyleRange;

};

} // namespace TechDrawGui

#endif // TECHDRAWGUI_VIEWPROVIDERRICHANNO_H
