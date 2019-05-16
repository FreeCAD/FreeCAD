/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>

# include <StdMeshers_Arithmetic1D.hxx>
# include <StdMeshers_AutomaticLength.hxx>
# include <StdMeshers_MaxLength.hxx>
# include <StdMeshers_LocalLength.hxx>
# include <StdMeshers_MaxElementArea.hxx>
# include <StdMeshers_NotConformAllowed.hxx>
# include <StdMeshers_QuadranglePreference.hxx>
# include <StdMeshers_Quadrangle_2D.hxx>
# include <StdMeshers_Regular_1D.hxx>
# include <StdMeshers_UseExisting_1D2D.hxx>
# include <StdMeshers_CompositeSegment_1D.hxx>
# include <StdMeshers_Deflection1D.hxx>
# include <StdMeshers_Hexa_3D.hxx>
# include <StdMeshers_LayerDistribution.hxx>
# include <StdMeshers_LengthFromEdges.hxx>
# include <StdMeshers_MaxElementVolume.hxx>
# include <StdMeshers_MEFISTO_2D.hxx>
# include <StdMeshers_NumberOfLayers.hxx>
# include <StdMeshers_NumberOfSegments.hxx>
# include <StdMeshers_Prism_3D.hxx>
# include <StdMeshers_Projection_1D.hxx>
# include <StdMeshers_Projection_2D.hxx>
# include <StdMeshers_Projection_3D.hxx>
# include <StdMeshers_QuadraticMesh.hxx>
# include <StdMeshers_RadialPrism_3D.hxx>
# include <StdMeshers_SegmentAroundVertex_0D.hxx>
# include <StdMeshers_ProjectionSource1D.hxx>
# include <StdMeshers_ProjectionSource2D.hxx>
# include <StdMeshers_ProjectionSource3D.hxx>
# include <StdMeshers_SegmentLengthAroundVertex.hxx>
# include <StdMeshers_StartEndLength.hxx>
# include <StdMeshers_CompositeHexa_3D.hxx>
# if SMESH_VERSION_MAJOR < 7
#  include <StdMeshers_TrianglePreference.hxx>
# endif
#endif

#include "HypothesisPy.h"
#include "FemMeshPy.h"
#include <Base/Interpreter.h>
#include <Mod/Part/App/TopoShapePy.h>


using namespace Fem;


HypothesisPy::HypothesisPy(boost::shared_ptr<SMESH_Hypothesis> h)
  : hyp(h)
{
}

HypothesisPy::~HypothesisPy()
{
}

// ----------------------------------------------------------------------------

template<class T>
void SMESH_HypothesisPy<T>::init_type(PyObject* module)
{
    // you must have overwritten the virtual functions
    SMESH_HypothesisPy<T>::behaviors().supportRepr();
    SMESH_HypothesisPy<T>::behaviors().supportGetattr();
    SMESH_HypothesisPy<T>::behaviors().supportSetattr();
    SMESH_HypothesisPy<T>::behaviors().set_tp_new(PyMake);

    SMESH_HypothesisPy::add_varargs_method("setLibName", &SMESH_HypothesisPy<T>::setLibName, "setLibName(String)");
    SMESH_HypothesisPy::add_varargs_method("getLibName", &SMESH_HypothesisPy<T>::getLibName, "String getLibName()");
#if SMESH_VERSION_MAJOR < 7
    SMESH_HypothesisPy::add_varargs_method("setParameters", &SMESH_HypothesisPy<T>::setParameters, "setParameters(String)");
    SMESH_HypothesisPy::add_varargs_method("getParameters", &SMESH_HypothesisPy<T>::getParameters, "String getParameters()");
    SMESH_HypothesisPy::add_varargs_method("setLastParameters", &SMESH_HypothesisPy<T>::setLastParameters, "setLastParameters(String)");
    SMESH_HypothesisPy::add_varargs_method("getLastParameters", &SMESH_HypothesisPy<T>::getLastParameters, "String getLastParameters()");
    SMESH_HypothesisPy::add_varargs_method("clearParameters", &SMESH_HypothesisPy<T>::clearParameters, "clearParameters()");
#endif
    SMESH_HypothesisPy::add_varargs_method("isAuxiliary", &SMESH_HypothesisPy<T>::isAuxiliary, "Bool isAuxiliary()");
    SMESH_HypothesisPy::add_varargs_method("setParametersByMesh", &SMESH_HypothesisPy<T>::setParametersByMesh, "setParametersByMesh(Mesh,Shape)");
    Base::Interpreter().addType(SMESH_HypothesisPy<T>::behaviors().type_object(),
        module,SMESH_HypothesisPy<T>::behaviors().getName());
}

template<class T>
SMESH_HypothesisPy<T>::SMESH_HypothesisPy(SMESH_Hypothesis* h) : hyp(h)
{
}

template<class T>
SMESH_HypothesisPy<T>::~SMESH_HypothesisPy()
{
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::getattr(const char *name)
{
    if (strcmp(name,"this") == 0)
        return Hypothesis(Py::asObject(new HypothesisPy(this->getHypothesis())));
    return Py::PythonExtension<T>::getattr(name);
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::repr()
{
    std::stringstream str;
    str << hyp->GetName() << ", " << hyp->GetID();
    return Py::String(str.str());
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::setLibName(const Py::Tuple& args)
{
    std::string libName = (std::string)Py::String(args[0]);
    hypothesis<SMESH_Hypothesis>()->SetLibName(libName.c_str());
    return Py::None();
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::getLibName(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::String(hypothesis<SMESH_Hypothesis>()->GetLibName());
}


#if SMESH_VERSION_MAJOR < 7 //////////////////////////////////////////////////////////
template<class T>
Py::Object SMESH_HypothesisPy<T>::setParameters(const Py::Tuple& args)
{
    std::string paramName = (std::string)Py::String(args[0]);
    hypothesis<SMESH_Hypothesis>()->SetParameters(paramName.c_str());
    return Py::None();
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::getParameters(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::String(hypothesis<SMESH_Hypothesis>()->GetParameters());
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::setLastParameters(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    std::string paramName = (std::string)Py::String(args[0]);
    hypothesis<SMESH_Hypothesis>()->SetLastParameters(paramName.c_str());
    return Py::None();
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::getLastParameters(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::String(hypothesis<SMESH_Hypothesis>()->GetLastParameters());
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::clearParameters(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    hypothesis<SMESH_Hypothesis>()->ClearParameters();
    return Py::None();
}
#endif //////////////////////////////////////////////////////////////////////////

template<class T>
Py::Object SMESH_HypothesisPy<T>::setParametersByMesh(const Py::Tuple& args)
{
    PyObject *mesh, *shape;
    if (!PyArg_ParseTuple(args.ptr(), "O!O!",
        &(Fem::FemMeshPy::Type), &mesh,
        &(Part::TopoShapePy::Type), &shape))
        throw Py::Exception();
    Fem::FemMesh* m = static_cast<Fem::FemMeshPy*>(mesh)->getFemMeshPtr();
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    return Py::Boolean(hypothesis<SMESH_Hypothesis>()->SetParametersByMesh(m->getSMesh(), s));
}

template<class T>
Py::Object SMESH_HypothesisPy<T>::isAuxiliary(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(hypothesis<SMESH_Hypothesis>()->IsAuxiliary());
}

template<class T>
PyObject *SMESH_HypothesisPy<T>::PyMake(struct _typeobject * /*type*/, PyObject * args, PyObject * /*kwds*/)
{
    int hypId;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "iO!",&hypId,&(FemMeshPy::Type),&obj))
        return 0;
    FemMesh* mesh = static_cast<FemMeshPy*>(obj)->getFemMeshPtr();
    return new T(hypId, 1, mesh->getGenerator());
}

// ----------------------------------------------------------------------------

void StdMeshers_Arithmetic1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Arithmetic1D");
    behaviors().doc("StdMeshers_Arithmetic1D");

    add_varargs_method("setLength", &StdMeshers_Arithmetic1DPy::setLength, "setLength()");
    add_varargs_method("getLength", &StdMeshers_Arithmetic1DPy::getLength, "getLength()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Arithmetic1DPy::StdMeshers_Arithmetic1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Arithmetic1D(hypId, studyId, gen))
{
}

StdMeshers_Arithmetic1DPy::~StdMeshers_Arithmetic1DPy()
{
}

Py::Object StdMeshers_Arithmetic1DPy::setLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_Arithmetic1D>()->
        SetLength((double)Py::Float(args[0]), (bool)Py::Boolean(args[1]));
    return Py::None();
}

Py::Object StdMeshers_Arithmetic1DPy::getLength(const Py::Tuple& args)
{
    int start;
    if (!PyArg_ParseTuple(args.ptr(), "i",&start))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_Arithmetic1D>()->
        GetLength(start ? true : false));
}

// ----------------------------------------------------------------------------

void StdMeshers_AutomaticLengthPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_AutomaticLength");
    behaviors().doc("StdMeshers_AutomaticLength");

    add_varargs_method("setFineness", &StdMeshers_AutomaticLengthPy::setFineness, "setFineness()");
    add_varargs_method("getFineness", &StdMeshers_AutomaticLengthPy::getFineness, "getFineness()");
    add_varargs_method("getLength", &StdMeshers_AutomaticLengthPy::getLength, "getLength()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_AutomaticLengthPy::StdMeshers_AutomaticLengthPy(int /*hypId*/, int /*studyId*/, SMESH_Gen* /*gen*/)
  : SMESH_HypothesisPyBase(0)
{
}

StdMeshers_AutomaticLengthPy::~StdMeshers_AutomaticLengthPy()
{
}

Py::Object StdMeshers_AutomaticLengthPy::setFineness(const Py::Tuple& args)
{
    double fine = (double)Py::Float(args[0]);
    hypothesis<StdMeshers_AutomaticLength>()->SetFineness(fine);
    return Py::None();
}

Py::Object StdMeshers_AutomaticLengthPy::getFineness(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_AutomaticLength>()->GetFineness());
}

namespace Py {
    typedef ExtensionObject<Fem::FemMeshPy>         FemMesh;
    typedef ExtensionObject<Part::TopoShapePy>      TopoShape;
    template<> bool FemMesh::accepts (PyObject *pyob) const
    {
        return (pyob && PyObject_TypeCheck(pyob, &(Fem::FemMeshPy::Type)));
    }
    template<> bool TopoShape::accepts (PyObject *pyob) const
    {
        return (pyob && PyObject_TypeCheck(pyob, &(Part::TopoShapePy::Type)));
    }
}

Py::Object StdMeshers_AutomaticLengthPy::getLength(const Py::Tuple& args)
{
    Py::FemMesh mesh(args[0]);
    Py::Object shape_or_double(args[1]);

    Fem::FemMesh* m = mesh.extensionObject()->getFemMeshPtr();
    if (shape_or_double.type() == Py::Float().type()) {
        double len = (double)Py::Float(shape_or_double);
        return Py::Float(hypothesis<StdMeshers_AutomaticLength>()->GetLength(m->getSMesh(),len));
    }
    else {
        Py::TopoShape shape(shape_or_double);
        const TopoDS_Shape& s = shape.extensionObject()->getTopoShapePtr()->getShape();
        return Py::Float(hypothesis<StdMeshers_AutomaticLength>()->GetLength(m->getSMesh(),s));
    }
}

// ----------------------------------------------------------------------------

void StdMeshers_NotConformAllowedPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_NotConformAllowed");
    behaviors().doc("StdMeshers_NotConformAllowed");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_NotConformAllowedPy::StdMeshers_NotConformAllowedPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_NotConformAllowed(hypId, studyId, gen))
{
}

StdMeshers_NotConformAllowedPy::~StdMeshers_NotConformAllowedPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_MaxLengthPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_MaxLength");
    behaviors().doc("StdMeshers_MaxLength");

    add_varargs_method("setLength", &StdMeshers_MaxLengthPy::setLength, "setLength()");
    add_varargs_method("getLength", &StdMeshers_MaxLengthPy::getLength, "getLength()");
    add_varargs_method("havePreestimatedLength", &StdMeshers_MaxLengthPy::havePreestimatedLength, "havePreestimatedLength()");
    add_varargs_method("getPreestimatedLength", &StdMeshers_MaxLengthPy::getPreestimatedLength, "getPreestimatedLength()");
    add_varargs_method("setPreestimatedLength", &StdMeshers_MaxLengthPy::setPreestimatedLength, "setPreestimatedLength()");
    add_varargs_method("setUsePreestimatedLength", &StdMeshers_MaxLengthPy::setUsePreestimatedLength, "setUsePreestimatedLength()");
    add_varargs_method("getUsePreestimatedLength", &StdMeshers_MaxLengthPy::getUsePreestimatedLength, "getUsePreestimatedLength()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_MaxLengthPy::StdMeshers_MaxLengthPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_MaxLength(hypId, studyId, gen))
{
}

StdMeshers_MaxLengthPy::~StdMeshers_MaxLengthPy()
{
}

Py::Object StdMeshers_MaxLengthPy::setLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_MaxLength>()->SetLength((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_MaxLengthPy::getLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_MaxLength>()->GetLength());
}

Py::Object StdMeshers_MaxLengthPy::havePreestimatedLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(hypothesis<StdMeshers_MaxLength>()->HavePreestimatedLength());
}

Py::Object StdMeshers_MaxLengthPy::getPreestimatedLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_MaxLength>()->GetPreestimatedLength());
}

Py::Object StdMeshers_MaxLengthPy::setPreestimatedLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_MaxLength>()->SetPreestimatedLength((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_MaxLengthPy::setUsePreestimatedLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_MaxLength>()->SetUsePreestimatedLength((bool)Py::Boolean(args[0]));
    return Py::None();
}

Py::Object StdMeshers_MaxLengthPy::getUsePreestimatedLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(hypothesis<StdMeshers_MaxLength>()->GetUsePreestimatedLength());
}

// ----------------------------------------------------------------------------

void StdMeshers_LocalLengthPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_LocalLength");
    behaviors().doc("StdMeshers_LocalLength");

    add_varargs_method("setLength", &StdMeshers_LocalLengthPy::setLength, "setLength()");
    add_varargs_method("getLength", &StdMeshers_LocalLengthPy::getLength, "getLength()");
    add_varargs_method("setPrecision", &StdMeshers_LocalLengthPy::setPrecision, "setPrecision()");
    add_varargs_method("getPrecision", &StdMeshers_LocalLengthPy::getPrecision, "getPrecision()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_LocalLengthPy::StdMeshers_LocalLengthPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_LocalLength(hypId, studyId, gen))
{
}

StdMeshers_LocalLengthPy::~StdMeshers_LocalLengthPy()
{
}

Py::Object StdMeshers_LocalLengthPy::setLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_LocalLength>()->SetLength((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_LocalLengthPy::getLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_LocalLength>()->GetLength());
}

Py::Object StdMeshers_LocalLengthPy::setPrecision(const Py::Tuple& args)
{
    hypothesis<StdMeshers_LocalLength>()->SetPrecision((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_LocalLengthPy::getPrecision(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_LocalLength>()->GetPrecision());
}

// ----------------------------------------------------------------------------

void StdMeshers_MaxElementAreaPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_MaxElementArea");
    behaviors().doc("StdMeshers_MaxElementArea");

    add_varargs_method("setMaxArea", &StdMeshers_MaxElementAreaPy::setMaxArea, "setMaxArea()");
    add_varargs_method("getMaxArea", &StdMeshers_MaxElementAreaPy::getMaxArea, "getMaxArea()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_MaxElementAreaPy::StdMeshers_MaxElementAreaPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_MaxElementArea(hypId, studyId, gen))
{
}

StdMeshers_MaxElementAreaPy::~StdMeshers_MaxElementAreaPy()
{
}

Py::Object StdMeshers_MaxElementAreaPy::setMaxArea(const Py::Tuple& args)
{
    hypothesis<StdMeshers_MaxElementArea>()->SetMaxArea((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_MaxElementAreaPy::getMaxArea(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_MaxElementArea>()->GetMaxArea());
}

// ----------------------------------------------------------------------------

void StdMeshers_QuadranglePreferencePy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_QuadranglePreference");
    behaviors().doc("StdMeshers_QuadranglePreference");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_QuadranglePreferencePy::StdMeshers_QuadranglePreferencePy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_QuadranglePreference(hypId, studyId, gen))
{
}

StdMeshers_QuadranglePreferencePy::~StdMeshers_QuadranglePreferencePy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Quadrangle_2DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Quadrangle_2D");
    behaviors().doc("StdMeshers_Quadrangle_2D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Quadrangle_2DPy::StdMeshers_Quadrangle_2DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Quadrangle_2D(hypId, studyId, gen))
{
}

StdMeshers_Quadrangle_2DPy::~StdMeshers_Quadrangle_2DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Regular_1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Regular_1D");
    behaviors().doc("StdMeshers_Regular_1D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Regular_1DPy::StdMeshers_Regular_1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Regular_1D(hypId, studyId, gen))
{
}

StdMeshers_Regular_1DPy::~StdMeshers_Regular_1DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_UseExisting_1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_UseExisting_1D");
    behaviors().doc("StdMeshers_UseExisting_1D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_UseExisting_1DPy::StdMeshers_UseExisting_1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_UseExisting_1D(hypId, studyId, gen))
{
}

StdMeshers_UseExisting_1DPy::~StdMeshers_UseExisting_1DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_UseExisting_2DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_UseExisting_2D");
    behaviors().doc("StdMeshers_UseExisting_2D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_UseExisting_2DPy::StdMeshers_UseExisting_2DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_UseExisting_2D(hypId, studyId, gen))
{
}

StdMeshers_UseExisting_2DPy::~StdMeshers_UseExisting_2DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_CompositeSegment_1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_CompositeSegment_1D");
    behaviors().doc("StdMeshers_CompositeSegment_1D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_CompositeSegment_1DPy::StdMeshers_CompositeSegment_1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_CompositeSegment_1D(hypId, studyId, gen))
{
}

StdMeshers_CompositeSegment_1DPy::~StdMeshers_CompositeSegment_1DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Deflection1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Deflection1D");
    behaviors().doc("StdMeshers_Deflection1D");

    add_varargs_method("setDeflection", &StdMeshers_Deflection1DPy::setDeflection, "setDeflection()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Deflection1DPy::StdMeshers_Deflection1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Deflection1D(hypId, studyId, gen))
{
}

StdMeshers_Deflection1DPy::~StdMeshers_Deflection1DPy()
{
}

Py::Object StdMeshers_Deflection1DPy::setDeflection(const Py::Tuple& args)
{
    double fine = (double)Py::Float(args[0]);
    hypothesis<StdMeshers_Deflection1D>()->SetDeflection(fine);
    return Py::None();
}

// ----------------------------------------------------------------------------

void StdMeshers_Hexa_3DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Hexa_3D");
    behaviors().doc("StdMeshers_Hexa_3D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Hexa_3DPy::StdMeshers_Hexa_3DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Hexa_3D(hypId, studyId, gen))
{
}

StdMeshers_Hexa_3DPy::~StdMeshers_Hexa_3DPy()
{
}

// ----------------------------------------------------------------------------

#if SMESH_VERSION_MAJOR < 7 ///////////////////////////////////////////////////////////
void StdMeshers_TrianglePreferencePy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_TrianglePreference");
    behaviors().doc("StdMeshers_TrianglePreference");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_TrianglePreferencePy::StdMeshers_TrianglePreferencePy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_TrianglePreference(hypId, studyId, gen))
{
}

StdMeshers_TrianglePreferencePy::~StdMeshers_TrianglePreferencePy()
{
}
#endif ///////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------

void StdMeshers_StartEndLengthPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_StartEndLength");
    behaviors().doc("StdMeshers_StartEndLength");
    add_varargs_method("setLength", &StdMeshers_StartEndLengthPy::setLength, "setLength()");
    add_varargs_method("getLength", &StdMeshers_StartEndLengthPy::getLength, "getLength()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_StartEndLengthPy::StdMeshers_StartEndLengthPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_StartEndLength(hypId, studyId, gen))
{
}

StdMeshers_StartEndLengthPy::~StdMeshers_StartEndLengthPy()
{
}

Py::Object StdMeshers_StartEndLengthPy::setLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_StartEndLength>()->SetLength((double)Py::Float(args[0]),(bool)Py::Boolean(args[1]));
    return Py::None();
}

Py::Object StdMeshers_StartEndLengthPy::getLength(const Py::Tuple& args)
{
    return Py::Float(hypothesis<StdMeshers_StartEndLength>()->GetLength((bool)Py::Boolean(args[0])));
}

// ----------------------------------------------------------------------------

void StdMeshers_SegmentLengthAroundVertexPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_SegmentLengthAroundVertex");
    behaviors().doc("StdMeshers_SegmentLengthAroundVertex");
    add_varargs_method("setLength", &StdMeshers_SegmentLengthAroundVertexPy::setLength, "setLength()");
    add_varargs_method("getLength", &StdMeshers_SegmentLengthAroundVertexPy::getLength, "getLength()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_SegmentLengthAroundVertexPy::StdMeshers_SegmentLengthAroundVertexPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_SegmentLengthAroundVertex(hypId, studyId, gen))
{
}

StdMeshers_SegmentLengthAroundVertexPy::~StdMeshers_SegmentLengthAroundVertexPy()
{
}

Py::Object StdMeshers_SegmentLengthAroundVertexPy::setLength(const Py::Tuple& args)
{
    hypothesis<StdMeshers_SegmentLengthAroundVertex>()->SetLength((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_SegmentLengthAroundVertexPy::getLength(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_SegmentLengthAroundVertex>()->GetLength());
}

// ----------------------------------------------------------------------------

void StdMeshers_SegmentAroundVertex_0DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_SegmentAroundVertex_0D");
    behaviors().doc("StdMeshers_SegmentAroundVertex_0D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_SegmentAroundVertex_0DPy::StdMeshers_SegmentAroundVertex_0DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_SegmentAroundVertex_0D(hypId, studyId, gen))
{
}

StdMeshers_SegmentAroundVertex_0DPy::~StdMeshers_SegmentAroundVertex_0DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_RadialPrism_3DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_RadialPrism_3D");
    behaviors().doc("StdMeshers_RadialPrism_3D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_RadialPrism_3DPy::StdMeshers_RadialPrism_3DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_RadialPrism_3D(hypId, studyId, gen))
{
}

StdMeshers_RadialPrism_3DPy::~StdMeshers_RadialPrism_3DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_QuadraticMeshPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_QuadraticMesh");
    behaviors().doc("StdMeshers_QuadraticMesh");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_QuadraticMeshPy::StdMeshers_QuadraticMeshPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_QuadraticMesh(hypId, studyId, gen))
{
}

StdMeshers_QuadraticMeshPy::~StdMeshers_QuadraticMeshPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_ProjectionSource3DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_ProjectionSource3D");
    behaviors().doc("StdMeshers_ProjectionSource3D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_ProjectionSource3DPy::StdMeshers_ProjectionSource3DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_ProjectionSource3D(hypId, studyId, gen))
{
}

StdMeshers_ProjectionSource3DPy::~StdMeshers_ProjectionSource3DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_ProjectionSource2DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_ProjectionSource2D");
    behaviors().doc("StdMeshers_ProjectionSource2D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_ProjectionSource2DPy::StdMeshers_ProjectionSource2DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_ProjectionSource2D(hypId, studyId, gen))
{
}

StdMeshers_ProjectionSource2DPy::~StdMeshers_ProjectionSource2DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_ProjectionSource1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_ProjectionSource1D");
    behaviors().doc("StdMeshers_ProjectionSource1D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_ProjectionSource1DPy::StdMeshers_ProjectionSource1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_ProjectionSource1D(hypId, studyId, gen))
{
}

StdMeshers_ProjectionSource1DPy::~StdMeshers_ProjectionSource1DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Projection_3DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Projection_3D");
    behaviors().doc("StdMeshers_Projection_3D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Projection_3DPy::StdMeshers_Projection_3DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Projection_3D(hypId, studyId, gen))
{
}

StdMeshers_Projection_3DPy::~StdMeshers_Projection_3DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Projection_2DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Projection_2D");
    behaviors().doc("StdMeshers_Projection_2D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Projection_2DPy::StdMeshers_Projection_2DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Projection_2D(hypId, studyId, gen))
{
}

StdMeshers_Projection_2DPy::~StdMeshers_Projection_2DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Projection_1DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Projection_1D");
    behaviors().doc("StdMeshers_Projection_1D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Projection_1DPy::StdMeshers_Projection_1DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Projection_1D(hypId, studyId, gen))
{
}

StdMeshers_Projection_1DPy::~StdMeshers_Projection_1DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_Prism_3DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_Prism_3D");
    behaviors().doc("StdMeshers_Prism_3D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_Prism_3DPy::StdMeshers_Prism_3DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_Prism_3D(hypId, studyId, gen))
{
}

StdMeshers_Prism_3DPy::~StdMeshers_Prism_3DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_NumberOfSegmentsPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_NumberOfSegments");
    behaviors().doc("StdMeshers_NumberOfSegments");
    add_varargs_method("setNumberOfSegments",&StdMeshers_NumberOfSegmentsPy::setNumSegm,"setNumberOfSegments()");
    add_varargs_method("getNumberOfSegments",&StdMeshers_NumberOfSegmentsPy::getNumSegm,"getNumberOfSegments()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_NumberOfSegmentsPy::StdMeshers_NumberOfSegmentsPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_NumberOfSegments(hypId, studyId, gen))
{
}

StdMeshers_NumberOfSegmentsPy::~StdMeshers_NumberOfSegmentsPy()
{
}

Py::Object StdMeshers_NumberOfSegmentsPy::setNumSegm(const Py::Tuple& args)
{
#if PY_MAJOR_VERSION >= 3
    hypothesis<StdMeshers_NumberOfSegments>()->SetNumberOfSegments((int)Py::Long(args[0]));
#else
    hypothesis<StdMeshers_NumberOfSegments>()->SetNumberOfSegments((int)Py::Int(args[0]));
#endif
    return Py::None();
}

Py::Object StdMeshers_NumberOfSegmentsPy::getNumSegm(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Long(hypothesis<StdMeshers_NumberOfSegments>()->GetNumberOfSegments());
}

// ----------------------------------------------------------------------------

void StdMeshers_NumberOfLayersPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_NumberOfLayers");
    behaviors().doc("StdMeshers_NumberOfLayers");
    add_varargs_method("setNumberOfLayers",&StdMeshers_NumberOfLayersPy::setNumLayers,"setNumberOfLayers()");
    add_varargs_method("getNumberOfLayers",&StdMeshers_NumberOfLayersPy::getNumLayers,"getNumberOfLayers()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_NumberOfLayersPy::StdMeshers_NumberOfLayersPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_NumberOfLayers(hypId, studyId, gen))
{
}

StdMeshers_NumberOfLayersPy::~StdMeshers_NumberOfLayersPy()
{
}

Py::Object StdMeshers_NumberOfLayersPy::setNumLayers(const Py::Tuple& args)
{
#if PY_MAJOR_VERSION >= 3
    hypothesis<StdMeshers_NumberOfLayers>()->SetNumberOfLayers((int)Py::Long(args[0]));
#else
    hypothesis<StdMeshers_NumberOfLayers>()->SetNumberOfLayers((int)Py::Int(args[0]));
#endif
    return Py::None();
}

Py::Object StdMeshers_NumberOfLayersPy::getNumLayers(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Long(hypothesis<StdMeshers_NumberOfLayers>()->GetNumberOfLayers());
}

// ----------------------------------------------------------------------------

void StdMeshers_MEFISTO_2DPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_MEFISTO_2D");
    behaviors().doc("StdMeshers_MEFISTO_2D");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_MEFISTO_2DPy::StdMeshers_MEFISTO_2DPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_MEFISTO_2D(hypId, studyId, gen))
{
}

StdMeshers_MEFISTO_2DPy::~StdMeshers_MEFISTO_2DPy()
{
}

// ----------------------------------------------------------------------------

void StdMeshers_MaxElementVolumePy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_MaxElementVolume");
    behaviors().doc("StdMeshers_MaxElementVolume");
    add_varargs_method("setMaxVolume",&StdMeshers_MaxElementVolumePy::setMaxVolume,"setMaxVolume()");
    add_varargs_method("getMaxVolume",&StdMeshers_MaxElementVolumePy::getMaxVolume,"getMaxVolume()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_MaxElementVolumePy::StdMeshers_MaxElementVolumePy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_MaxElementVolume(hypId, studyId, gen))
{
}

StdMeshers_MaxElementVolumePy::~StdMeshers_MaxElementVolumePy()
{
}

Py::Object StdMeshers_MaxElementVolumePy::setMaxVolume(const Py::Tuple& args)
{
    hypothesis<StdMeshers_MaxElementVolume>()->SetMaxVolume((double)Py::Float(args[0]));
    return Py::None();
}

Py::Object StdMeshers_MaxElementVolumePy::getMaxVolume(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Float(hypothesis<StdMeshers_MaxElementVolume>()->GetMaxVolume());
}

// ----------------------------------------------------------------------------

void StdMeshers_LengthFromEdgesPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_LengthFromEdges");
    behaviors().doc("StdMeshers_LengthFromEdges");
    add_varargs_method("setMode",&StdMeshers_LengthFromEdgesPy::setMode,"setMode()");
    add_varargs_method("getMode",&StdMeshers_LengthFromEdgesPy::getMode,"getMode()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_LengthFromEdgesPy::StdMeshers_LengthFromEdgesPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_LengthFromEdges(hypId, studyId, gen))
{
}

StdMeshers_LengthFromEdgesPy::~StdMeshers_LengthFromEdgesPy()
{
}

Py::Object StdMeshers_LengthFromEdgesPy::setMode(const Py::Tuple& args)
{
#if PY_MAJOR_VERSION >= 3
    hypothesis<StdMeshers_LengthFromEdges>()->SetMode((int)Py::Long(args[0]));
#else
    hypothesis<StdMeshers_LengthFromEdges>()->SetMode((int)Py::Int(args[0]));
#endif
    return Py::None();
}

Py::Object StdMeshers_LengthFromEdgesPy::getMode(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Long(hypothesis<StdMeshers_LengthFromEdges>()->GetMode());
}

// ----------------------------------------------------------------------------

void StdMeshers_LayerDistributionPy::init_type(PyObject* module)
{
    behaviors().name("StdMeshers_LayerDistribution");
    behaviors().doc("StdMeshers_LayerDistribution");
    add_varargs_method("setLayerDistribution",
        &StdMeshers_LayerDistributionPy::setLayerDistribution,
        "setLayerDistribution()");
    add_varargs_method("getLayerDistribution",
        &StdMeshers_LayerDistributionPy::getLayerDistribution,
        "getLayerDistribution()");
    SMESH_HypothesisPyBase::init_type(module);
}

StdMeshers_LayerDistributionPy::StdMeshers_LayerDistributionPy(int hypId, int studyId, SMESH_Gen* gen)
  : SMESH_HypothesisPyBase(new StdMeshers_LayerDistribution(hypId, studyId, gen))
{
}

StdMeshers_LayerDistributionPy::~StdMeshers_LayerDistributionPy()
{
}

Py::Object StdMeshers_LayerDistributionPy::setLayerDistribution(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::None();
}

Py::Object StdMeshers_LayerDistributionPy::getLayerDistribution(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    //return hypothesis<StdMeshers_LayerDistribution>()->GetLayerDistribution();
    return Py::None();
}
