/***************************************************************************
 *   Copyright (c) 2022 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale <dahale.a.p@gmail.com>                         *
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

#ifndef GUI_VIEWPROVIDERFEMCONSTRAINTONBOUNDARY_H
#define GUI_VIEWPROVIDERFEMCONSTRAINTONBOUNDARY_H

#include <Mod/Part/App/PartFeature.h>

#include "ViewProviderFemConstraint.h"

namespace FemGui
{

class FemGuiExport ViewProviderFemConstraintOnBoundary: public FemGui::ViewProviderFemConstraint
{
    PROPERTY_HEADER_WITH_OVERRIDE(FemGui::ViewProviderFemConstraintOnBoundary);

public:
    ViewProviderFemConstraintOnBoundary();
    ~ViewProviderFemConstraintOnBoundary() override;

    void highlightReferences(const bool on) override;

private:
    std::map<Part::Feature*, std::vector<App::Color>> originalPointColors;
    std::map<Part::Feature*, std::vector<App::Color>> originalLineColors;
    std::map<Part::Feature*, std::vector<App::Color>> originalFaceColors;
};

}  // namespace FemGui

#endif  // GUI_VIEWPROVIDERFEMCONSTRAINTONBOUNDARY_H
