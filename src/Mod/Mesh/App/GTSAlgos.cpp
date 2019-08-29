/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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


#include "PreCompiled.h"
#ifndef _PreComp_
# ifdef FC_OS_LINUX
#	  include <unistd.h>
# endif
#endif


#include "GTSAlgos.h"
#include "Mesh.h"

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include "Core/Iterator.h"
#include "Core/Algorithm.h"
#include "Core/TopoAlgorithm.h"
#include "Core/Evaluation.h"

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

using namespace Mesh;
using namespace MeshCore;



void GTSAlgos::coarsen(float f)
{
  GtsSurface * surface;

  // create a GTS surface
  surface = GTSAlgos::createGTSSurface(_Mesh);


  guint stop_number=100000;
  gdouble fold = 3.1415 / 180.;

  try{
    gts_surface_coarsen (surface, 
                         NULL, NULL, 
                         NULL, NULL, 
                         (GtsStopFunc)gts_coarsen_stop_number,
                         &stop_number, fold);
  } catch (...)
  {
    gts_object_destroy (GTS_OBJECT (surface));
    throw Base::RuntimeError("Unknown error in GTSAlgos::coarsen()");
  }

  // get the standard mesh
  _Mesh.getKernel().Clear();
  fillMeshFromGTSSurface(_Mesh,surface);
  gts_object_destroy (GTS_OBJECT (surface));
}


void GTSAlgos::boolean(const Mesh::MeshObject& ToolMesh, int Type)
{
  GtsSurface * s1, * s2, * s3;
  GtsSurfaceInter * si;
  GNode * tree1, * tree2;
  gboolean check_self_intersection = FALSE;
  gboolean closed = TRUE, is_open1, is_open2;


  // create a GTS surface
  s1 = GTSAlgos::createGTSSurface(_Mesh);
  s2 = GTSAlgos::createGTSSurface(ToolMesh);

  // clear the mesh (memory)
  //Mesh1.clear();
  //Mesh2.clear();

  // check that the surfaces are orientable manifolds
  if (!gts_surface_is_orientable (s1)) {
    gts_object_destroy (GTS_OBJECT (s1));
    gts_object_destroy (GTS_OBJECT (s2));
    throw Base::RuntimeError("surface 1 is not an orientable manifold\n");
  }
  if (!gts_surface_is_orientable (s2)) {
    gts_object_destroy (GTS_OBJECT (s1));
    gts_object_destroy (GTS_OBJECT (s2));
    throw Base::RuntimeError("surface 2 is not an orientable manifold\n");
  }

  // check that the surfaces are not self-intersecting
  if (check_self_intersection) {
    GtsSurface * self_intersects;

    self_intersects = gts_surface_is_self_intersecting (s1);
    if (self_intersects != NULL) {
//      if (verbose)
//	      gts_surface_print_stats (self_intersects, stderr);
//      gts_surface_write (self_intersects, stdout);
      gts_object_destroy (GTS_OBJECT (self_intersects));
      gts_object_destroy (GTS_OBJECT (s1));
      gts_object_destroy (GTS_OBJECT (s2));
      throw Base::RuntimeError("surface is self-intersecting\n");
    }
    self_intersects = gts_surface_is_self_intersecting (s2);
    if (self_intersects != NULL) {
//      if (verbose)
//	      gts_surface_print_stats (self_intersects, stderr);
//      gts_surface_write (self_intersects, stdout);
      gts_object_destroy (GTS_OBJECT (self_intersects));
      gts_object_destroy (GTS_OBJECT (s1));
      gts_object_destroy (GTS_OBJECT (s2));
      throw Base::RuntimeError("surface is self-intersecting\n");
    }
  }

  // build bounding box tree for first surface
  tree1 = gts_bb_tree_surface (s1);
  is_open1 = gts_surface_volume (s1) < 0. ? TRUE : FALSE;

  // build bounding box tree for second surface 
  tree2 = gts_bb_tree_surface (s2);
  is_open2 = gts_surface_volume (s2) < 0. ? TRUE : FALSE;

  si = gts_surface_inter_new (gts_surface_inter_class (),
			      s1, s2, tree1, tree2, is_open1, is_open2);
  g_assert (gts_surface_inter_check (si, &closed));
  if (!closed) {
    gts_object_destroy (GTS_OBJECT (s1));
    gts_object_destroy (GTS_OBJECT (s2));
    gts_bb_tree_destroy (tree1, TRUE);
    gts_bb_tree_destroy (tree2, TRUE);  
    throw Base::RuntimeError("the intersection of 1 and  2 is not a closed curve\n");
  }

  s3 = gts_surface_new (gts_surface_class (),
			gts_face_class (),
			gts_edge_class (),
			gts_vertex_class ());  
  if (Type==0) { // union
    gts_surface_inter_boolean (si, s3, GTS_1_OUT_2);
    gts_surface_inter_boolean (si, s3, GTS_2_OUT_1);
  }
  else if (Type==1) { // inter
    gts_surface_inter_boolean (si, s3, GTS_1_IN_2);
    gts_surface_inter_boolean (si, s3, GTS_2_IN_1);
  }
  else if (Type==2) { //diff
    gts_surface_inter_boolean (si, s3, GTS_1_OUT_2);
    gts_surface_inter_boolean (si, s3, GTS_2_IN_1);
    gts_surface_foreach_face (si->s2, (GtsFunc) gts_triangle_revert, NULL);
    gts_surface_foreach_face (s2, (GtsFunc) gts_triangle_revert, NULL);
  }
  else if (Type==3) { // cut inner
    gts_surface_inter_boolean (si, s3, GTS_1_IN_2);
  }
  else if (Type==4) { // cut outer
    gts_surface_inter_boolean (si, s3, GTS_1_OUT_2);
  }
   
  // check that the resulting surface is not self-intersecting
  if (check_self_intersection) {
    GtsSurface * self_intersects;

    self_intersects = gts_surface_is_self_intersecting (s3);
    if (self_intersects != NULL) {
 //      if (verbose)
 //	      gts_surface_print_stats (self_intersects, stderr);
 //     gts_surface_write (self_intersects, stdout);
      gts_object_destroy (GTS_OBJECT (self_intersects));
      gts_object_destroy (GTS_OBJECT (s1));
      gts_object_destroy (GTS_OBJECT (s2));
      gts_object_destroy (GTS_OBJECT (s3));
      gts_object_destroy (GTS_OBJECT (si));
      gts_bb_tree_destroy (tree1, TRUE);
      gts_bb_tree_destroy (tree2, TRUE);
      throw Base::RuntimeError("the resulting surface is self-intersecting\n");
    }
  }
  // display summary information about the resulting surface
  //  if (verbose)
  //    gts_surface_print_stats (s3, stderr);
  // write resulting surface to standard output

  // get the standard mesh
  _Mesh.clear();
  fillMeshFromGTSSurface(_Mesh,s3);


  // destroy surfaces 
  gts_object_destroy (GTS_OBJECT (s1));
  gts_object_destroy (GTS_OBJECT (s2));
  gts_object_destroy (GTS_OBJECT (s3));
  gts_object_destroy (GTS_OBJECT (si));

  // destroy bounding box trees (including bounding boxes)
  gts_bb_tree_destroy (tree1, TRUE);
  gts_bb_tree_destroy (tree2, TRUE);
  

}





/// helper function - construct a Edge out of two Vertexes if not already there
static GtsEdge * new_edge (GtsVertex * v1, GtsVertex * v2)
{
  GtsSegment * s = gts_vertices_are_connected (v1, v2);
  if( s == NULL ) 
    return gts_edge_new (gts_edge_class (), v1, v2);
  else
    return GTS_EDGE (s);
}


GtsSurface* GTSAlgos::createGTSSurface(const Mesh::MeshObject& Mesh)
{
  GtsSurface* Surf = gts_surface_new (gts_surface_class (),
                                      gts_face_class (),
                                      gts_edge_class (),
                                      gts_vertex_class () );

  unsigned long p1,p2,p3;
  Base::Vector3f Vertex;


  // Getting all the points
  GtsVertex ** aVertex = (GtsVertex **) malloc(Mesh.getKernel().CountPoints() * sizeof (GtsVertex *));
  for (unsigned int PIter = 0;PIter < Mesh.getKernel().CountPoints(); PIter++)
  {
    // Transform the point in the global coordinate system
    Vertex = Mesh.getTransform() * Mesh.getKernel().GetPoint(PIter);

    aVertex[PIter] = gts_vertex_new (gts_vertex_class (), Vertex.x, Vertex.y, Vertex.z);
  }

    // cycling through the facets
  for (unsigned int pFIter = 0;pFIter < Mesh.getKernel().CountFacets(); pFIter++)
  {
    // getting the three points of the facet
    Mesh.getKernel().GetFacetPoints(pFIter,p1,p2,p3);
    
    // creating the edges and add the face to the surface
    gts_surface_add_face (Surf, 
  	    gts_face_new (Surf->face_class,
          new_edge (aVertex[p1],aVertex[p2]),
          new_edge (aVertex[p2],aVertex[p3]),
          new_edge (aVertex[p3],aVertex[p1])));
  }

  Base::Console().Log("GTS [%d faces, %d Points, %d Edges,%s ,%s]\n",gts_surface_face_number(Surf),
                                                                     gts_surface_vertex_number(Surf),
                                                                     gts_surface_edge_number(Surf),
                                                                     gts_surface_is_orientable (Surf)?"orientable":"not orientable",
                                                                     gts_surface_is_self_intersecting(Surf)?"self-intersections":"no self-intersection" );

  return Surf;
}

/// helper function for the face (triangle iteration
static void onFaces (GtsTriangle * t,  std::vector<MeshGeomFacet> *VAry )
{
  GtsVertex *mv0,*mv1,*mv2;

  gts_triangle_vertices (t,&mv0,&mv1,&mv2);

  VAry->push_back(MeshGeomFacet(Base::Vector3f(mv0->p.x,mv0->p.y,mv0->p.z),
                                Base::Vector3f(mv1->p.x,mv1->p.y,mv1->p.z),
                                Base::Vector3f(mv2->p.x,mv2->p.y,mv2->p.z)));

}


void GTSAlgos::fillMeshFromGTSSurface(Mesh::MeshObject& Mesh, GtsSurface* pSurface)
{
  std::vector<MeshGeomFacet> VAry;

  // remove old mesh
  Mesh.getKernel().Clear();

//  gts_surface_foreach_vertex(pSurface,(GtsFunc) onVertices,&MeshK);
  gts_surface_foreach_face (pSurface, (GtsFunc) onFaces,&VAry);

  // destroy surfaces
  //gts_object_destroy (GTS_OBJECT (pSurface));

  // put the facets the simple way in the mesh, totp is recalculated!
  Mesh.getKernel() = VAry;

}

