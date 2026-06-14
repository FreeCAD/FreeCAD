/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
//this file originally part of TechDraw workbench
//migrated to TechDraw workbench 2022-01-26 by Wandererfan

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>


namespace TechDraw
{

/** Base class of all View Features in the drawing module
 */
class TechDrawExport FeatureProjection : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::FeatureProjection);

public:
    /// Constructor
    FeatureProjection();
    ~FeatureProjection() override;

    App::PropertyLink   Source;
    App::PropertyVector Direction;
    App::PropertyBool   VCompound;
    App::PropertyBool   Rg1LineVCompound;
    App::PropertyBool   RgNLineVCompound;
    App::PropertyBool   OutLineVCompound;
    App::PropertyBool   IsoLineVCompound;
    App::PropertyBool   HCompound;
    App::PropertyBool   Rg1LineHCompound;
    App::PropertyBool   RgNLineHCompound;
    App::PropertyBool   OutLineHCompound;
    App::PropertyBool   IsoLineHCompound;

    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    //@}
};

} //namespace TechDraw