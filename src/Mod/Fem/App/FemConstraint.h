/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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


#ifndef FEM_CONSTRAINT_H
#define FEM_CONSTRAINT_H

#include <Base/Vector3D.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyGeo.h>

namespace Fem
{

class AppFemExport Constraint : public App::DocumentObject
{
    PROPERTY_HEADER(Fem::Constraint);

public:
    /// Constructor
    Constraint(void);
    virtual ~Constraint();

    App::PropertyLinkSubList References;
    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyVector NormalDirection;

    /// recalculate the object
    virtual App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderFemConstraint";
    }

protected:
    virtual void onChanged(const App::Property* prop);
    virtual void onDocumentRestored();

protected:
    /// Calculate the points where symbols should be drawn
    const bool getPoints(std::vector<Base::Vector3d>& points, std::vector<Base::Vector3d>& normals) const;
    const bool getCylinder(double& radius, double& height, Base::Vector3d& base, Base::Vector3d& axis) const;
    Base::Vector3d getBasePoint(const Base::Vector3d& base, const Base::Vector3d& axis,
                                const App::PropertyLinkSub &location, const double& dist);
    const Base::Vector3d getDirection(const App::PropertyLinkSub &direction);

};

} //namespace Fem


#endif // FEM_CONSTRAINT_H
