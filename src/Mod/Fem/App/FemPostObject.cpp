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
#endif

#include "FemPostObject.h"
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <vtkPointData.h>
#include <vtkCellData.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostObject, App::GeoFeature)


FemPostObject::FemPostObject()
{
    ADD_PROPERTY(ModificationTime,(0));
}

FemPostObject::~FemPostObject()
{
}

short FemPostObject::mustExecute(void) const
{
    return 1;
}

DocumentObjectExecReturn* FemPostObject::execute(void) {
       
    if(providesPolyData()) {
        polyDataSource->Update();
        vtkSmartPointer<vtkPolyData> poly = polyDataSource->GetOutput();
        
        if(static_cast<unsigned long>(ModificationTime.getValue()) != poly->GetMTime()) {
            
            //update the bounding box
            m_boundingBox = vtkBoundingBox(poly->GetBounds());
            
            //update the modification time to let the viewprovider know something changed
            ModificationTime.setValue(static_cast<long>(poly->GetMTime()));   
        }
    }
    
    return DocumentObject::StdReturn;
}

void FemPostObject::onChanged(const Property* prop)
{
    App::GeoFeature::onChanged(prop);
}
