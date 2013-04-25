/*
    openDCM, dimensional constraint manager
    Copyright (C) 2012  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GCM_GEOMETRY_PART_H
#define GCM_GEOMETRY_PART_H

#include <opendcm/core/geometry.hpp>
#include <opendcm/core/kernel.hpp>

namespace dcm {
namespace tag {

struct part  {};

}

namespace modell {
  
  struct quaternion_wxyz_vec3 {
    /*Modell XYZ: 
     * 0 = w;
     * 1 = x;
     * 2 = y;
     * 3 = z;
     */    
    template<typename Kernel, typename Accessor, typename Type>
    void extract(Type& t, typename Kernel::Transform3D& trans) {
      
      typedef typename Kernel::number_type Scalar;
      typedef typename Kernel::Transform3D::Rotation 	Rotation;
      typedef typename Kernel::Transform3D::Translation Translation;
      
      Accessor a;
      Rotation r;
      r.w() = a.template get<Scalar, 0>(t);
      r.x() = a.template get<Scalar, 1>(t);
      r.y() = a.template get<Scalar, 2>(t);
      r.z() = a.template get<Scalar, 3>(t);
      
      Translation tr;;
      tr.vector()(0) = a.template get<Scalar, 4>(t);
      tr.vector()(1) = a.template get<Scalar, 5>(t);
      tr.vector()(2) = a.template get<Scalar, 6>(t);
      
      trans =  r;
      trans *= tr;      
    }
    
    template<typename Kernel, typename Accessor, typename Type>
    void inject(Type& t, typename Kernel::Transform3D& trans) {
      
      typedef typename Kernel::number_type Scalar;
      typedef typename Kernel::Transform3D::Rotation 	Rotation;
      typedef typename Kernel::Transform3D::Translation Translation;
      
      Accessor a;
      
      const Rotation& r = trans.rotation();
      a.template set<Scalar, 0>(r.w(), t);
      a.template set<Scalar, 1>(r.x(), t);
      a.template set<Scalar, 2>(r.y(), t);
      a.template set<Scalar, 3>(r.z(), t);
      
      const Translation& tr = trans.translation();
      a.template set<Scalar, 4>(tr.vector()(0), t);
      a.template set<Scalar, 5>(tr.vector()(1), t);
      a.template set<Scalar, 6>(tr.vector()(2), t);
      
      a.finalize(t);
    };
  };
}

}

#endif //GCM_GEOMETRY_PART_H
