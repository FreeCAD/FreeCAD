/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "ViewProviderDerivedPart.h"
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
// #include <Mod/Part/App/FeaturePartBoolean.h>
// #include <Mod/Part/App/FeaturePartFuse.h>
// #include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeatureDerivedPart.h>

using namespace PartGui;
PROPERTY_SOURCE(PartGui::ViewProviderDerivedPart,PartGui::ViewProviderPart)

ViewProviderDerivedPart::ViewProviderDerivedPart()
{
}


ViewProviderDerivedPart::~ViewProviderDerivedPart()
{
}

