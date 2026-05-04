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


#pragma once

#include "Feature.h"

/// Base class of all features that can be refined PartDesign
namespace PartDesign
{

class PartDesignExport FeatureRefine: public PartDesign::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::FeatureRefine);

public:
    enum class RefineErrorPolicy
    {
        Raise = 0,
        Warn
    };

    FeatureRefine();

    App::PropertyBool Refine;

protected:
    // store the shape before refinement
    TopoShape rawShape;


    /**
     * Check if the feature *only* requires the refinement operation, and do that refinement if so.
     * Typically called as the first operation in a subclass's `execute()` method to provide an
     * early exit if no other parameters have been changed (so the base feature is still
     * up-to-date).
     *
     * @return true if the refine was done and that was the only thing needed, or false if further
     * computation is necessary.
     */
    bool onlyHaveRefined();
    TopoShape refineShapeIfActive(
        const TopoShape& oldShape,
        const RefineErrorPolicy onError = RefineErrorPolicy::Warn
    ) const;
};

using FeatureRefinePython = App::FeaturePythonT<FeatureRefine>;

}  // namespace PartDesign
