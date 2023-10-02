/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Facet.h"
#include "FacetPy.h"
#include "FacetPy.cpp"
#include "EdgePy.h"
#include "Mesh.h"


using namespace Mesh;

namespace
{
class Index
{
    FacetIndex index;

public:
    Index(FacetIndex index)
        : index {index}
    {}

    friend std::ostream& operator<<(std::ostream& os, Index idx)
    {
        if (idx.index < MeshCore::FACET_INDEX_MAX) {
            os << idx.index;
        }
        else {
            os << -1;
        }
        return os;
    }
};
}  // namespace

// returns a string which represent the object e.g. when printed in python
std::string FacetPy::representation() const
{
    // clang-format off
    FacetPy::PointerType ptr = getFacetPtr();
    std::stringstream str;
    str << "Facet (";
    if (ptr->isBound()) {
        str << "(" << ptr->_aclPoints[0].x << ", "
                   << ptr->_aclPoints[0].y << ", "
                   << ptr->_aclPoints[0].z << ", Idx="
                   << ptr->PIndex[0] << "), ";
        str << "(" << ptr->_aclPoints[1].x << ", "
                   << ptr->_aclPoints[1].y << ", "
                   << ptr->_aclPoints[1].z << ", Idx="
                   << ptr->PIndex[1] << "), ";
        str << "(" << ptr->_aclPoints[2].x << ", "
                   << ptr->_aclPoints[2].y << ", "
                   << ptr->_aclPoints[2].z << ", Idx="
                   << ptr->PIndex[2] << "), ";
        str << "Idx=" << Index(ptr->Index) << ", ("
                      << Index(ptr->NIndex[0]) << ", "
                      << Index(ptr->NIndex[1]) << ", "
                      << Index(ptr->NIndex[2]) << ")";
    }
    else {
        str << "(" << ptr->_aclPoints[0].x << ", "
                   << ptr->_aclPoints[0].y << ", "
                   << ptr->_aclPoints[0].z << "), ";
        str << "(" << ptr->_aclPoints[1].x << ", "
                   << ptr->_aclPoints[1].y << ", "
                   << ptr->_aclPoints[1].z << "), ";
        str << "(" << ptr->_aclPoints[2].x << ", "
                   << ptr->_aclPoints[2].y << ", "
                   << ptr->_aclPoints[2].z << ")";
    }
    str << ")";

    return str.str();
    // clang-format on
}

PyObject* FacetPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of FacetPy and the Twin object
    return new FacetPy(new Facet);
}

// constructor method
int FacetPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (!PyArg_ParseTuple(args, "")) {
        return -1;
    }
    return 0;
}

PyObject* FacetPy::unbound(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    getFacetPtr()->Index = MeshCore::FACET_INDEX_MAX;
    getFacetPtr()->Mesh = nullptr;
    Py_Return;
}

PyObject* FacetPy::getEdge(PyObject* args)
{
    int index {};
    if (!PyArg_ParseTuple(args, "i", &index)) {
        return nullptr;
    }

    Edge edge = getFacetPtr()->getEdge(index);
    return new EdgePy(new Edge(edge));
}

Py::Long FacetPy::getIndex() const
{
    return Py::Long((long)getFacetPtr()->Index);
}

Py::Boolean FacetPy::getBound() const
{
    return {getFacetPtr()->isBound()};
}

Py::Object FacetPy::getNormal() const
{
    Base::VectorPy* normal = new Base::VectorPy(getFacetPtr()->GetNormal());
    normal->setConst();
    return Py::Object(normal, true);
}

PyObject* FacetPy::intersect(PyObject* args)
{
    PyObject* object {};
    if (!PyArg_ParseTuple(args, "O!", &FacetPy::Type, &object)) {
        return nullptr;
    }
    FacetPy* face = static_cast<FacetPy*>(object);
    FacetPy::PointerType face_ptr = face->getFacetPtr();
    FacetPy::PointerType this_ptr = this->getFacetPtr();
    Base::Vector3f p0, p1;
    int ret = this_ptr->IntersectWithFacet(*face_ptr, p0, p1);

    try {
        Py::List sct;

        if (ret > 0) {
            Py::Tuple pt(3);
            pt.setItem(0, Py::Float(p0.x));
            pt.setItem(1, Py::Float(p0.y));
            pt.setItem(2, Py::Float(p0.z));
            sct.append(pt);
        }
        if (ret > 1) {
            Py::Tuple pt(3);
            pt.setItem(0, Py::Float(p1.x));
            pt.setItem(1, Py::Float(p1.y));
            pt.setItem(2, Py::Float(p1.z));
            sct.append(pt);
        }

        return Py::new_reference_to(sct);
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject* FacetPy::isDegenerated(PyObject* args)
{
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "|f", &fEpsilon)) {
        return nullptr;
    }

    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        throw Py::RuntimeError("Unbound facet");
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::new_reference_to(Py::Boolean(tria.IsDegenerated(fEpsilon)));
}

PyObject* FacetPy::isDeformed(PyObject* args)
{
    float fMinAngle {};
    float fMaxAngle {};
    if (!PyArg_ParseTuple(args, "ff", &fMinAngle, &fMaxAngle)) {
        return nullptr;
    }

    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        throw Py::RuntimeError("Unbound facet");
    }

    float fCosOfMinAngle = cos(fMinAngle);
    float fCosOfMaxAngle = cos(fMaxAngle);
    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::new_reference_to(Py::Boolean(tria.IsDeformed(fCosOfMinAngle, fCosOfMaxAngle)));
}

Py::List FacetPy::getPoints() const
{
    FacetPy::PointerType face = this->getFacetPtr();

    Py::List pts;
    for (const auto& vec : face->_aclPoints) {
        Py::Tuple pt(3);
        pt.setItem(0, Py::Float(vec.x));
        pt.setItem(1, Py::Float(vec.y));
        pt.setItem(2, Py::Float(vec.z));
        pts.append(pt);
    }

    return pts;
}

Py::Tuple FacetPy::getPointIndices() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Tuple();
    }

    Py::Tuple idxTuple(3);
    for (int i = 0; i < 3; i++) {
        idxTuple.setItem(i, Py::Long(face->PIndex[i]));
    }
    return idxTuple;
}

Py::Tuple FacetPy::getNeighbourIndices() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Tuple();
    }

    Py::Tuple idxTuple(3);
    for (int i = 0; i < 3; i++) {
        auto index = face->NIndex[i];
        if (index < MeshCore::FACET_INDEX_MAX) {
            idxTuple.setItem(i, Py::Long(index));
        }
        else {
            idxTuple.setItem(i, Py::Long(-1L));
        }
    }
    return idxTuple;
}

Py::Float FacetPy::getArea() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Float(0.0);
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::Float(tria.Area());
}

Py::Float FacetPy::getAspectRatio() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Float(-1.0);
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::Float(tria.AspectRatio());
}

Py::Float FacetPy::getAspectRatio2() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Float(-1.0);
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::Float(tria.AspectRatio2());
}

Py::Float FacetPy::getRoundness() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::Float(-1.0);
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    return Py::Float(tria.Roundness());
}

Py::Tuple FacetPy::getCircumCircle() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::None();
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    Base::Vector3f center;
    float radius = tria.CenterOfCircumCircle(center);
    Py::Tuple tuple(2);
    tuple.setItem(0, Py::Vector(center));
    tuple.setItem(1, Py::Float(radius));
    return tuple;
}

Py::Tuple FacetPy::getInCircle() const
{
    FacetPy::PointerType face = this->getFacetPtr();
    if (!face->isBound()) {
        return Py::None();
    }

    const MeshCore::MeshKernel& kernel = face->Mesh->getKernel();
    MeshCore::MeshGeomFacet tria = kernel.GetFacet(face->Index);
    Base::Vector3f center;
    float radius = tria.CenterOfInscribedCircle(center);
    Py::Tuple tuple(2);
    tuple.setItem(0, Py::Vector(center));
    tuple.setItem(1, Py::Float(radius));
    return tuple;
}

PyObject* FacetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FacetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
