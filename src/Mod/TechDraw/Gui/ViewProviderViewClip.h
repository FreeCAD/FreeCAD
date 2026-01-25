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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Mod/TechDraw/App/DrawViewClip.h>

#include "ViewProviderDrawingView.h"


namespace TechDrawGui {

class TechDrawGuiExport ViewProviderViewClip : public ViewProviderDrawingView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderViewClip);

public:
    /// constructor
    ViewProviderViewClip();
    /// destructor
    ~ViewProviderViewClip() override;

    App::PropertyBool   ClipChildren;

    bool useNewSelectionModel() const override {return false;}

    TechDraw::DrawViewClip* getViewObject() const override;
    TechDraw::DrawViewClip* getObject() const;

    void onChanged(const App::Property *prop) override;

    /// Hide the object in the view
    void hide() override;
    /// Show the object in the view
    void show() override;
    std::vector<App::DocumentObject*> claimChildren() const override;

    bool canDelete(App::DocumentObject* obj) const override;

    void dragObject(App::DocumentObject* docObj) override;
    void dropObject(App::DocumentObject* docObj) override;
};
} // namespace TechDrawGui