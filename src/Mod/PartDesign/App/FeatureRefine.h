// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 <bgbsww@gmail.com>                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#ifndef PARTDESIGN_FeatureRefine_H
#define PARTDESIGN_FeatureRefine_H

#include "Feature.h"

/// Base class of all features that can be refined PartDesign
namespace PartDesign
{

class PartDesignExport FeatureRefine : public PartDesign::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::FeatureRefine);

public:

    enum class RefineErrorPolicy {
        Raise = 0,
        Warn
    };

    FeatureRefine();

    App::PropertyBool Refine;

protected:
    //store the shape before refinement
    TopoShape rawShape;

    bool onlyHasToRefine() const;
    bool onlyHaveRefined();
    TopoShape refineShapeIfActive(const TopoShape& oldShape, const RefineErrorPolicy onError = RefineErrorPolicy::Raise) const;
};

using FeatureRefinePython = App::FeaturePythonT<FeatureRefine>;

} //namespace PartDesign


#endif // PARTDESIGN_FeatureRefine_H
