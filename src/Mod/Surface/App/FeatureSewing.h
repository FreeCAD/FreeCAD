// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
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

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Surface/SurfaceGlobal.h>


namespace Surface
{

class SurfaceExport Sewing: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Surface::Sewing);

public:
    Sewing();

    App::PropertyLinkSubList ShapeList;  // Shapes to be sewn.

    App::PropertyFloat Tolerance;
    App::PropertyBool SewingOption;     // Option for sewing (if false only control)
    App::PropertyBool DegenerateShape;  // Option for analysis of degenerated shapes
    App::PropertyBool CutFreeEdges;     // Option for cutting of free edges
    App::PropertyBool Nonmanifold;      // Option for non-manifold processing

    // recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
};

}  // Namespace Surface
