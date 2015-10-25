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


#ifndef Fem_FemPostObject_H
#define Fem_FemPostObject_H

#include <App/GeoFeature.h>

#include <vtkSmartPointer.h>
#include <vtkPolyDataAlgorithm.h>

namespace Fem
{

//poly data is the only data we can visualize, hence every post processing object needs to expose it
class AppFemExport FemPostObject : public App::GeoFeature
{
    PROPERTY_HEADER(Fem::FemPostObject);

public:
    /// Constructor
    FemPostObject(void);
    virtual ~FemPostObject();
    
    App::PropertyInteger ModificationTime;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostObject";
    }
    
    short mustExecute(void) const;
    virtual App::DocumentObjectExecReturn* execute(void);
    
    PyObject* getPyObject();
    
    //get the algorithm or the data
    vtkPolyData*                                getPolyData() {return polyDataSource->GetOutput();};
    vtkSmartPointer<vtkPolyDataAlgorithm>       getPolyAlgorithm() {return polyDataSource;};
    

protected:
    virtual void onChanged(const App::Property* prop);
    
    //members
    vtkSmartPointer<vtkPolyDataAlgorithm> polyDataSource;
};

} //namespace Fem


#endif // Fem_FemPostObject_H
