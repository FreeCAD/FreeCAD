/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef FeatureMeshCurvature_H
#define FeatureMeshCurvature_H

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyGeo.h>

#include "Mesh.h"
#include "MeshProperties.h"

namespace Mesh
{

/**
 * The Curvature class calculates the curvature values of a related mesh feature.
 * @author Werner Mayer
 */
class MeshExport Curvature : public App::DocumentObject
{
    PROPERTY_HEADER(Mesh::Curvature);

public:
    Curvature();

    App::PropertyLink Source;
    PropertyCurvatureList CurvInfo;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute();
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const {
        return "MeshGui::ViewProviderMeshCurvature"; 
    }
  //@}
};

}

#endif // Curvature_H 
