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


#ifndef Fem_FemPostPipeline_H
#define Fem_FemPostPipeline_H

#include "FemPostObject.h"

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>

namespace Fem
{

class AppFemExport FemPostPipeline : public Fem::FemPostObject
{
    PROPERTY_HEADER(Fem::FemPostPipeline);

public:
    /// Constructor
    FemPostPipeline(void);
    virtual ~FemPostPipeline();

    /// returns the type name of the ViewProvider
//     virtual const char* getViewProviderName(void) const {
//         return "FemGui::ViewProviderPostPipeline";
//     }
    
    short mustExecute(void) const;
    virtual App::DocumentObjectExecReturn* execute(void);
    PyObject* getPyObject();

    //load data from files
    static bool canRead(Base::FileInfo file);
    void read(Base::FileInfo file);
    
protected:
    virtual void onChanged(const App::Property* prop);
    
    //members
    vtkSmartPointer<vtkDataSet> source;
};

} //namespace Fem


#endif // Fem_FemPostPipeline_H
