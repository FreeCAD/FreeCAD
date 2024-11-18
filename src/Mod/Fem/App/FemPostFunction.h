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
#include <vtkBox.h>
#include <vtkCylinder.h>
#include <vtkImplicitFunction.h>
#include <vtkPlane.h>
#include <vtkSmartPointer.h>
#include <vtkSphere.h>

#include <App/PropertyUnits.h>

#include "FemPostObject.h"


namespace Fem
{

class FemExport FemPostFunction: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostFunction);

public:
    /// Constructor
    FemPostFunction();
    ~FemPostFunction() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostFunction";
    }

    App::DocumentObjectExecReturn* execute() override;

    // bound box handling
    void setBoundingBox(vtkBoundingBox b)
    {
        m_boundingBox = b;
    };

    // get the algorithm or the data
    vtkSmartPointer<vtkImplicitFunction> getImplicitFunction()
    {
        return m_implicit;
    };

protected:
    vtkSmartPointer<vtkImplicitFunction> m_implicit;
    vtkBoundingBox m_boundingBox;
};

class FemExport FemPostFunctionProvider: public App::DocumentObject
{

    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostFunctionProvider);

public:
    FemPostFunctionProvider();
    ~FemPostFunctionProvider() override;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostFunctionProvider";
    }

    App::PropertyLinkList Functions;

protected:
    void onChanged(const App::Property* prop) override;
};

// ---------------------------------------------------------------------------

class FemExport FemPostBoxFunction: public FemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostBoxFunction);

public:
    FemPostBoxFunction();
    ~FemPostBoxFunction() override;

    App::PropertyVectorDistance Center;
    App::PropertyDistance Length;
    App::PropertyDistance Width;
    App::PropertyDistance Height;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostBoxFunction";
    }

protected:
    void onChanged(const App::Property* prop) override;
    /// get called after a document has been fully restored
    void onDocumentRestored() override;

    vtkSmartPointer<vtkBox> m_box;
};

// ---------------------------------------------------------------------------

class FemExport FemPostCylinderFunction: public FemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostCylinderFunction);

public:
    FemPostCylinderFunction();
    ~FemPostCylinderFunction() override;

    App::PropertyVector Axis;
    App::PropertyVectorDistance Center;
    App::PropertyDistance Radius;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostCylinderFunction";
    }

protected:
    void onChanged(const App::Property* prop) override;
    /// get called after a document has been fully restored
    void onDocumentRestored() override;

    vtkSmartPointer<vtkCylinder> m_cylinder;
};

// ---------------------------------------------------------------------------

class FemExport FemPostPlaneFunction: public FemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostPlaneFunction);

public:
    FemPostPlaneFunction();
    ~FemPostPlaneFunction() override;

    App::PropertyVector Normal;
    App::PropertyVectorDistance Origin;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostPlaneFunction";
    }

protected:
    void onChanged(const App::Property* prop) override;
    /// get called after a document has been fully restored
    void onDocumentRestored() override;

    vtkSmartPointer<vtkPlane> m_plane;
};

// ---------------------------------------------------------------------------

class FemExport FemPostSphereFunction: public FemPostFunction
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostSphereFunction);

public:
    FemPostSphereFunction();
    ~FemPostSphereFunction() override;

    App::PropertyDistance Radius;
    App::PropertyVectorDistance Center;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemPostSphereFunction";
    }

protected:
    void onChanged(const App::Property* prop) override;

    vtkSmartPointer<vtkSphere> m_sphere;
};

}  // namespace Fem


#endif  // Fem_FemPostFunction_H
