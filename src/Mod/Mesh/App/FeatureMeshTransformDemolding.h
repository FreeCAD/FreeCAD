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


#ifndef FEATURE_MESH_TRANSFORM_DEMOLDING_H
#define FEATURE_MESH_TRANSFORM_DEMOLDING_H

#include "FeatureMeshTransform.h"

#include <App/PropertyLinks.h>
#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>


namespace Mesh
{

/**
 * The TransformDemolding class.
 */
class TransformDemolding : public Transform
{
    PROPERTY_HEADER(Mesh::TransformDemolding);

public:
    TransformDemolding();

    App::PropertyLink Source;
    App::PropertyFloat Rotation;
    App::PropertyVector Axis;

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "MeshGui::ViewProviderMeshTransformDemolding";
    }
    //@}
};

}

#endif // FEATURE_MESH_TRANSFORM_H 
