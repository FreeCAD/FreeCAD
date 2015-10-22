/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#ifndef MESH_INTERFACE_H
#define MESH_INTERFACE_H

#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Base/Vector3D.h>

#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#error
namespace Mesh
{

/** The Interface class is useful to convert between the MeshKernel and the OpenMesh data
 * structures. This class is implemented as template to accept any OpenMesh kernel type.
 * @author Werner Mayer
 */
template <class Kernel>
class Interface : public Kernel
{
public:
    Interface()
    {
    }

    Interface(const MeshCore::MeshKernel& kernel)
    {
        copy(kernel);
    }

    ~Interface()
    {
    }

    void copy(const MeshCore::MeshKernel& kernel_In)
    {
        this->clear();
        std::vector<Kernel::VertexHandle>  vertex_handles;
        vertex_handles.reserve(kernel_In.CountPoints());
        MeshCore::MeshPointIterator it(kernel_In);
        for (it.Init(); it.More(); it.Next()) {
            vertex_handles.push_back(this->add_vertex(Kernel::Point(it->x, it->y, it->z)));
        }

        const MeshCore::MeshFacetArray& ary = kernel_In.GetFacets();
        for (MeshCore::MeshFacetArray::_TConstIterator it = ary.begin(); it != ary.end(); ++it) {
            this->add_face(vertex_handles[it->_aulPoints[0]], vertex_handles[it->_aulPoints[1]], vertex_handles[it->_aulPoints[2]]);
        }
        vertex_handles.clear();
    }

    void release(MeshCore::MeshKernel& kernel_Out)
    {
        MeshCore::MeshFacetArray facets;
        MeshCore::MeshPointArray points;

        facets.reserve(this->n_faces());
        points.reserve(this->n_vertices());

        // get the points
        Kernel::ConstVertexIter v_it, v_end(this->vertices_end());
        for (v_it=this->vertices_begin(); v_it!=v_end; ++v_it) {
            Kernel::Point p = this->point(v_it);
            points.push_back(Base::Vector3f(p[0], p[1], p[2]));
        }

        // get the facets
        Kernel::ConstFaceIter f_it, f_end(this->faces_end());
        for (f_it=this->faces_begin(); f_it!=f_end; ++f_it) {
            MeshCore::MeshFacet face;
            int i=0;
            for (Kernel::ConstFaceVertexIter fv_it=this->cfv_iter(f_it); fv_it; ++fv_it) {
                face._aulPoints[i++] = fv_it.handle().idx();
            }

            facets.push_back(face);
        }
        // get the neighbourhood
        //
        // iterate over all faces
        for (f_it=this->faces_begin(); f_it!=f_end; ++f_it) {
            // iterate over the half edges to which belong the current face 
            for (Kernel::ConstFaceHalfedgeIter fh_it=this->cfh_iter(f_it); fh_it; ++fh_it) {
                // get the opposite half edge of the current half edge
                Kernel::HalfedgeHandle hh = this->opposite_halfedge_handle(fh_it);
                if (hh.is_valid()) {
                    // if the opposite half edge is valid a neighbour face must exist
                    Kernel::FaceHandle fh = this->face_handle(hh);
                    Kernel::VertexHandle vh = this->to_vertex_handle(hh);
                    for (int j=0; j<3; j++) {
                        // find the appropriate vertex and set the neighbour face
                        if (facets[f_it.handle().idx()]._aulPoints[j] == vh.idx()) {
                            facets[f_it.handle().idx()]._aulNeighbours[j] = fh.idx();
                            break;
                        }
                    }
                }
            }
        }

        this->clear();
        kernel_Out.Adopt(points, facets, false);
    }
};

} // namespace Mesh


#endif  // MESH_INTERFACE_H 
