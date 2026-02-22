// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <utility>

#include <Gui/ViewProviderBuilder.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Mod/Part/PartGlobal.h>

class SoSeparator;

namespace Gui
{
class GizmoContainer;
}

namespace Part
{
struct ShapeHistory;
}

namespace PartGui
{

class ViewProviderShapeBuilder: public Gui::ViewProviderBuilder
{
public:
    ViewProviderShapeBuilder() = default;
    ~ViewProviderShapeBuilder() override = default;
    void buildNodes(const App::Property*, std::vector<SoNode*>&) const override;
    void createShape(const App::Property*, SoSeparator*) const;
};

class PartGuiExport ViewProviderPart: public ViewProviderPartExt
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderPart);

public:
    /// constructor
    ViewProviderPart();
    /// destructor
    ~ViewProviderPart() override;
    bool doubleClicked() override;

protected:
    void applyColor(
        const Part::ShapeHistory& hist,
        const std::vector<Base::Color>& colBase,
        std::vector<Base::Color>& colBool
    );
    void applyMaterial(
        const Part::ShapeHistory& hist,
        const std::vector<App::Material>& colBase,
        std::vector<App::Material>& colBool
    );
    void applyTransparency(float transparency, std::vector<Base::Color>& colors);
    void applyTransparency(float transparency, std::vector<App::Material>& colors);
};

}  // namespace PartGui
