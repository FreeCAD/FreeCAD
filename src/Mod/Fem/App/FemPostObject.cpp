/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <vtkPointData.h>
# include <vtkCellData.h>
#endif

#include "FemPostObject.h"
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostObject, App::GeoFeature)


FemPostObject::FemPostObject()
{
    ADD_PROPERTY(Data,(0));
}

FemPostObject::~FemPostObject()
{
}

vtkBoundingBox FemPostObject::getBoundingBox() {

    vtkBoundingBox box;

    if(Data.getValue() && Data.getValue()->IsA("vtkDataSet"))
        box.AddBounds(vtkDataSet::SafeDownCast(Data.getValue())->GetBounds());

    //TODO: add calculation of multiblock and Multipiece datasets

    return box;
}
