/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <Mod/TechDraw/App/DrawViewImage.h>

#include "ViewProviderDrawingView.h"


namespace TechDrawGui {


class TechDrawGuiExport ViewProviderImage : public ViewProviderDrawingView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderImage);

public:
    /// constructor
    ViewProviderImage();
    /// destructor
    ~ViewProviderImage() override;

    App::PropertyBool  Crop;              //crop to feature width x height

    bool useNewSelectionModel() const override {return false;}
    /// returns a list of all possible modes
    void updateData(const App::Property*) override;
    void onChanged(const App::Property *prop) override;

    TechDraw::DrawViewImage* getViewObject() const override;
};


} // namespace TechDrawGui