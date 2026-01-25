/***************************************************************************
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

#pragma once

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


namespace TechDraw
{

class TechDrawExport DrawTile : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawTile);

public:
    DrawTile();
    ~DrawTile() override = default;

    App::PropertyLink         TileParent;           //eg DrawWeldSymbol
    App::PropertyIntegerConstraint TileRow;
    App::PropertyIntegerConstraint::Constraints  TileRowConstraints;
    App::PropertyInteger      TileColumn;

    short mustExecute() const override;
    App::DocumentObjectExecReturn *execute(void) override;

    const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderTile";
    }
    PyObject *getPyObject(void) override;
    virtual DrawView* getParent(void) const;

protected:
    void onChanged(const App::Property* prop) override;
    void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop) override;

private:
};

using DrawTilePython = App::FeaturePythonT<DrawTile>;

} //namespace TechDraw