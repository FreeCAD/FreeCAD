/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef Fem_FemMeshShapeNetgenObject_H
#define Fem_FemMeshShapeNetgenObject_H


#include "FemMesh.h"
#include "FemMeshShapeObject.h"
#include <App/PropertyStandard.h>

namespace Fem
{

class AppFemExport FemMeshShapeNetgenObject : public FemMeshShapeObject
{
    PROPERTY_HEADER(Fem::FemMeshShapeNetgenObject);

public:
    /// Constructor
    FemMeshShapeNetgenObject(void);
    virtual ~FemMeshShapeNetgenObject();

    App::PropertyFloat          MaxSize;
    App::PropertyBool           SecondOrder;
    App::PropertyEnumeration    Fininess;
    App::PropertyFloat          GrowthRate;
    App::PropertyInteger        NbSegsPerEdge;
    App::PropertyInteger        NbSegsPerRadius;    
    App::PropertyBool           Optimize;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemMeshShapeNetgen";
    }
    virtual App::DocumentObjectExecReturn *execute(void);

    //virtual short mustExecute(void) const;
    //virtual PyObject *getPyObject(void);

    //App::PropertyLink Shape;

protected:
    /// get called by the container when a property has changed
    //virtual void onChanged (const App::Property* prop);
};

} //namespace Fem


#endif // Fem_FemMeshShapeNetgenObject_H
