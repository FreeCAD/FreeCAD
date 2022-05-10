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

#ifndef Fem_FemPostFunction_H
#define Fem_FemPostFunction_H

#include <vtkBoundingBox.h>
#include <vtkImplicitFunction.h>
#include <vtkPlane.h>
#include <vtkSphere.h>
#include <vtkSmartPointer.h>

#include <App/PropertyUnits.h>

#include "FemPostObject.h"


namespace Fem
{

class FemExport FemPostFunction : public App::DocumentObject
{
    PROPERTY_HEADER(Fem::FemPostFunction);

public:
    /// Constructor
    FemPostFunction(void);
    virtual ~FemPostFunction();

    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostFunction";
    }

    virtual App::DocumentObjectExecReturn* execute(void);

    //bound box handling
    void setBoundingBox(vtkBoundingBox b) {m_boundingBox = b;};

    //get the algorithm or the data
    vtkSmartPointer<vtkImplicitFunction> getImplicitFunction() {return m_implicit;};

protected:
    vtkSmartPointer<vtkImplicitFunction>  m_implicit;
    vtkBoundingBox                        m_boundingBox;
};

class FemExport FemPostFunctionProvider : public App::DocumentObject {

    PROPERTY_HEADER(Fem::FemPostFunctionProvider);

public:
    FemPostFunctionProvider(void);
    virtual ~FemPostFunctionProvider();

    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostFunctionProvider";
    }

    App::PropertyLinkList Functions;

protected:
    virtual void onChanged(const App::Property* prop);
};


// ---------------------------------------------------------------------------

class FemExport FemPostPlaneFunction : public FemPostFunction
{
    PROPERTY_HEADER(Fem::FemPostPlaneFunction);

public:

    FemPostPlaneFunction(void);
    virtual ~FemPostPlaneFunction();

    App::PropertyVector           Normal;
    App::PropertyVectorDistance   Origin;

    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostPlaneFunction";
    }

protected:
    virtual void onChanged(const App::Property* prop);
    /// get called after a document has been fully restored
    virtual void onDocumentRestored();

    vtkSmartPointer<vtkPlane> m_plane;
};

// ---------------------------------------------------------------------------

class FemExport FemPostSphereFunction : public FemPostFunction
{
    PROPERTY_HEADER(Fem::FemPostSphereFunction);

public:

    FemPostSphereFunction(void);
    virtual ~FemPostSphereFunction();

    App::PropertyDistance         Radius;
    App::PropertyVectorDistance   Center;

    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemPostSphereFunction";
    }

protected:
    virtual void onChanged(const App::Property* prop);

    vtkSmartPointer<vtkSphere> m_sphere;
};

} //namespace Fem


#endif // Fem_FemPostFunction_H
