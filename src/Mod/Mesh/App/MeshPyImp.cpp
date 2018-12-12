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
#include <Base/GeometryPyCXX.h>
#include <Base/MatrixPy.h>

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
    PyObject *pcObj=0;
    if (!PyArg_ParseTuple(args, "|O", &pcObj))     // convert args: Python->C 
        return -1;                             // NULL triggers exception

    try {
        this->parentProperty = 0;
        // if no mesh is given
        if (!pcObj) return 0;
        if (PyObject_TypeCheck(pcObj, &(MeshPy::Type))) {
            getMeshObjectPtr()->operator = (*static_cast<MeshPy*>(pcObj)->getMeshObjectPtr());
        }
        else if (PyList_Check(pcObj)) {
            PyObject* ret = addFacets(args);
            bool ok = (ret!=0);
            Py_XDECREF(ret);
            if (!ok) return -1;
        }
        else if (PyTuple_Check(pcObj)) {
            PyObject* ret = addFacets(args);
            bool ok = (ret!=0);
            Py_XDECREF(ret);
            if (!ok) return -1;
        }
        else if (PyUnicode_Check(pcObj)) {
#if PY_MAJOR_VERSION >= 3
            getMeshObjectPtr()->load(PyUnicode_AsUTF8(pcObj));
#else
            PyObject* unicode = PyUnicode_AsEncodedString(pcObj, "utf-8", 0);
            char* pItem = PyString_AsString(unicode);
            Py_DECREF(unicode);
            getMeshObjectPtr()->load(pItem);
        }
        else if (PyString_Check(pcObj)) {
            getMeshObjectPtr()->load(PyString_AsString(pcObj));
#endif
        }
        else {
            PyErr_Format(PyExc_TypeError, "Cannot create a mesh out of a '%s'",
                pcObj->ob_type->tp_name);
            return -1;
        }
    }
    catch (const Base::Exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return -1;
    }
    catch (const std::exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return -1;
    }
    catch (const Py::Exception&) {
        return -1;
    }

    return 0;
}

// returns a string which represent the object e.g. when printed in python
std::string MeshPy::representation(void) const
{
    // Note: As the return type is 'const char*' we cannot create a temporary char array neither on the stack because the array would be freed
    // when leaving the scope nor on the heap because we would have a memory leak.
    // So we use a static array that is used by all instances of this class. This, however, is not a problem as long as we only
    // use this method in _repr().
    MeshPy::PointerType ptr = reinterpret_cast<MeshPy::PointerType>(_pcTwinPointer);

    return  ptr->representation();
}

PyObject *MeshPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MeshPy and the Twin object 
    return new MeshPy(new MeshObject);
}

PyObject* MeshPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    return new MeshPy(new MeshObject(kernel));
}

PyObject*  MeshPy::read(PyObject *args, PyObject *kwds)
{
    char* Name;
    static char* keywords_path[] = {"Filename",NULL};
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
    static char* keywords_stream[] = {"Stream","Format",NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "Os",keywords_stream, &input, &Ext)) {
        std::string fmt(Ext);
        boost::to_upper(fmt);
        if (ext.find(fmt) != ext.end()) {
            format = ext[fmt];
        }

        // read mesh
        Base::PyStreambuf buf(input);
        std::istream str(0);
        str.rdbuf(&buf);
        getMeshObjectPtr()->load(str, format);

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return NULL;
}

PyObject*  MeshPy::write(PyObject *args, PyObject *kwds)
{
    char* Name;
    char* Ext=0;
    char* ObjName=0;
    PyObject* List=0;

    MeshCore::MeshIO::Format format = MeshCore::MeshIO::Undefined;
    std::map<std::string, MeshCore::MeshIO::Format> ext;
    ext["BMS" ] = MeshCore::MeshIO::BMS;
    ext["STL" ] = MeshCore::MeshIO::BSTL;
    ext["AST" ] = MeshCore::MeshIO::ASTL;
    ext["OBJ" ] = MeshCore::MeshIO::OBJ;
    ext["SMF" ] = MeshCore::MeshIO::SMF;
    ext["OFF" ] = MeshCore::MeshIO::OFF;
    ext["IDTF"] = MeshCore::MeshIO::IDTF;
    ext["MGL" ] = MeshCore::MeshIO::MGL;
    ext["IV"  ] = MeshCore::MeshIO::IV;
    ext["X3D" ] = MeshCore::MeshIO::X3D;
    ext["VRML"] = MeshCore::MeshIO::VRML;
    ext["WRL" ] = MeshCore::MeshIO::VRML;
    ext["WRZ" ] = MeshCore::MeshIO::WRZ;
    ext["NAS" ] = MeshCore::MeshIO::NAS;
    ext["BDF" ] = MeshCore::MeshIO::NAS;
    ext["PLY" ] = MeshCore::MeshIO::PLY;
    ext["APLY"] = MeshCore::MeshIO::APLY;
    ext["PY"  ] = MeshCore::MeshIO::PY;

    static char* keywords_path[] = {"Filename","Format","Name","Material",NULL};
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
                mat.diffuseColor.push_back(App::Color(r,g,b));
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
            getMeshObjectPtr()->save(Name, format, 0, ObjName);
        }

        PyMem_Free(Name);
        Py_Return;
    }

    PyErr_Clear();

    static char* keywords_stream[] = {"Stream","Format","Name","Material",NULL};
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
                mat->diffuseColor.push_back(App::Color(r,g,b));
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
        std::ostream str(0);
        str.rdbuf(&buf);
        getMeshObjectPtr()->save(str, format, mat.get(), ObjName);

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "expect string or file object");
    return NULL;
}

PyObject*  MeshPy::writeInventor(PyObject *args)
{
    float creaseangle=0.0f;
    if (!PyArg_ParseTuple(args, "|f",&creaseangle))
        return NULL;

    MeshObject* mesh = getMeshObjectPtr();
    const MeshCore::MeshFacetArray& faces = mesh->getKernel().GetFacets();
    std::vector<int> indices;
    std::vector<Base::Vector3f> coords;
    coords.reserve(mesh->countPoints());
    for (MeshObject::const_point_iterator it = mesh->points_begin(); it != mesh->points_end(); ++it)
        coords.push_back(Base::Vector3f((float)it->x,(float)it->y,(float)it->z));
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
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->offsetSpecial2(Float);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::offsetSpecial(PyObject *args)
{
    float Float,zmin,zmax;
    if (!PyArg_ParseTuple(args, "fff",&Float,&zmin,&zmax))			 
        return NULL;                         

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
        return 0;

    Py::Sequence list(obj);
    union PyType_Object pyType = {&(Base::VectorPy::Type)};
    Py::Type vType(pyType.o);

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
                polyline.append(Py::Object(new Base::VectorPy(*kt)));
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
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

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
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

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
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

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
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

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
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    pcObject = static_cast<MeshPy*>(pcObj);

    PY_TRY {
        MeshObject* mesh = getMeshObjectPtr()->outer(*pcObject->getMeshObjectPtr());
        return new MeshPy(mesh);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::coarsen(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject*  MeshPy::translate(PyObject *args)
{
    float x,y,z;
    if (!PyArg_ParseTuple(args, "fff",&x,&y,&z))
        return NULL;

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
        return NULL;

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
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->getKernel().Transform(static_cast<Base::MatrixPy*>(mat)->value());
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::transformToEigen(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getMeshObjectPtr()->transformToEigenSystem();
    Py_Return;
}

PyObject*  MeshPy::getEigenSystem(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
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

    PyErr_SetString(Base::BaseExceptionFreeCADError, "set 9 floats or three vectors or a facet");
    return 0;
}

PyObject*  MeshPy::addFacets(PyObject *args)
{
    PyObject *list;
    if (PyArg_ParseTuple(args, "O!", &PyList_Type, &list)) {
        Py::List list_f(list);
        union PyType_Object pyVType = {&(Base::VectorPy::Type)};
        Py::Type vVType(pyVType.o);

        union PyType_Object pyFType = {&(Mesh::FacetPy::Type)};
        Py::Type vFType(pyFType.o);

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
                        PyErr_SetString(Base::BaseExceptionFreeCADError, "expect a sequence of floats or Vector");
                        return NULL;
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
        union PyType_Object pyVertType = {&(Base::VectorPy::Type)};
        Py::Type vType(pyVertType.o);
        for (Py::List::iterator it = list_v.begin(); it != list_v.end(); ++it) {
            if ((*it).isType(vType)) {
                Base::Vector3d v = static_cast<Base::VectorPy*>((*it).ptr())->value();
                vertices.push_back(Base::Vector3f((float)v.x,(float)v.y,(float)v.z));
            }
        }

        Py::List list_f(tuple.getItem(1));
        MeshCore::MeshFacetArray faces;
        for (Py::List::iterator it = list_f.begin(); it != list_f.end(); ++it) {
            Py::Tuple f(*it);
            MeshCore::MeshFacet face;
#if PY_MAJOR_VERSION >= 3
            face._aulPoints[0] = (long)Py::Long(f.getItem(0));
            face._aulPoints[1] = (long)Py::Long(f.getItem(1));
            face._aulPoints[2] = (long)Py::Long(f.getItem(2));
#else
            face._aulPoints[0] = (long)Py::Int(f.getItem(0));
            face._aulPoints[1] = (long)Py::Int(f.getItem(1));
            face._aulPoints[2] = (long)Py::Int(f.getItem(2));
#endif
            faces.push_back(face);
        }

        getMeshObjectPtr()->addFacets(faces, vertices, PyObject_IsTrue(check) ? true : false);

        Py_Return;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "either expect\n"
        "-- [Vector] (3 of them define a facet)\n"
        "-- ([Vector],[(int,int,int)])");
    return NULL;
}

PyObject* MeshPy::removeFacets(PyObject *args)
{
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return 0;

    std::vector<unsigned long> indices;
    Py::Sequence ary(list);
    for (Py::Sequence::iterator it = ary.begin(); it != ary.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        Py::Long f(*it);
#else
        Py::Int f(*it);
#endif
        indices.push_back((long)f);
    }

    getMeshObjectPtr()->deleteFacets(indices);
    Py_Return;
}

PyObject* MeshPy::getInternalFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshEvalInternalFacets eval(kernel);
    eval.Evaluate();

    const std::vector<unsigned long>& indices = eval.GetIndices();
    Py::List ary(indices.size());
    Py::List::size_type pos=0;
    for (std::vector<unsigned long>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
        ary[pos++] = Py::Long(*it);
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::rebuildNeighbourHood(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    kernel.RebuildNeighbours();
    Py_Return;
}

PyObject*  MeshPy::addMesh(PyObject *args)
{
    PyObject* mesh;
    if (!PyArg_ParseTuple(args, "O!",&(MeshPy::Type), &mesh))
        return NULL;

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
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->setPoint(index, static_cast<Base::VectorPy*>(pnt)->value());
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::getPointNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

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

PyObject* MeshPy::countSegments(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    unsigned long count = getMeshObjectPtr()->countSegments();
    return Py_BuildValue("k",count);
}

PyObject* MeshPy::getSegment(PyObject *args)
{
    unsigned long index;
    if (!PyArg_ParseTuple(args, "k", &index))
        return 0;

    unsigned long count = getMeshObjectPtr()->countSegments();
    if (index >= count) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return 0;
    }

    Py::List ary;
    const std::vector<unsigned long>& segm = getMeshObjectPtr()->getSegment(index).getIndices();
    for (std::vector<unsigned long>::const_iterator it = segm.begin(); it != segm.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        ary.append(Py::Long((int)*it));
#else
        ary.append(Py::Int((int)*it));
#endif
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::getSeparateComponents(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Py::List meshesList;
    std::vector<std::vector<unsigned long> > segs;
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
        return 0;

    Py::List ary;
    std::vector<unsigned long> facets;
    getMeshObjectPtr()->getFacetsFromSelection(facets);
    for (std::vector<unsigned long>::const_iterator it = facets.begin(); it != facets.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        ary.append(Py::Long((int)*it));
#else
        ary.append(Py::Int((int)*it));
#endif
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::getPointSelection(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Py::List ary;
    std::vector<unsigned long> points;
    getMeshObjectPtr()->getPointsFromSelection(points);
    for (std::vector<unsigned long>::const_iterator it = points.begin(); it != points.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        ary.append(Py::Long((int)*it));
#else
        ary.append(Py::Int((int)*it));
#endif
    }

    return Py::new_reference_to(ary);
}

PyObject* MeshPy::meshFromSegment(PyObject *args)
{
    PyObject* list;
    if (!PyArg_ParseTuple(args, "O", &list))
        return 0;

    std::vector<unsigned long> segment;
    Py::Sequence ary(list);
    for (Py::Sequence::iterator it = ary.begin(); it != ary.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        Py::Long f(*it);
#else
        Py::Int f(*it);
#endif
        segment.push_back((long)f);
    }


    MeshObject* mesh = getMeshObjectPtr()->meshFromSegment(segment);
    return new MeshPy(mesh);
}

PyObject*  MeshPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getMeshObjectPtr()->clear();
    Py_Return;
}

PyObject*  MeshPy::isSolid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool ok = getMeshObjectPtr()->isSolid();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::hasNonManifolds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool ok = getMeshObjectPtr()->hasNonManifolds();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::removeNonManifolds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getMeshObjectPtr()->removeNonManifolds();
    Py_Return;
}

PyObject*  MeshPy::removeNonManifoldPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getMeshObjectPtr()->removeNonManifoldPoints();
    Py_Return;
}

PyObject*  MeshPy::hasSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool ok = getMeshObjectPtr()->hasSelfIntersections();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::getSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    std::vector<std::pair<unsigned long, unsigned long> > selfIndices;
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
        return NULL;
    try {
        getMeshObjectPtr()->removeSelfIntersections();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    Py_Return;
}

PyObject*  MeshPy::removeFoldsOnSurface(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        getMeshObjectPtr()->removeFoldsOnSurface();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    Py_Return;
}

PyObject*  MeshPy::hasInvalidPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool ok = getMeshObjectPtr()->hasInvalidPoints();
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::removeInvalidPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        getMeshObjectPtr()->removeInvalidPoints();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }
    Py_Return;
}

PyObject*  MeshPy::flipNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->flipNormals();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::hasNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool ok = getMeshObjectPtr()->countNonUniformOrientedFacets() > 0;
    return Py_BuildValue("O", (ok ? Py_True : Py_False)); 
}

PyObject*  MeshPy::countNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    unsigned long count = getMeshObjectPtr()->countNonUniformOrientedFacets();
    return Py_BuildValue("k", count); 
}

PyObject*  MeshPy::getNonUniformOrientedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshEvalOrientation cMeshEval(kernel);
    std::vector<unsigned long> inds = cMeshEval.GetIndices();
    Py::Tuple tuple(inds.size());
    for (std::size_t i=0; i<inds.size(); i++) {
        tuple.setItem(i, Py::Long(inds[i]));
    }

    return Py::new_reference_to(tuple);
}

PyObject*  MeshPy::harmonizeNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->harmonizeNormals();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::countComponents(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    unsigned long count = getMeshObjectPtr()->countComponents();
    return Py_BuildValue("k",count);
}

PyObject*  MeshPy::removeComponents(PyObject *args)
{
    unsigned long count;
    if (!PyArg_ParseTuple(args, "k", &count))
        return NULL;                         

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
        return NULL;
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
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  MeshPy::fixIndices(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->validateIndices();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fixDeformations(PyObject *args)
{
    float fMaxAngle;
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "f|f", &fMaxAngle, &fEpsilon))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->validateDeformations(fMaxAngle, fEpsilon);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::fixDegenerations(PyObject *args)
{
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "|f", &fEpsilon))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->validateDegenerations(fEpsilon);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::removeDuplicatedPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->removeDuplicatedPoints();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::removeDuplicatedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->removeDuplicatedFacets();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::refine(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->refine();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::optimizeTopology(PyObject *args)
{
    float fMaxAngle=-1.0f;
    if (!PyArg_ParseTuple(args, "|f; specify the maximum allowed angle between the normals of two adjacent facets", &fMaxAngle))
        return NULL;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->optimizeTopology(fMaxAngle);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::optimizeEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        MeshPropertyLock lock(this->parentProperty);
        getMeshObjectPtr()->optimizeEdges();
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::splitEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

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
        return NULL;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }

        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return NULL;
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
        return NULL;

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
            return NULL;
        }
        
        getMeshObjectPtr()->splitFacet(facet, v1, v2);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::swapEdge(PyObject *args)
{
    unsigned long facet, neighbour;
    if (!PyArg_ParseTuple(args, "kk", &facet, &neighbour))
        return NULL;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
  
        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return NULL;
        }
        
        getMeshObjectPtr()->swapEdge(facet, neighbour);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::collapseEdge(PyObject *args)
{
    unsigned long facet, neighbour;
    if (!PyArg_ParseTuple(args, "kk", &facet, &neighbour))
        return NULL;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    PY_TRY {
        if (facet >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
        if (neighbour >= kernel.CountFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
  
        const MeshCore::MeshFacet& rclF = kernel.GetFacets()[facet];
        if (rclF._aulNeighbours[0] != neighbour && rclF._aulNeighbours[1] != neighbour &&
            rclF._aulNeighbours[2] != neighbour) {
            PyErr_SetString(PyExc_IndexError, "No adjacent facets");
            return NULL;
        }
        
        getMeshObjectPtr()->collapseEdge(facet, neighbour);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::collapseFacet(PyObject *args)
{
    unsigned long facet;
    if (!PyArg_ParseTuple(args, "k", &facet))
        return NULL;

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
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
        return NULL;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
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
        return NULL;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(vertex);
    Base::Vector3d* val = pcObject->getVectorPtr();
    Base::Vector3f v((float)val->x,(float)val->y,(float)val->z);

    PY_TRY {
        if (facet >= getMeshObjectPtr()->countFacets()) {
            PyErr_SetString(PyExc_IndexError, "Facet index out of range");
            return NULL;
        }
        
        getMeshObjectPtr()->snapVertex(facet, v);
    } PY_CATCH;

    Py_Return;
}

PyObject*  MeshPy::printInfo(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return Py_BuildValue("s", getMeshObjectPtr()->topologyInfo().c_str());
}

PyObject*  MeshPy::collapseFacets(PyObject *args)
{
    PyObject *pcObj=0;
    if (!PyArg_ParseTuple(args, "O", &pcObj))     // convert args: Python->C 
        return 0;                             // NULL triggers exception

    // if no mesh is given
    try {
        Py::Sequence list(pcObj);
        std::vector<unsigned long> facets;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            Py::Long idx(*it);
            unsigned long iIdx = static_cast<unsigned long>(idx);
            facets.push_back(iIdx);
#else
            Py::Int idx(*it);
            unsigned long iIdx = static_cast<unsigned long>(idx);
            facets.push_back(iIdx);
#endif
        }

        getMeshObjectPtr()->collapseFacets(facets);
    }
    catch (const Py::Exception&) {
        return 0;
    }

    Py_Return;
}

PyObject*  MeshPy::foraminate(PyObject *args)
{
    PyObject* pnt_p;
    PyObject* dir_p;
    if (!PyArg_ParseTuple(args, "OO", &pnt_p, &dir_p))
        return NULL;

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
#if PY_MAJOR_VERSION >= 3
                dict.setItem(Py::Long(index), tuple);
#else
                dict.setItem(Py::Int(index), tuple);
#endif
            }
        }

        return Py::new_reference_to(dict);
    }
    catch (const Py::Exception&) {
        return 0;
    }
}

PyObject*  MeshPy::cut(PyObject *args)
{
    PyObject* poly;
    int mode;
    if (!PyArg_ParseTuple(args, "Oi", &poly, &mode))
        return NULL;

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
        return NULL;

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
    getMeshObjectPtr()->trim(polygon2d, proj, MeshObject::CutType(mode));

    Py_Return;
}

PyObject*  MeshPy::smooth(PyObject *args, PyObject *kwds)
{
    char* method = "Laplace";
    int iter=1;
    double lambda = 0;
    double micro = 0;
    static char* keywords_smooth[] = {"Method","Iteration","Lambda","Micro",NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sidd",keywords_smooth,
                                     &method, &iter, &lambda, &micro))
        return 0;

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
    if (!PyArg_ParseTuple(args, "ff", &fTol,&fRed))
        return NULL;

    PY_TRY {
        getMeshObjectPtr()->decimate(fTol, fRed);
    } PY_CATCH;

    Py_Return;
}

PyObject* MeshPy::nearestFacetOnRay(PyObject *args)
{
    PyObject* pnt_p;
    PyObject* dir_p;
    if (!PyArg_ParseTuple(args, "OO", &pnt_p, &dir_p))
        return NULL;

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

        unsigned long index = 0;
        Base::Vector3f res;
        MeshCore::MeshAlgorithm alg(getMeshObjectPtr()->getKernel());

#if 0 // for testing only
        MeshCore::MeshFacetGrid grid(getMeshObjectPtr()->getKernel(),10);
        // With grids we might search in the opposite direction, too
        if (alg.NearestFacetOnRay(pnt,  dir, grid, res, index) ||
            alg.NearestFacetOnRay(pnt, -dir, grid, res, index)) {
#else
        if (alg.NearestFacetOnRay(pnt, dir, res, index)) {
#endif
            Py::Tuple tuple(3);
            tuple.setItem(0, Py::Float(res.x));
            tuple.setItem(1, Py::Float(res.y));
            tuple.setItem(2, Py::Float(res.z));
#if PY_MAJOR_VERSION >= 3
            dict.setItem(Py::Long((int)index), tuple);
#else
            dict.setItem(Py::Int((int)index), tuple);
#endif
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
            builder.addText(box.CalcCenter(), szBuf);
        }
        builder.addSingleArrow(pnt-20.0f*dir, pnt+10.0f*dir);
        builder.close();
        str.close();
#endif

        return Py::new_reference_to(dict);
    }
    catch (const Py::Exception&) {
        return 0;
    }
}

PyObject*  MeshPy::getPlanarSegments(PyObject *args)
{
    float dev;
    unsigned long minFacets=0;
    if (!PyArg_ParseTuple(args, "f|k",&dev,&minFacets))
        return NULL;

    Mesh::MeshObject* mesh = getMeshObjectPtr();
    std::vector<Mesh::Segment> segments = mesh->getSegmentsOfType
        (Mesh::MeshObject::PLANE, dev, minFacets);

    Py::List s;
    for (std::vector<Mesh::Segment>::iterator it = segments.begin(); it != segments.end(); ++it) {
        const std::vector<unsigned long>& segm = it->getIndices();
        Py::List ary;
        for (std::vector<unsigned long>::const_iterator jt = segm.begin(); jt != segm.end(); ++jt) {
#if PY_MAJOR_VERSION >= 3
            ary.append(Py::Long((int)*jt));
#else
            ary.append(Py::Int((int)*jt));
#endif
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
        return NULL;

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
        const std::vector<unsigned long>& segm = it->getIndices();
        Py::List ary;
        for (std::vector<unsigned long>::const_iterator jt = segm.begin(); jt != segm.end(); ++jt) {
#if PY_MAJOR_VERSION >= 3
            ary.append(Py::Long((int)*jt));
#else
            ary.append(Py::Int((int)*jt));
#endif
        }
        s.append(ary);
    }

    return Py::new_reference_to(s);
}

PyObject*  MeshPy::getSegmentsByCurvature(PyObject *args)
{
    PyObject* l;
    if (!PyArg_ParseTuple(args, "O",&l))
        return NULL;

    const MeshCore::MeshKernel& kernel = getMeshObjectPtr()->getKernel();
    MeshCore::MeshSegmentAlgorithm finder(kernel);
    MeshCore::MeshCurvature meshCurv(kernel);
    meshCurv.ComputePerVertex();

    Py::Sequence func(l);
    std::vector<MeshCore::MeshSurfaceSegment*> segm;
    for (Py::Sequence::iterator it = func.begin(); it != func.end(); ++it) {
        Py::Tuple t(*it);
        float c1 = (float)Py::Float(t[0]);
        float c2 = (float)Py::Float(t[1]);
        float tol1 = (float)Py::Float(t[2]);
        float tol2 = (float)Py::Float(t[3]);
#if PY_MAJOR_VERSION >= 3
        int num = (int)Py::Long(t[4]);
#else
        int num = (int)Py::Int(t[4]);
#endif
        segm.push_back(new MeshCore::MeshCurvatureFreeformSegment(meshCurv.GetCurvature(), num, tol1, tol2, c1, c2));
    }

    finder.FindSegments(segm);

    Py::List list;
    for (std::vector<MeshCore::MeshSurfaceSegment*>::iterator segmIt = segm.begin(); segmIt != segm.end(); ++segmIt) {
        const std::vector<MeshCore::MeshSegment>& data = (*segmIt)->GetSegments();
        for (std::vector<MeshCore::MeshSegment>::const_iterator it = data.begin(); it != data.end(); ++it) {
            Py::List ary;
            for (MeshCore::MeshSegment::const_iterator jt = it->begin(); jt != it->end(); ++jt) {
#if PY_MAJOR_VERSION >= 3
                ary.append(Py::Long((int)*jt));
#else
                ary.append(Py::Int((int)*jt));
#endif
            }
            list.append(ary);
        }
        delete (*segmIt);
    }

    return Py::new_reference_to(list);
}

Py::Long MeshPy::getCountPoints(void) const
{
    return Py::Long((long)getMeshObjectPtr()->countPoints());
}

Py::Long MeshPy::getCountEdges(void) const
{
    return Py::Long((long)getMeshObjectPtr()->countEdges());
}

Py::Long MeshPy::getCountFacets(void) const
{
    return Py::Long((long)getMeshObjectPtr()->countFacets());
}

Py::Float MeshPy::getArea(void) const
{
    return Py::Float(getMeshObjectPtr()->getSurface());
}

Py::Float MeshPy::getVolume(void) const
{
    return Py::Float(getMeshObjectPtr()->getVolume());
}

PyObject *MeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int MeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

Py::List MeshPy::getPoints(void) const
{
    Py::List PointList;
    unsigned int Index=0;
    MeshObject* mesh = getMeshObjectPtr();
    for (MeshObject::const_point_iterator it = mesh->points_begin(); it != mesh->points_end(); ++it) {
        PointList.append(Py::Object(new MeshPointPy(new MeshPoint(*it,getMeshObjectPtr(),Index++)), true));
    }
    return PointList;
}

Py::List MeshPy::getFacets(void) const
{
    Py::List FacetList;
    MeshObject* mesh = getMeshObjectPtr();
    for (MeshObject::const_facet_iterator it = mesh->facets_begin(); it != mesh->facets_end(); ++it) {
        FacetList.append(Py::Object(new FacetPy(new Facet(*it)), true));
    }
    return FacetList;
}

Py::Tuple MeshPy::getTopology(void) const
{
    std::vector<Base::Vector3d> Points;
    std::vector<Data::ComplexGeoData::Facet> Facets;
    getMeshObjectPtr()->getFaces(Points, Facets, 0.0f);
    Py::Tuple tuple(2);
    Py::List vertex;
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin();
        it != Points.end(); ++it)
        vertex.append(Py::Object(new Base::VectorPy(*it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
        it = Facets.begin(); it != Facets.end(); ++it) {
        Py::Tuple f(3);
#if PY_MAJOR_VERSION >= 3
        f.setItem(0,Py::Long((int)it->I1));
        f.setItem(1,Py::Long((int)it->I2));
        f.setItem(2,Py::Long((int)it->I3));
#else
        f.setItem(0,Py::Int((int)it->I1));
        f.setItem(1,Py::Int((int)it->I2));
        f.setItem(2,Py::Int((int)it->I3));
#endif
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return tuple;
}


