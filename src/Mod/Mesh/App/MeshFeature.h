/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESH_FEATURE_H
#define MESH_FEATURE_H

#include <App/GeoFeature.h>
#include <App/FeaturePython.h>

#include "Core/MeshKernel.h"
#include "Mesh.h"
#include "MeshProperties.h"

namespace Base{
  class XMLReader;
  class Writer;
}

namespace MeshCore {
  class MeshKernel;
}

namespace Mesh
{

class Property;
class MeshFeaturePy;

/** Base class of all mesh feature classes in FreeCAD.
 * This class holds a MeshKernel object.
 * \author Werner Mayer
 */
class MeshExport Feature : public App::GeoFeature
{
    PROPERTY_HEADER(Mesh::Feature);

public:
    /// Constructor
    Feature(void);
    virtual ~Feature();

    /** @name Properties */
    //@{
    /** Property that holds the mesh data. */
    PropertyMeshKernel Mesh;
    //@}

    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "MeshGui::ViewProviderMeshFaceSet";
    }

    /// handles the MeshPy object
    virtual PyObject* getPyObject(void);
};

typedef App::FeaturePythonT<Feature> FeaturePython;

} //namespace Mesh



#endif 
