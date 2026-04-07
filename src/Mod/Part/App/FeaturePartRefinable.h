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

#pragma once

#include <Mod/Part/PartGlobal.h>

#include "PartFeature.h"

namespace Part
{

/// Abstract base for boolean features that support Refine and CheckRefine.
///
/// Owns the Refine and CheckRefine properties and the logic to validate and
/// apply the refine step. Boolean, MultiFuse, and MultiCommon all inherit
/// from this class so the properties and behaviour are defined once.
class PartExport RefinableFeature: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::RefinableFeature);

public:
    RefinableFeature();

    /// Refine shape (clean up redundant edges) after the boolean operation.
    App::PropertyBool Refine;

    /// Validate the refined shape using BOPAlgo self-intersection detection
    /// and revert to the pre-refine result if corruption is detected.
    /// Seeded from the CheckRefine user preference at feature creation time.
    /// Has no effect when Refine is false.
    App::PropertyBool CheckRefine;

protected:
    /// Check whether @p shape is valid after a refine step.
    ///
    /// Returns true immediately when CheckRefine is false (fast path).
    /// Otherwise runs BOPAlgo_ArgumentAnalyzer with self-intersection
    /// detection, which catches corruption that BRepCheck_Analyzer misses.
    bool isRefineResultValid(const TopoDS_Shape& shape) const;

    /// Apply makeElementRefine() to @p res.
    ///
    /// When Refine is false, does nothing. When Refine is true, applies the
    /// refine step; if CheckRefine is true and the result is invalid, reverts
    /// to the pre-refine shape and emits a warning to the Report View.
    void applyRefine(TopoShape& res) const;
};

}  // namespace Part
