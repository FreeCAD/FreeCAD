// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   License along with this library; see the file COPYING.LIB. If not,   *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <FCConfig.h>

#include <BOPAlgo_ArgumentAnalyzer.hxx>
#include <BRepBuilderAPI_Copy.hxx>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "FeaturePartRefinable.h"
#include "TopoShape.h"

using namespace Part;

namespace
{

bool getRefineModelParameter()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    return hGrp->GetBool("RefineModel", true);
}

bool getCheckRefineParameter()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    return hGrp->GetBool("CheckRefine", false);
}

}  // namespace

PROPERTY_SOURCE_ABSTRACT(Part::RefinableFeature, Part::Feature)


RefinableFeature::RefinableFeature()
{
    ADD_PROPERTY_TYPE(
        Refine,
        (0),
        "Boolean",
        (App::PropertyType)(App::Prop_None),
        "Refine shape (clean up redundant edges) after this boolean operation"
    );
    ADD_PROPERTY_TYPE(
        CheckRefine,
        (false),
        "Boolean",
        (App::PropertyType)(App::Prop_None),
        "Validate refine result and revert if it introduces self-intersections (slower)"
    );

    this->Refine.setValue(getRefineModelParameter());
    this->CheckRefine.setValue(getCheckRefineParameter());
}

bool RefinableFeature::isRefineResultValid(const TopoDS_Shape& shape) const
{
    if (!this->CheckRefine.getValue()) {
        return true;  // validation disabled, assume valid
    }

    // Run BOPAlgo_ArgumentAnalyzer to detect self-intersections that
    // BRepCheck_Analyzer misses.  This catches corruption introduced by
    // ShapeUpgrade_UnifySameDomain (removeSplitter/Refine) on shapes with
    // coplanar faces and partial overlap.
    TopoDS_Shape copy = BRepBuilderAPI_Copy(shape).Shape();
    BOPAlgo_ArgumentAnalyzer checker;
    checker.SetShape1(copy);
    checker.SelfInterMode() = true;
    checker.SetRunParallel(true);
    checker.Perform();
    return !checker.HasFaulty();
}

void RefinableFeature::applyRefine(TopoShape& res) const
{
    if (!this->Refine.getValue()) {
        return;
    }

    TopoShape preRefine = res;
    res = res.makeElementRefine();
    if (!isRefineResultValid(res.getShape())) {
        res = preRefine;
        Base::Console().warning(
            "'%s': The boolean result is correct, but the Refine "
            "(cleanup) step damaged it and was skipped. The result "
            "may have extra internal edges. To prevent this, disable "
            "Refine in this feature's properties. This is a known "
            "limitation of the geometry engine. "
            "See: https://wiki.freecad.org/Boolean_Troubleshooting\n",
            this->Label.getValue()
        );
    }
}
