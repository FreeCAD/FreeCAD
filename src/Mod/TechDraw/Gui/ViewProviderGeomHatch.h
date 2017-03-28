/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2017 Wandererfan <wandererfan@gmail.com>                *
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


#ifndef DRAWINGGUI_VIEWPROVIDERCROSSHATCH_H
#define DRAWINGGUI_VIEWPROVIDERCROSSHATCH_H

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyStandard.h>

#include <Gui/ViewProviderFeature.h>


namespace TechDraw{
    class DrawGeomHatch;
}

namespace TechDrawGui {


class TechDrawGuiExport ViewProviderGeomHatch : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(TechDrawGui::ViewProviderGeomHatch);

public:
    /// constructor
    ViewProviderGeomHatch();
    /// destructor
    virtual ~ViewProviderGeomHatch();

    App::PropertyFloat       WeightPattern;
    App::PropertyColor       ColorPattern;


    virtual void attach(App::DocumentObject *);
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property *prop);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual bool doubleClicked(void);

    virtual bool useNewSelectionModel(void) const {return false;}
    virtual void setDisplayMode(const char* ModeName);
    virtual std::vector<std::string> getDisplayModes(void) const;
    void updateGraphic(void);
    void getParameters(void);

    TechDraw::DrawGeomHatch* getViewObject() const;
};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERHATCH_H
