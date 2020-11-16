/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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


#ifndef DRAWINGGUI_VIEWPROVIDERHATCH_H
#define DRAWINGGUI_VIEWPROVIDERHATCH_H

#include <Gui/ViewProviderDocumentObject.h>

namespace TechDraw{
    class DrawHatch;
}

namespace TechDrawGui {


class TechDrawGuiExport ViewProviderHatch : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderHatch);

public:
    /// constructor
    ViewProviderHatch();
    /// destructor
    virtual ~ViewProviderHatch();

    App::PropertyColor       HatchColor;
    App::PropertyFloatConstraint HatchScale;

    virtual void attach(App::DocumentObject *) override;
    virtual void setDisplayMode(const char* ModeName) override;
    virtual bool useNewSelectionModel(void) const override {return false;}
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const override;
    virtual void onChanged(const App::Property* prop) override;
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;
    virtual bool doubleClicked(void) override;
    virtual void updateData(const App::Property*) override;
    virtual bool canDelete(App::DocumentObject* obj) const override;

    TechDraw::DrawHatch* getViewObject() const;

    virtual Gui::MDIView *getMDIView() const override;

private:
    static App::PropertyFloatConstraint::Constraints scaleRange;

};

} // namespace TechDrawGui


#endif // DRAWINGGUI_VIEWPROVIDERHATCH_H
