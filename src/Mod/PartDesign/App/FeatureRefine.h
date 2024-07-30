/***************************************************************************
 *   Copyright (c) 2024 <bgbsww@gmail.com>                                 *
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
    FeatureRefine();

    App::PropertyBool Refine;

protected:
    TopoShape refineShapeIfActive(const TopoShape&) const;
};

using FeatureRefinePython = App::FeaturePythonT<FeatureRefine>;

} //namespace PartDesign


#endif // PARTDESIGN_FeatureRefine_H
