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


#ifndef FEATURE_MESH_SEGMENT_H
#define FEATURE_MESH_SEGMENT_H

#include <App/PropertyStandard.h>

#include "MeshFeature.h"

#include <App/PropertyLinks.h>
#include <App/PropertyGeo.h>


namespace Mesh
{

/**
 * The SegmentByMesh class creates a mesh segment from two meshes whereat 
 * the facets of the first mesh that lie inside the second mesh build up the segment.  
 * @author Werner Mayer
 */
class SegmentByMesh : public Mesh::Feature
{
  PROPERTY_HEADER(Mesh::SegmentByMesh);

public:
  SegmentByMesh();

  App::PropertyLink Source;
  App::PropertyLink Tool;
  App::PropertyVector Base;
  App::PropertyVector Normal;


  /** @name methods overide Feature */
  //@{
  /// recalculate the Feature
  App::DocumentObjectExecReturn *execute(void);
  short mustExecute() const;
  //@}
};

}

#endif // FEATURE_MESH_SEGMENT_H 
