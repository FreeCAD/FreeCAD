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

#include <Base/VectorPy.h>
#include <Base/Handle.h>
#include <Base/Builder3D.h>
#include <Base/Converter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/MatrixPy.h>
#include <Base/Stream.h>
#include <Base/Tools.h>

#include "Mesh.h"
#include "MeshPy.h"
#include "MeshPointPy.h"
#include "FacetPy.h"
#include "MeshPy.cpp"
#include "MeshProperties.h"
#include "Core/Algorithm.h"
#include "Core/Triangulation.h"
#include "Core/Iterator.h"
#include "Core/Degeneration.h"
#include "Core/Elements.h"
#include "Core/Grid.h"
#include "Core/MeshKernel.h"
#include "Core/Segmentation.h"
#include "Core/Smoothing.h"
#include "Core/Curvature.h"

#include <boost/algorithm/string.hpp>

using namespace Mesh;


struct MeshPropertyLock {
    MeshPropertyLock(PropertyMeshKernel* p) : prop(p)
    { if (prop) prop->startEditing(); }
    ~MeshPropertyLock()
    { if (prop) prop->finishEditing(); }
private:
    PropertyMeshKernel* prop;
};

int MeshPy::PyInit(PyObject* args, PyObject*)
{
    PyObject *pcObj=nullptr;
    if (!PyArg_ParseTuple(args, "|O", &pcObj))
        return -1;

    try {
        this->parentProperty = nullptr;
        // if no mesh is given
        if (!pcObj)
            return 0;
        if (PyObject_TypeCheck(pcObj, &(MeshPy::Type))) {
            getMeshObjectPtr()->operator = (*static_cast<MeshPy*>(pcObj)->getMeshObjectPtr());
        }
        else if (PyList_Check(pcObj)) {
            PyObject* ret = addFacets(args);
            bool ok = (ret!=nullptr);
            Py_XDECREF(ret);
            if (!ok)
                return -1;
        }
        else if (PyTuple_Check(pcObj)) {
            PyObject* ret = addFacets(args);
            bool ok = (ret!=nullptr);
            Py_XDECREF(ret);
            if (!ok)
                return -1;
        }
        else if (PyUnicode_Check(pcObj)) {
            getMeshObjectPtr()->load(PyUnicode_AsUTF8(pcObj));
        }
        else {
            PyErr_Format(PyExc_TypeError, "Cannot create a mesh out of a '%s'",
                pcObj->ob_type->tp_name);
            return -1;
        }
    }
    catch (const Base::Exception &e) {
        e.setPyException();
        return -1;
    }
    catch (const std::exception &e) {
        PyErr_SetString(Base::PyExc_FC_GeneralError,e.what());
        return -1;
    }
    catch (const Py::Exception&) {
        return -1;
    }

    return 0;
}

// returns a string which represent the object e.g. when printed in python
std::string MeshPy::representation() const
{
    return getMeshObjectPtr()->representation();
}

PyObject *MeshPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MeshPy and the Twin object 
    return new MeshPy(new MeshObject);
}

PyObject* MeshPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    return new MeshPy(new MeshObject(*getMeshObjectPtr()));
}

PyObject*  MeshPy::read(PyObject *args, PyObject *kwds)
{
    char* Name;
    static char* keywords_path[] = {"Filename",nullptr};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "et", keywords_path, "utf-8", &Name)) {
        getMeshObjectPtr()->load(Name);
        PyMem_Free(Name);
        Py_Return;
    }

    PyErr_Clear();

    MeshCore::MeshIO::Format format = MeshCore::MeshIO::Undefined;
    std::map<std::string, MeshCore::MeshIO::Format> ext;
    ext["BMS" ] = MeshCore::MeshIO::BMS;
    ext["STL" ] = MeshCore::MeshIO::BSTL;
    ext["AST" ] = MeshCore::MeshIO::ASTL;
    ext["OBJ" ] = MeshCore::MeshIO::OBJ;
    ext["SMF" ] = MeshCore::MeshIO::SMF;
    ext["OFF" ] = MeshCore::MeshIO::OFF;
    ext["IV"  ] = MeshCore::MeshIO::IV;
    ext["X3D" ] = MeshCore::MeshIO::X3D;
    ext["X3DZ"] = MeshCore::MeshIO::X3DZ;
    ext["VRML"] = MeshCore::MeshIO::VRML;
    ext["WRL" ] = MeshCore::MeshIO::VRML;
    ext["WRZ" ] = MeshCore::MeshIO::WRZ;
    ext["NAS" ] = MeshCore::MeshIO::NAS;
    ext["BDF" ] = MeshCore::MeshIO::NAS;
    ext["PLY" ] = MeshCore::MeshIO::PLY;
    ext["APLY"] = MeshCore::MeshIO::APLY;
    ext["PY"  ] = MeshCore::MeshIO::PY;

    PyObject* input;
    char* Ext;
    static char* keywords_stream[] = {"Stream","Format",nullptr};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "Os",keywords_stream, &input, &Ext)) {
        std::string fmt(Ext);
        boost::to_upper(fmt);
        if (ext.find(fmt) != ext.end()) {
            format = ext[fmt];
        }

        // read mesh
        Base::PyStreambuf buf(input);
        std::istream str(nullptr);
        str.rdbuf(&buf);
        getMeshObjectPtr()->load(str, format);

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return nullptr;
}

PyObject*  MeshPy::write(PyObject *args, PyObject *kwds)
{
    char* Name;
    char* Ext=nullptr;
    char* ObjName=nullptr;
    PyObject* List=nullptr;

    MeshCore::MeshIO::Format format = MeshCore::MeshIO::Undefined;
    std::map<std::string, MeshCore::MeshIO::Format> ext;
    ext["BMS"  ] = MeshCore::MeshIO::BMS;
    ext["STL"  ] = MeshCore::MeshIO::BSTL;
    ext["AST"  ] = MeshCore::MeshIO::ASTL;
    ext["OBJ"  ] = MeshCore::MeshIO::OBJ;
    ext["SMF"  ] = MeshCore::MeshIO::SMF;
    ext["OFF"  ] = MeshCore::MeshIO::OFF;
    ext["IDTF" ] = MeshCore::MeshIO::IDTF;
    ext["MGL"  ] = MeshCore::MeshIO::MGL;
    ext["IV"   ] = MeshCore::MeshIO::IV;
    ext["X3D"  ] = MeshCore::MeshIO::X3D;
    ext["X3DZ" ] = MeshCore::MeshIO::X3DZ;
    ext["X3DOM"] = MeshCore::MeshIO::X3DOM;
    ext["VRML" ] = MeshCore::MeshIO::VRML;
    ext["WRL"  ] = MeshCore::MeshIO::VRML;
    ext["WRZ"  ] = MeshCore::MeshIO::WRZ;
    ext["NAS"  ] = MeshCore::MeshIO::NAS;
    ext["BDF"  ] = MeshCore::MeshIO::NAS;
    ext["PLY"  ] = MeshCore::MeshIO::PLY;
    ext["APLY" ] = MeshCore::MeshIO::APLY;
    ext["PY"   ] = MeshCore::MeshIO::PY;
    ext["ASY"  ] = MeshCore::MeshIO::ASY;
    ext["3MF"  ] = MeshCore::MeshIO::ThreeMF;

    static char* keywords_path[] = {"Filename","Format","Name","Material",nullptr};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "et|ssO", keywords_path, "utf-8",
                                    &Name, &Ext, &ObjName, &List)) {
        if (Ext) {
            std::string fmt(Ext);
            boost::to_upper(fmt);
            if (ext.find(fmt) != ext.end()) {
                format = ext[fmt];
            }
        }

        if (List) {
            MeshCore::Material mat;
            Py::Sequence list(List);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::Tuple t(*it);
                float r = (float)Py::Float(t.getItem(0));
                float g = (float)Py::Float(t.getItem(1));
                float b = (float)Py::Float(t.getItem(2));
                mat.diffuseColor.emplace_back(r,g,b);
            }

            if (mat.diffuseColor.size() == getMeshObjectPtr()->countPoints())
                mat.binding = MeshCore::MeshIO::PER_VERTEX;
            else if (mat.diffuseColor.size() == getMeshObjectPtr()->countFacets())
                mat.binding = MeshCore::MeshIO::PER_FACE;
            else
                mat.binding = MeshCore::MeshIO::OVERALL;
            getMeshObjectPtr()->save(Name, format, &mat, ObjName);
        }
        else {
            getMeshObjectPtr()->save(Name, format, nullptr, ObjName);
        }

        PyMem_Free(Name);
        Py_Return;
    }

    PyErr_Clear();

    static char* keywords_stream[] = {"Stream","Format","Name","Material",nullptr};
    PyObject* input;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "Os|sO", keywords_stream,
                                    &input, &Ext, &ObjName, &List)) {
        std::string fmt(Ext);
        boost::to_upper(fmt);
        if (ext.find(fmt) != ext.end()) {
            format = ext[fmt];
        }

        std::unique_ptr<MeshCore::Material> mat;
        if (List) {
            mat.reset(new MeshCore::Material);
            Py::Sequence list(List);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::Tuple t(*it);
                float r = (float)Py::Float(t.getItem(0));
                float g = (float)Py::Float(t.getItem(1));
                float b = (float)Py::Float(t.getItem(2));
                mat->diffuseColor.emplace_back(r,g,b);
            }

            if (mat->diffuseColor.size() == getMeshObjectPtr()->countPoints())
                mat->binding = MeshCore::MeshIO::PER_VERTEX;
            else if (mat->diffuseColor.size() == getMeshObjectPtr()->countFacets())
                mat->binding = MeshCore::MeshIO::PER_FACE;
            else
                mat->binding = MeshCore::MeshIO::OVERALL;
        }

        // write mesh
        Base::PyStreambuf buf(input);
        std::ostream str(nullptr);
        str.rdbuf(&buf);
        getMeshObjectPtr()->save(str, format, mat.get(), ObjName);

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return nullptr;
}

PyObject*  MeshPy::writeInventor(PyObject *args)
{
    float creaseangle=0.0f;
    if (!PyArg_ParseTuple(args, "|f",&creaseangle))
        return nullptr;

    MeshObject* mesh = getMeshObjectPtr();
    const MeshCore::MeshFacetArray& faces = mesh->getKernel().GetFacets();
    std::vector<int> indices;
    std::vector<Base::Vector3f> coords;
    coords.reserve(mesh->countPoints());
    for (MeshObject::const_point_iterator it = mesh->points_begin(); it != mesh->points_end(); ++it)
        coords.emplace_back((float)it->x,(float)it->y,(float)it->z);
    indices.reserve(4*faces.size());
    for (MeshCore::MeshFacetArray::_TConstIterator it = faces.begin(); it != faces.end(); ++it) {
        indices.push_back(it->_aulPoints[0]);
        indices.push_back(it->_aulPoints[1]);
        indices.push_back(it->_aulPoints[2]);
        indices.push_back(-1);
    }

    std::stringstream result;
    Base::InventorBuilder builder(result);
    builder.beginSeparator();
    builder.addShapeHints(creaseangle);
    builder.beginPoints();
    builder.addPoints(coords);
    builder.endPoints();
    builder.addIndexedFaceSet(indices);
    builder.endSeparator();
    builder.close();

    return Py::new_reference_to(Py::String(result.str()));
}

PyObject*  MeshPy::offset(PyObject *args)
{
    float Float;
    if (!PyArg_ParseTuple(args, "f",&Float))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->offsetSpecial2(Float);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::offsetSpecial(PyObject *args)
{
    float Float,zmin,zmax;
    if (!PyArg_ParseTuple(args, "fff",&Float,&zmin,&zmax))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->offsetSpecial(Float,zmax,zmin);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::crossSections(PyObject *args)
{
    PyObject *obj;
    PyObject *poly=Py_False;
    float min_eps = 1.0e-2f;
    if (!PyArg_ParseTuple(args, "O|fO!", &obj, &min_eps, &PyBool_Type, &poly))
        return nullptr;

    Py::Sequence list(obj);
    Py::Type vType(Base::getTypeAsObject(&Base::VectorPy::Type));

    std::vector<MeshObject::TPlane> csPlanes;
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Py::Tuple pair(*it);
        Py::Object p1 = pair.getItem(0);
        Py::Object p2 = pair.getItem(1);
        if (p1.isType(vType) && p2.isType(vType)) {
            MeshObject::TPlane plane;
            Base::Vector3d b = static_cast<Base::VectorPy*>(p1.ptr())->value();
            Base::Vector3d n = static_cast<Base::VectorPy*>(p2.ptr())->value();
            plane.first.Set((float)b.x,(float)b.y,(float)b.z);
            plane.second.Set((float)n.x,(float)n.y,(float)n.z);
            csPlanes.push_back(plane);
        }
        else if (p1.isTuple() && p2.isTuple()) {
            Py::Tuple b(p1);
            Py::Tuple n(p2);
            float bx = (float)Py::Float(b.getItem(0));
            float by = (float)Py::Float(b.getItem(1));
            float bz = (float)Py::Float(b.getItem(2));
            float nx = (float)Py::Float(n.getItem(0));
            float ny = (float)Py::Float(n.getItem(1));
            float nz = (float)Py::Float(n.getItem(2));

            MeshObject::TPlane plane;
            plane.first .Set(bx,by,bz);
            plane.second.Set(nx,ny,nz);
            csPlanes.push_back(plane);
        }
    }

    std::vector<MeshObject::TPolylines> sections;
    getMeshObjectPtr()->crossSections(csPlanes, sections, min_eps, PyObject_IsTrue(poly) ? true : false);

    // convert to Python objects
    Py::List crossSections;
    for (std::vector<MeshObject::TPolylines>::iterator it = sections.begin(); it != sections.end(); ++it) {
        Py::List section;
        for (MeshObject::TPolylines::const_iterator jt = it->begin(); jt != it->end(); ++jt) {
            Py::List polyline;
            for (std::vector<Base::Vector3f>::const_iterator kt = jt->begin(); kt != jt->end(); ++kt) {
                polyline.append(Py::asObject(new Base::VectorPy(*kt)));
            }
            section.append(polyline);
        }
        crossSections.append(section);
    }

    return Py::new_reference_to(crossSections);
}

PyObject*  MeshPy::unite(PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return nullptr;

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->unite(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::intersect(PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return nullptr;

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->intersect(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::difference(PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return nullptr;

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->subtract(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::inner(PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return nullptr;

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->inner(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::outer(PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return nullptr;

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->outer(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::section(PyObject *args, PyObject *kwds)
{
    PyObject *pcObj;
    PyObject *connectLines = Py_True;
    float fMinDist = 0.0001f;

    static char* keywords_section[] = {"Mesh", "ConnectLines", "MinDist", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!f",keywords_section,
                                     &(MeshPy::Type), &pcObj, &PyBool_Type, &connectLines, &fMinDist))
        return nullptr;

    MeshPy* pcObject = static_cast<MeshPy*>(pcObj);

    std::vector< std::vector<Base::Vector3f> > curves = getMeshObjectPtr()->section(*pcObject->getMeshObjectPtr(), PyObject_IsTrue(connectLines) ? true : false, fMinDist);
    Py::List outer;
    for (const auto& it : curves) {
        Py::List inner;
        for (const auto& jt : it) {
            inner.append(Py::Vector(jt));
        }
        outer.append(inner);
    }

    return Py::new_reference_to(outer);
}

PyObject*  MeshPy::coarsen(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject*  MeshPy::translate(PyObject *args)
{
    float x,y,z;
    if (!PyArg_ParseTuple(args, "fff",&x,&y,&z))
        return nullptr;

    PY_TRY {
        Base::Matrix4D m;
        m.move(x,y,z);
        getMeshObjectPtr()->getKernel().Transform(m);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::rotate(PyObject *args)
{
    double x,y,z;
    if (!PyArg_ParseTuple(args, "ddd",&x,&y,&z))
        return nullptr;

    PY_TRY {
        Base::Matrix4D m;
        m.rotX(x);
        m.rotY(y);
        m.rotZ(z);
        getMeshObjectPtr()->getKernel().Transform(m);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::transform(PyObject *args)
{
    PyObject *mat;
    if (!PyArg_ParseTuple(args, "O!",&(Base::MatrixPy::Type), &mat))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->getKernel().Transform(static_cast<Base::MatrixPy*>(mat)->value());
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::transformToEigen(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getMeshObjectPtr()->transformToEigenSystem();
    Py_Return;
}

PyObject*  MeshPy::getEigenSystem(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Base::Vector3d vec;
    Base::Matrix4D mat = getMeshObjectPtr()->getEigenSystem(vec);
    Py::Tuple t(2);
    t.setItem(0, Py::Matrix(mat));
    t.setItem(1, Py::Vector(vec));
    return Py::new_reference_to(t);
}

PyObject*  MeshPy::addFacet(PyObject *args)
{
    double x1,y1,z1,x2,y2,z2,x3,y3,z3;
    if (PyArg_ParseTuple(args, "ddddddddd",&x1,&y1,&z1,&x2,&y2,&z2,&x3,&y3,&z3)) {
        getMeshObjectPtr()->addFacet(MeshCore::MeshGeomFacet(
                        Base::Vector3f((float)x1,(float)y1,(float)z1),
                        Base::Vector3f((float)x2,(float)y2,(float)z2),
                        Base::Vector3f((float)x3,(float)y3,(float)z3)));
        Py_Return;
    }

    PyErr_Clear();
    PyObject *v1, *v2, *v3;
    if (PyArg_ParseTuple(args, "O!O!O!",&(Base::VectorPy::Type), &v1,
                                        &(Base::VectorPy::Type), &v2,
                                        &(Base::VectorPy::Type), &v3)) {
        Base::Vector3d *p1 = static_cast<Base::VectorPy*>(v1)->getVectorPtr();
        Base::Vector3d *p2 = static_cast<Base::VectorPy*>(v2)->getVectorPtr();
        Base::Vector3d *p3 = static_cast<Base::VectorPy*>(v3)->getVectorPtr();
        getMeshObjectPtr()->addFacet(MeshCore::MeshGeomFacet(
                        Base::Vector3f((float)p1->x,(float)p1->y,(float)p1->z),
                        Base::Vector3f((float)p2->x,(float)p2->y,(float)p2->z),
                        Base::Vector3f((float)p3->x,(float)p3->y,(float)p3->z)));
        Py_Return;
    }

    PyErr_Clear();
    PyObject *f;
    if (PyArg_ParseTuple(args, "O!",&(Mesh::FacetPy::Type), &f)) {
        Mesh::FacetPy* face = static_cast<Mesh::FacetPy*>(f);
        getMeshObjectPtr()->addFacet(*face->getFacetPtr());
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "set 9 floats or three vectors or a facet");
    return nullptr;
}

PyObject*  MeshPy::addFacets(PyObject *args)
{
    PyObject *list;
    if (PyArg_ParseTuple(args, "O!", &PyList_Type, &list)) {
        Py::List list_f(list);
        Py::Type vVType(Base::getTypeAsObject(&Base::VectorPy::Type));
        Py::Type vFType(Base::getTypeAsObject(&Mesh::FacetPy::Type));

        std::vector<MeshCore::MeshGeomFacet> facets;
        MeshCore::MeshGeomFacet facet;
        for (Py::List::iterator it = list_f.begin(); it != list_f.end(); ++it) {
            if ((*it).isType(vFType)) {
                Mesh::FacetPy* face = static_cast<Mesh::FacetPy*>((*it).ptr());
                facets.push_back(*face->getFacetPtr());
            }
            else if ((*it).isSequence()) {
                Py::Sequence seq(*it);
                if (seq.size() == 3) {
                    if (PyFloat_Check(seq[0].ptr())) {
                        // every three triples build a triangle
                        facet._aclPoints[0] = Base::getVectorFromTuple<float>((*it).ptr());
                        ++it;
                        facet._aclPoints[1] = Base::getVectorFromTuple<float>((*it).ptr());
                        ++it;
                        facet._aclPoints[2] = Base::getVectorFromTuple<float>((*it).ptr());
                    }
                    else if (seq[0].isSequence()) {
                        // a sequence of sequence of flots
                        for (int i=0; i<3; i++) {
                            facet._aclPoints[i] = Base::getVectorFromTuple<float>(seq[i].ptr());
                        }
                    }
                    else if (PyObject_TypeCheck(seq[0].ptr(), &(Base::VectorPy::Type))) {
                        // a sequence of vectors
                        for (int i=0; i<3; i++) {
                            Base::Vector3d p = Py::Vector(seq[i]).toVector();
                            facet._aclPoints[i].Set((float)p.x,(float)p.y,(float)p.z);
                        }
                    }
                    else {
                        PyErr_SetString(PyExc_TypeError, "expect a sequence of floats or Vector");
                        return nullptr;
                    }

                    facet.CalcNormal();
                    facets.push_back(facet);
                }
                else {
                    // 9 consecutive floats expected
                    int index=0;
                    for (int i=0; i<3; i++) {
                        facet._aclPoints[i].x = (float)(double)Py::Float(seq[index++]);
                        facet._aclPoints[i].y = (float)(double)Py::Float(seq[index++]);
                        facet._aclPoints[i].z = (float)(double)Py::Float(seq[index++]);
                    }
                    facet.CalcNormal();
                    facets.push_back(facet);
                }
            } // sequence
        }

        getMeshObjectPtr()->addFacets(facets);
        Py_Return;
    }

    PyErr_Clear();
    PyObject *check = Py_True;
    if (PyArg_ParseTuple(args, "O!|O!", &PyTuple_Type, &list, &PyBool_Type, &check)) {
        Py::Tuple tuple(list);
        Py::List list_v(tuple.getItem(0));
        std::vector<Base::Vector3f> vertices;
        Py::Type vType(Base::getTypeAsObject(&Base::VectorPy::Type));
        for (Py::List::iterator it = list_v.begin(); it != list_v.end(); ++it) {
            if ((*it).isType(vType)) {
                Base::Vector3d v = static_cast<Base::VectorPy*>((*it).ptr())->value();
                vertices.emplace_back((float)v.x,(float)v.y,(float)v.z);
            }
        }

        Py::List list_f(tuple.getItem(1));
        MeshCore::MeshFacetArray faces;
        for (Py::List::iterator it = list_f.begin(); it != list_f.end(); ++it) {
            Py::Tuple f(*it);
            MeshCore::MeshFacet face;
            face._aulPoints[0] = static_cast<long>(Py::Long(f.getItem(0)));
            face._aulPoints[1] = static_cast<long>(Py::Long(f.getItem(1)));
            face._aulPoints[2] = static_cast<long>(Py::Long(f.getItem(2)));
            faces.push_back(face);
        }

        getMeshObjectPtr()->addFacets(faces, vertices, PyObject_IsTrue(check) ? true : false);

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "either expect\n"
        "-- [Vector] (3 of them define a facet)\n"
        "-- ([Vector],[(int,int,int)])");
    return nullptr;
}

PyObject* MeshPy::removeFacets(PyObject *args)
{
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return nullptr;

    std::vector<FacetIndex> indices;
    Py::Sequence ary(list);
    for (Py::Sequence::iterator it = ary.begin(); it != ary.end(); ++it) {
        Py::Long f(*it);
        indices.push_back((long)f);
    }

    getMeshObjectPtr()->deleteFacets(indices);
    Py_Return;
}

PyObject* MeshPy::getInternalFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshEvalInternalFacets eval(kernel);
    eval.Evaluate();

    const std::vector<FacetIndex>& indices = eval.GetIndices();
    Py::List ary(indices.size());
    Py::List::size_type pos=0;
    for (std::vector<FacetIndex>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
        ary[pos++] = Py::Long(*it);
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::rebuildNeighbourHood(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    kernel.RebuildNeighbours();
    Py_Return;
}

PyObject*  MeshPy::addMesh(PyObject *args)
{
    PyObject* mesh;
    if (!PyArg_ParseTuple(args, "O!",&(MeshPy::Type), &mesh))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->addMesh(*static_cast<MeshPy*>(mesh)->getMeshObjectPtr());
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::setPoint(PyObject *args)
{
    unsigned long index;
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "kO!",&index, &(Base::VectorPy::Type), &pnt))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->setPoint(index, static_cast<Base::VectorPy*>(pnt)->value());
    } PY_CATCH;

    Py_Return;
}

PyObject* MeshPy::movePoint(PyObject *args)
{
    unsigned long index;
    Base::Vector3d vec;

    do {
        double  x=0.0,y=0.0,z=0.0;
        if (PyArg_ParseTuple(args, "kddd", &index,&x,&y,&z)) {
            vec.Set(x,y,z);
            break;
        }

        PyErr_Clear(); // set by PyArg_ParseTuple()
        PyObject *object;
        if (PyArg_ParseTuple(args,"kO!", &index, &(Base::VectorPy::Type), &object)) {
            vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
            break;
        }

        PyErr_SetString(PyExc_TypeError, "Tuple of three floats or Vector expected");
        return nullptr;
    }
    while (false);

    getMeshObjectPtr()->movePoint(index, vec);
    Py_Return;
}

PyObject*  MeshPy::getPointNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        std::vector<Base::Vector3d> normals = getMeshObjectPtr()->getPointNormals();
        Py::Tuple ary(normals.size());
        std::size_t numNormals = normals.size();
        for (std::size_t i=0; i<numNormals; i++) {
            ary.setItem(i, Py::Vector(normals[i]));
        }
        return Py::new_reference_to(ary);
    } PY_CATCH;
}

PyObject* MeshPy::addSegment(PyObject *args)
{
    PyObject* pylist;
    if (!PyArg_ParseTuple(args, "O", &pylist))
        return nullptr;

    Py::Sequence list(pylist);
    std::vector<Mesh::FacetIndex> segment;
    unsigned long numFacets = getMeshObjectPtr()->countFacets();
    segment.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Py::Long value(*it);
        Mesh::FacetIndex index = static_cast<unsigned long>(value);
        if (index < numFacets)
            segment.push_back(index);
    }

    getMeshObjectPtr()->addSegment(segment);
    Py_Return;
}

PyObject* MeshPy::countSegments(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    unsigned long count = getMeshObjectPtr()->countSegments();
    return Py_BuildValue("k",count);
}

PyObject* MeshPy::getSegment(PyObject *args)
{
    unsigned long index;
    if (!PyArg_ParseTuple(args, "k", &index))
        return nullptr;

    unsigned long count = getMeshObjectPtr()->countSegments();
    if (index >= count) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return nullptr;
    }

    Py::List ary;
    const std::vector<FacetIndex>& segm = getMeshObjectPtr()->getSegment(index).getIndices();
    for (std::vector<FacetIndex>::const_iterator it = segm.begin(); it != segm.end(); ++it) {
        ary.append(Py::Long(*it));
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::getSeparateComponents(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::List meshesList;
    std::vector<std::vector<FacetIndex> > segs;
    segs = getMeshObjectPtr()->getComponents();
    for (unsigned int i=0; i<segs.size(); i++) {
        MeshObject* mesh = getMeshObjectPtr()->meshFromSegment(segs[i]);
        meshesList.append(Py::Object(new MeshPy(mesh),true));
    }
    return Py::new_reference_to(meshesList);
}

PyObject* MeshPy::getFacetSelection(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::List ary;
    std::vector<FacetIndex> facets;
    getMeshObjectPtr()->getFacetsFromSelection(facets);
    for (std::vector<FacetIndex>::const_iterator it = facets.begin(); it != facets.end(); ++it) {
        ary.append(Py::Long((int)*it));
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::getPointSelection(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::List ary;
    std::vector<PointIndex> points;
    getMeshObjectPtr()->getPointsFromSelection(points);
    for (std::vector<PointIndex>::const_iterator it = points.begin(); it != points.end(); ++it) {
        ary.append(Py::Long((int)*it));
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::meshFromSegment(PyObject *args)
{
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return nullptr;

    std::vector<FacetIndex> segment;
    Py::Sequence ary(list);
    for (Py::Sequence::iterator it = ary.begin(); it != ary.end(); ++it) {
        Py::Long f(*it);
        segment.push_back((long)f);
    }


    MeshObject* mesh = getMeshObjectPtr()->meshFromSegment(segment);
    return new MeshPy(mesh);
}

PyObject*  MeshPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getMeshObjectPtr()->clear();
    Py_Return;
}

PyObject*  MeshPy::isSolid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->isSolid();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::hasNonManifolds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasNonManifolds();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::hasInvalidNeighbourhood(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasInvalidNeighbourhood();
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  MeshPy::hasPointsOutOfRange(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasPointsOutOfRange();
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  MeshPy::hasFacetsOutOfRange(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasFacetsOutOfRange();
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  MeshPy::hasCorruptedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasFacetsOutOfRange();
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  MeshPy::removeNonManifolds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getMeshObjectPtr()->removeNonManifolds();
    Py_Return;
}

PyObject*  MeshPy::removeNonManifoldPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    getMeshObjectPtr()->removeNonManifoldPoints();
    Py_Return;
}

PyObject*  MeshPy::hasSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasSelfIntersections();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::getSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    std::vector<std::pair<FacetIndex, FacetIndex> > selfIndices;
    std::vector<std::pair<Base::Vector3f, Base::Vector3f> > selfPoints;
    MeshCore::MeshEvalSelfIntersection eval(getMeshObjectPtr()->getKernel());
    eval.GetIntersections(selfIndices);
    eval.GetIntersections(selfIndices, selfPoints);

    Py::Tuple tuple(selfIndices.size());
    if (selfIndices.size() == selfPoints.size()) {
        for (std::size_t i=0; i<selfIndices.size(); i++) {
            Py::Tuple item(4);
            item.setItem(0, Py::Long(selfIndices[i].first));
            item.setItem(1, Py::Long(selfIndices[i].second));
            item.setItem(2, Py::Vector(selfPoints[i].first));
            item.setItem(3, Py::Vector(selfPoints[i].second));
            tuple.setItem(i, item);
        }
    }

    return Py::new_reference_to(tuple);
}

PyObject*  MeshPy::fixSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        getMeshObjectPtr()->removeSelfIntersections();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject*  MeshPy::removeFoldsOnSurface(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        getMeshObjectPtr()->removeFoldsOnSurface();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject*  MeshPy::hasInvalidPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasInvalidPoints();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::removeInvalidPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    try {
        getMeshObjectPtr()->removeInvalidPoints();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject*  MeshPy::hasPointsOnEdge(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->hasPointsOnEdge();
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  MeshPy::removePointsOnEdge(PyObject *args, PyObject *kwds)
{
    PyObject *fillBoundary = Py_False;
    static char* keywords[] = {"FillBoundary", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", keywords, &PyBool_Type, &fillBoundary))
        return nullptr;
    try {
        getMeshObjectPtr()->removePointsOnEdge(PyObject_IsTrue(fillBoundary) ? true : false);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject*  MeshPy::flipNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->flipNormals();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::hasNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    bool ok = getMeshObjectPtr()->countNonUniformOrientedFacets() > 0;
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::countNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    unsigned long count = getMeshObjectPtr()->countNonUniformOrientedFacets();
    return Py_BuildValue("k", count); 
}

PyObject*  MeshPy::getNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshEvalOrientation cMeshEval(kernel);
    std::vector<FacetIndex> inds = cMeshEval.GetIndices();
    Py::Tuple tuple(inds.size());
    for (std::size_t i=0; i<inds.size(); i++) {
        tuple.setItem(i, Py::Long(inds[i]));
    }

    return Py::new_reference_to(tuple);
}

PyObject*  MeshPy::harmonizeNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->harmonizeNormals();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::countComponents(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    unsigned long count = getMeshObjectPtr()->countComponents();
    return Py_BuildValue("k",count);
}

PyObject*  MeshPy::removeComponents(PyObject *args)
{
    unsigned long count;
    if (!PyArg_ParseTuple(args, "k", &count))
        return nullptr;

    PY_TRY {
        if (count > 0) {
            getMeshObjectPtr()->removeComponents(count);
        }
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fillupHoles(PyObject *args)
{
    unsigned long len;
    int level = 0;
    float max_area = 0.0f;
    if (!PyArg_ParseTuple(args, "k|if", &len,&level,&max_area))
        return nullptr;
    try {
        std::unique_ptr<MeshCore::AbstractPolygonTriangulator> tria;
        if (max_area > 0.0f) {
            tria = std::unique_ptr<MeshCore::AbstractPolygonTriangulator>
                (new MeshCore::ConstraintDelaunayTriangulator(max_area));
        }
        else {
            tria = std::unique_ptr<MeshCore::AbstractPolygonTriangulator>
                (new MeshCore::FlatTriangulator());
        }

        MeshPropertyLock lock(this->parentProperty);
        tria->SetVerifier(new MeshCore::TriangulationVerifierV2);
        getMeshObjectPtr()->fillupHoles(len, level, *tria);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }

    Py_Return;
}

PyObject*  MeshPy::fixIndices(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->validateIndices();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fixCaps(PyObject *args)
{
    float fMaxAngle = Base::toRadians<float>(150.0f);
    float fSplitFactor = 0.25f;
    if (!PyArg_ParseTuple(args, "|ff", &fMaxAngle, &fSplitFactor))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->validateCaps(fMaxAngle, fSplitFactor);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fixDeformations(PyObject *args)
{
    float fMaxAngle;
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "f|f", &fMaxAngle, &fEpsilon))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->validateDeformations(fMaxAngle, fEpsilon);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fixDegenerations(PyObject *args)
{
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "|f", &fEpsilon))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->validateDegenerations(fEpsilon);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::removeDuplicatedPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->removeDuplicatedPoints();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::removeDuplicatedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->removeDuplicatedFacets();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::refine(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->refine();
    } PY_CATCH;

    Py_Return;
}

PyObject* MeshPy::removeNeedles(PyObject *args)
{
    float length;
    if (!PyArg_ParseTuple(args, "f", &length))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->removeNeedles(length);
    } PY_CATCH;

    Py_Return;
}

PyObject* MeshPy::removeFullBoundaryFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->removeFullBoundaryFacets();
    } PY_CATCH;

    Py_Return;
}

PyObject* MeshPy::mergeFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->mergeFacets();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::optimizeTopology(PyObject *args)
{
    float fMaxAngle=-1.0f;
    if (!PyArg_ParseTuple(args, "|f; specify the maximum allowed angle between the normals of two adjacent facets", &fMaxAngle))
        return nullptr;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->optimizeTopology(fMaxAngle);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::optimizeEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->optimizeEdges();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::splitEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        getMeshObjectPtr()->splitEdges();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::splitEdge(PyObject *args)
{
    unsigned long facet, neighbour;
    PyObject* vertex;
    if (!PyArg_ParseTuple(args, "kkO!", &facet, &neighbour, &Base::VectorPy::Type, &vertex))
        return nullptr;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }

        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return nullptr;
        }
        
        getMeshObjectPtr()->splitEdge(facet, neighbour, v);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::splitFacet(PyObject *args)
{
    unsigned long facet;
    PyObject* vertex1;
    PyObject* vertex2;
    if (!PyArg_ParseTuple(args, "kO!O!", &facet, &Base::VectorPy::Type, &vertex1, 
                                                 &Base::VectorPy::Type, &vertex2))
        return nullptr;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex1);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v1((float)val->x,(float)val->y,(float)val->z);

    pcObject = static_cast<Base::VectorPy*>(vertex2);
    val = pcObject->getVectorPtr();
    Base::Vector3f v2((float)val->x,(float)val->y,(float)val->z);

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        
        getMeshObjectPtr()->splitFacet(facet, v1, v2);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::swapEdge(PyObject *args)
{
    unsigned long facet, neighbour;
    if (!PyArg_ParseTuple(args, "kk", &facet, &neighbour))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
  
        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return nullptr;
        }
        
        getMeshObjectPtr()->swapEdge(facet, neighbour);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::collapseEdge(PyObject *args)
{
    unsigned long facet, neighbour;
    if (!PyArg_ParseTuple(args, "kk", &facet, &neighbour))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
  
        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return nullptr;
        }
        
        getMeshObjectPtr()->collapseEdge(facet, neighbour);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::collapseFacet(PyObject *args)
{
    unsigned long facet;
    if (!PyArg_ParseTuple(args, "k", &facet))
        return nullptr;

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
  
        getMeshObjectPtr()->collapseFacet(facet);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::insertVertex(PyObject *args)
{
    unsigned long facet;
    PyObject* vertex;
    if (!PyArg_ParseTuple(args, "kO!", &facet, &Base::VectorPy::Type, &vertex))
        return nullptr;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        
        getMeshObjectPtr()->insertVertex(facet, v);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::snapVertex(PyObject *args)
{
    unsigned long facet;
    PyObject* vertex;
    if (!PyArg_ParseTuple(args, "kO!", &facet, &Base::VectorPy::Type, &vertex))
        return nullptr;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return nullptr;
        }
        
        getMeshObjectPtr()->snapVertex(facet, v);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::printInfo(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py_BuildValue("s", getMeshObjectPtr()->topologyInfo().c_str());
}

PyObject*  MeshPy::collapseFacets(PyObject *args)
{
    PyObject *pcObj=nullptr;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return nullptr;

    // if no mesh is given
    try {
        Py::Sequence list(pcObj);
        std::vector<FacetIndex> facets;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Long idx(*it);
            unsigned long iIdx = static_cast<unsigned long>(idx);
            facets.push_back(iIdx);
        }

        getMeshObjectPtr()->collapseFacets(facets);
    }
    catch (const Py::Exception&) {
        return nullptr;
    }

    Py_Return;
}

PyObject*  MeshPy::foraminate(PyObject *args)
{
    PyObject* pnt_p;
    PyObject* dir_p;
    if (!PyArg_ParseTuple(args, "OO", &pnt_p, &dir_p))
        return nullptr;

    try {
        Py::Tuple pnt_t(pnt_p);
        Py::Tuple dir_t(dir_p);
        Base::Vector3f pnt((float)Py::Float(pnt_t.getItem(0)),
                           (float)Py::Float(pnt_t.getItem(1)),
                           (float)Py::Float(pnt_t.getItem(2)));
        Base::Vector3f dir((float)Py::Float(dir_t.getItem(0)),
                           (float)Py::Float(dir_t.getItem(1)),
                           (float)Py::Float(dir_t.getItem(2)));

        Base::Vector3f res;
        MeshCore::MeshFacetIterator f_it(getMeshObjectPtr()->getKernel());
        int index = 0;

        Py::Dict dict;
        for (f_it.Begin(); f_it.More(); f_it.Next(), index++) {
            if (f_it->Foraminate(pnt, dir, res)) {
                Py::Tuple tuple(3);
                tuple.setItem(0, Py::Float(res.x));
                tuple.setItem(1, Py::Float(res.y));
                tuple.setItem(2, Py::Float(res.z));
                dict.setItem(Py::Long(index), tuple);
            }
        }

        return Py::new_reference_to(dict);
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject*  MeshPy::cut(PyObject *args)
{
    PyObject* poly;
    int mode;
    if (!PyArg_ParseTuple(args, "Oi", &poly, &mode))
        return nullptr;

    Py::Sequence list(poly);
    std::vector<Base::Vector3f> polygon;
    polygon.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Base::Vector3d pnt = Py::Vector(*it).toVector();
        polygon.push_back(Base::convertTo<Base::Vector3f>(pnt));
    }

    MeshCore::FlatTriangulator tria;
    tria.SetPolygon(polygon);
    // this gives us the inverse matrix
    Base::Matrix4D inv = tria.GetTransformToFitPlane();
    // compute the matrix for the coordinate transformation
    Base::Matrix4D mat = inv;
    mat.inverseOrthogonal();

    polygon = tria.ProjectToFitPlane();

    Base::ViewProjMatrix proj(mat);
    Base::Polygon2d polygon2d;
    for (std::vector<Base::Vector3f>::const_iterator it = polygon.begin(); it != polygon.end(); ++it)
        polygon2d.Add(Base::Vector2d(it->x, it->y));
    getMeshObjectPtr()->cut(polygon2d, proj, MeshObject::CutType(mode));

    Py_Return;
}

PyObject*  MeshPy::trim(PyObject *args)
{
    PyObject* poly;
    int mode;
    if (!PyArg_ParseTuple(args, "Oi", &poly, &mode))
        return nullptr;

    Py::Sequence list(poly);
    std::vector<Base::Vector3f> polygon;
    polygon.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Base::Vector3d pnt = Py::Vector(*it).toVector();
        polygon.push_back(Base::convertTo<Base::Vector3f>(pnt));
    }

    MeshCore::FlatTriangulator tria;
    tria.SetPolygon(polygon);
    // this gives us the inverse matrix
    Base::Matrix4D inv = tria.GetTransformToFitPlane();
    // compute the matrix for the coordinate transformation
    Base::Matrix4D mat = inv;
    mat.inverseOrthogonal();

    polygon = tria.ProjectToFitPlane();

    Base::ViewOrthoProjMatrix proj(mat);
    Base::Polygon2d polygon2d;
    for (std::vector<Base::Vector3f>::const_iterator it = polygon.begin(); it != polygon.end(); ++it)
        polygon2d.Add(Base::Vector2d(it->x, it->y));
    getMeshObjectPtr()->trim(polygon2d, proj, MeshObject::CutType(mode));

    Py_Return;
}

PyObject*  MeshPy::trimByPlane(PyObject *args)
{
    PyObject *base, *norm;
    if (!PyArg_ParseTuple(args, "O!O!", &Base::VectorPy::Type, &base,
                                        &Base::VectorPy::Type, &norm))
        return nullptr;

    Base::Vector3d pnt = Py::Vector(base, false).toVector();
    Base::Vector3d dir = Py::Vector(norm, false).toVector();

    getMeshObjectPtr()->trimByPlane(Base::convertTo<Base::Vector3f>(pnt),
                                    Base::convertTo<Base::Vector3f>(dir));

    Py_Return;
}

PyObject*  MeshPy::smooth(PyObject *args, PyObject *kwds)
{
    char* method = "Laplace";
    int iter=1;
    double lambda = 0;
    double micro = 0;
    static char* keywords_smooth[] = {"Method","Iteration","Lambda","Micro",nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sidd",keywords_smooth,
                                     &method, &iter, &lambda, &micro))
        return nullptr;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
        if (strcmp(method, "Laplace") == 0) {
            MeshCore::LaplaceSmoothing smooth(kernel);
            if (lambda > 0)
                smooth.SetLambda(lambda);
            smooth.Smooth(iter);
        }
        else if (strcmp(method, "Taubin") == 0) {
            MeshCore::TaubinSmoothing smooth(kernel);
            if (lambda > 0)
                smooth.SetLambda(lambda);
            if (micro > 0)
                smooth.SetMicro(micro);
            smooth.Smooth(iter);
        }
        else if (strcmp(method, "PlaneFit") == 0) {
            MeshCore::PlaneFitSmoothing smooth(kernel);
            smooth.Smooth(iter);
        }
        else {
            throw Py::ValueError("No such smoothing algorithm");
        }
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::decimate(PyObject *args)
{
    float fTol, fRed;
    if (PyArg_ParseTuple(args, "ff", &fTol,&fRed)) {
        PY_TRY {
            getMeshObjectPtr()->decimate(fTol, fRed);
        } PY_CATCH;

        Py_Return;
    }

    PyErr_Clear();
    int targetSize;
    if (PyArg_ParseTuple(args, "i", &targetSize)) {
        PY_TRY {
            getMeshObjectPtr()->decimate(targetSize);
        } PY_CATCH;

        Py_Return;
    }

    PyErr_SetString(PyExc_ValueError, "decimate(tolerance=float, reduction=float) or decimate(targetSize=int)");
    return nullptr;
}

PyObject* MeshPy::nearestFacetOnRay(PyObject *args)
{
    PyObject* pnt_p;
    PyObject* dir_p;
    double maxAngle = MeshCore::Mathd::PI;
    if (!PyArg_ParseTuple(args, "OO|d", &pnt_p, &dir_p, &maxAngle))
        return nullptr;

    try {
        Py::Tuple pnt_t(pnt_p);
        Py::Tuple dir_t(dir_p);
        Py::Dict dict;
        Base::Vector3f pnt((float)Py::Float(pnt_t.getItem(0)),
                           (float)Py::Float(pnt_t.getItem(1)),
                           (float)Py::Float(pnt_t.getItem(2)));
        Base::Vector3f dir((float)Py::Float(dir_t.getItem(0)),
                           (float)Py::Float(dir_t.getItem(1)),
                           (float)Py::Float(dir_t.getItem(2)));

        FacetIndex index = 0;
        Base::Vector3f res;
        MeshCore::MeshAlgorithm alg(getMeshObjectPtr()->getKernel());

#if 0 // for testing only
        MeshCore::MeshFacetGrid grid(getMeshObjectPtr()->getKernel(),10);
        // With grids we might search in the opposite direction, too
        if (alg.NearestFacetOnRay(pnt,  dir, grid, res, index) ||
            alg.NearestFacetOnRay(pnt, -dir, grid, res, index)) {
#else
        if (alg.NearestFacetOnRay(pnt, dir, static_cast<float>(maxAngle), res, index)) {
#endif
            Py::Tuple tuple(3);
            tuple.setItem(0, Py::Float(res.x));
            tuple.setItem(1, Py::Float(res.y));
            tuple.setItem(2, Py::Float(res.z));
            dict.setItem(Py::Long((int)index), tuple);
        }

#if 0 // for testing only
        char szBuf[200];
        std::ofstream str("grid_test.iv");
        Base::InventorBuilder builder(str);
        MeshCore::MeshGridIterator g_it(grid);
        for (g_it.Init(); g_it.More(); g_it.Next()) {
            Base::BoundBox3f box = g_it.GetBoundBox();
            unsigned long uX,uY,uZ;
            g_it.GetGridPos(uX,uY,uZ);
            builder.addBoundingBox(Base::Vector3f(box.MinX,box.MinY, box.MinZ),
                                   Base::Vector3f(box.MaxX,box.MaxY, box.MaxZ));
            sprintf(szBuf, "(%lu,%lu,%lu)", uX, uY, uZ);
            builder.addText(box.GetCenter(), szBuf);
        }
        builder.addSingleArrow(pnt-20.0f*dir, pnt+10.0f*dir);
        builder.close();
        str.close();
#endif

        return Py::new_reference_to(dict);
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject*  MeshPy::getPlanarSegments(PyObject *args)
{
    float dev;
    unsigned long minFacets=0;
    if (!PyArg_ParseTuple(args, "f|k",&dev,&minFacets))
        return nullptr;

    Mesh::MeshObject* mesh = getMeshObjectPtr();
    std::vector<Mesh::Segment> segments = mesh->getSegmentsOfType
        (Mesh::MeshObject::PLANE, dev, minFacets);

    Py::List s;
    for (std::vector<Mesh::Segment>::iterator it = segments.begin(); it != segments.end(); ++it) {
        const std::vector<FacetIndex>& segm = it->getIndices();
        Py::List ary;
        for (std::vector<FacetIndex>::const_iterator jt = segm.begin(); jt != segm.end(); ++jt) {
            ary.append(Py::Long(*jt));
        }
        s.append(ary);
    }

    return Py::new_reference_to(s);
}

PyObject*  MeshPy::getSegmentsOfType(PyObject *args)
{
    char* type;
    float dev;
    unsigned long minFacets=0;
    if (!PyArg_ParseTuple(args, "sf|k",&type,&dev,&minFacets))
        return nullptr;

    Mesh::MeshObject::GeometryType geoType;
    if (strcmp(type, "Plane") == 0) {
        geoType = Mesh::MeshObject::PLANE;
    }
    else if (strcmp(type, "Cylinder") == 0) {
        geoType = Mesh::MeshObject::CYLINDER;
    }
    else if (strcmp(type, "Sphere") == 0) {
        geoType = Mesh::MeshObject::SPHERE;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Unsupported surface type");
        return nullptr;
    }

    Mesh::MeshObject* mesh = getMeshObjectPtr();
    std::vector<Mesh::Segment> segments = mesh->getSegmentsOfType
        (geoType, dev, minFacets);

    Py::List s;
    for (std::vector<Mesh::Segment>::iterator it = segments.begin(); it != segments.end(); ++it) {
        const std::vector<FacetIndex>& segm = it->getIndices();
        Py::List ary;
        for (std::vector<FacetIndex>::const_iterator jt = segm.begin(); jt != segm.end(); ++jt) {
            ary.append(Py::Long((int)*jt));
        }
        s.append(ary);
    }

    return Py::new_reference_to(s);
}

PyObject*  MeshPy::getSegmentsByCurvature(PyObject *args)
{
    PyObject* l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshSegmentAlgorithm finder(kernel);
    MeshCore::MeshCurvature meshCurv(kernel);
    meshCurv.ComputePerVertex();

    Py::Sequence func(l);
    std::vector<MeshCore::MeshSurfaceSegmentPtr> segm;
    for (Py::Sequence::iterator it = func.begin(); it != func.end(); ++it) {
        Py::Tuple t(*it);
        float c1 = (float)Py::Float(t[0]);
        float c2 = (float)Py::Float(t[1]);
        float tol1 = (float)Py::Float(t[2]);
        float tol2 = (float)Py::Float(t[3]);
        int num = (int)Py::Long(t[4]);
        segm.emplace_back(std::make_shared<MeshCore::MeshCurvatureFreeformSegment>(meshCurv.GetCurvature(), num, tol1, tol2, c1, c2));
    }

    finder.FindSegments(segm);

    Py::List list;
    for (std::vector<MeshCore::MeshSurfaceSegmentPtr>::iterator segmIt = segm.begin(); segmIt != segm.end(); ++segmIt) {
        const std::vector<MeshCore::MeshSegment>& data = (*segmIt)->GetSegments();
        for (std::vector<MeshCore::MeshSegment>::const_iterator it = data.begin(); it != data.end(); ++it) {
            Py::List ary;
            for (MeshCore::MeshSegment::const_iterator jt = it->begin(); jt != it->end(); ++jt) {
                ary.append(Py::Long((int)*jt));
            }
            list.append(ary);
        }
    }

    return Py::new_reference_to(list);
}

PyObject* MeshPy::getCurvaturePerVertex(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshSegmentAlgorithm finder(kernel);
    MeshCore::MeshCurvature meshCurv(kernel);
    meshCurv.ComputePerVertex();

    const std::vector<MeshCore::CurvatureInfo>& curv = meshCurv.GetCurvature();
    Py::List list;
    for (const auto& it : curv) {
        Py::Tuple tuple(4);
        tuple.setItem(0, Py::Float(it.fMaxCurvature));
        tuple.setItem(1, Py::Float(it.fMinCurvature));
        Py::Tuple maxDir(3);
        maxDir.setItem(0, Py::Float(it.cMaxCurvDir.x));
        maxDir.setItem(1, Py::Float(it.cMaxCurvDir.y));
        maxDir.setItem(2, Py::Float(it.cMaxCurvDir.z));
        tuple.setItem(2, maxDir);
        Py::Tuple minDir(3);
        minDir.setItem(0, Py::Float(it.cMinCurvDir.x));
        minDir.setItem(1, Py::Float(it.cMinCurvDir.y));
        minDir.setItem(2, Py::Float(it.cMinCurvDir.z));
        tuple.setItem(3, minDir);
        list.append(tuple);
    }

    return Py::new_reference_to(list);
}

Py::Long MeshPy::getCountPoints() const
{
    return Py::Long((long)getMeshObjectPtr()->countPoints());
}

Py::Long MeshPy::getCountEdges() const
{
    return Py::Long((long)getMeshObjectPtr()->countEdges());
}

Py::Long MeshPy::getCountFacets() const
{
    return Py::Long((long)getMeshObjectPtr()->countFacets());
}

Py::Float MeshPy::getArea() const
{
    return Py::Float(getMeshObjectPtr()->getSurface());
}

Py::Float MeshPy::getVolume() const
{
    return Py::Float(getMeshObjectPtr()->getVolume());
}

PyObject *MeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

Py::List MeshPy::getPoints() const
{
    Py::List PointList;
    unsigned int Index=0;
    MeshObject* mesh = getMeshObjectPtr();
    for (MeshObject::const_point_iterator it = mesh->points_begin(); it != mesh->points_end(); ++it) {
        PointList.append(Py::Object(new MeshPointPy(new MeshPoint(*it,getMeshObjectPtr(),Index++)), true));
    }
    return PointList;
}

Py::List MeshPy::getFacets() const
{
    Py::List FacetList;
    MeshObject* mesh = getMeshObjectPtr();
    for (MeshObject::const_facet_iterator it = mesh->facets_begin(); it != mesh->facets_end(); ++it) {
        FacetList.append(Py::Object(new FacetPy(new Facet(*it)), true));
    }
    return FacetList;
}

Py::Tuple MeshPy::getTopology() const
{
    std::vector<Base::Vector3d> Points;
    std::vector<Data::ComplexGeoData::Facet> Facets;
    getMeshObjectPtr()->getFaces(Points, Facets, 0.0f);
    Py::Tuple tuple(2);
    Py::List vertex;
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin();
        it != Points.end(); ++it)
        vertex.append(Py::asObject(new Base::VectorPy(*it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
        it = Facets.begin(); it != Facets.end(); ++it) {
        Py::Tuple f(3);
        f.setItem(0,Py::Long((int)it->I1));
        f.setItem(1,Py::Long((int)it->I2));
        f.setItem(2,Py::Long((int)it->I3));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return tuple;
}


