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

#ifndef FEM_HYPOTHESISPY_H
#define FEM_HYPOTHESISPY_H

#include <CXX/Extensions.hxx>
#include <SMESH_Version.h>  // needed for SMESH_VERSION_MAJOR
#include <memory>

class SMESH_Hypothesis;
class SMESH_Gen;

namespace Fem
{

class HypothesisPy: public Py::PythonExtension<HypothesisPy>
{
public:
    using HypothesisPyBase = Py::PythonExtension<HypothesisPy>;
    explicit HypothesisPy(std::shared_ptr<SMESH_Hypothesis>);
    ~HypothesisPy() override;
    std::shared_ptr<SMESH_Hypothesis> getHypothesis() const
    {
        return hyp;
    }

private:
    std::shared_ptr<SMESH_Hypothesis> hyp;
};

using Hypothesis = Py::ExtensionObject<HypothesisPy>;

template<class T>
class SMESH_HypothesisPy: public Py::PythonExtension<T>
{
public:
    using SMESH_HypothesisPyBase = SMESH_HypothesisPy<T>;
    static void init_type(PyObject*);  // announce properties and methods

    explicit SMESH_HypothesisPy(SMESH_Hypothesis*);
    ~SMESH_HypothesisPy() override;

    Py::Object getattr(const char* name) override;
    Py::Object repr() override;
    Py::Object getLibName(const Py::Tuple& args);
    Py::Object setLibName(const Py::Tuple& args);
    Py::Object isAuxiliary(const Py::Tuple& args);
    Py::Object setParametersByMesh(const Py::Tuple& args);

    std::shared_ptr<SMESH_Hypothesis> getHypothesis() const
    {
        return hyp;
    }

protected:
    template<typename type>
    type* hypothesis() const
    {
        return static_cast<type*>(hyp.get());
    }

private:
    static PyObject* PyMake(struct _typeobject*, PyObject*, PyObject*);

private:
    std::shared_ptr<SMESH_Hypothesis> hyp;
};

#if SMESH_VERSION_MAJOR >= 9
class StdMeshers_Arithmetic1DPy: public SMESH_HypothesisPy<StdMeshers_Arithmetic1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Arithmetic1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Arithmetic1DPy();

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_AutomaticLengthPy: public SMESH_HypothesisPy<StdMeshers_AutomaticLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_AutomaticLengthPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_AutomaticLengthPy();

    Py::Object setFineness(const Py::Tuple& args);
    Py::Object getFineness(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_NotConformAllowedPy: public SMESH_HypothesisPy<StdMeshers_NotConformAllowedPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NotConformAllowedPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_NotConformAllowedPy();
};

class StdMeshers_MaxLengthPy: public SMESH_HypothesisPy<StdMeshers_MaxLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxLengthPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_MaxLengthPy();

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
    Py::Object havePreestimatedLength(const Py::Tuple& args);
    Py::Object getPreestimatedLength(const Py::Tuple& args);
    Py::Object setPreestimatedLength(const Py::Tuple& args);
    Py::Object setUsePreestimatedLength(const Py::Tuple& args);
    Py::Object getUsePreestimatedLength(const Py::Tuple& args);
};

class StdMeshers_LocalLengthPy: public SMESH_HypothesisPy<StdMeshers_LocalLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LocalLengthPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_LocalLengthPy();

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
    Py::Object setPrecision(const Py::Tuple& args);
    Py::Object getPrecision(const Py::Tuple& args);
};

class StdMeshers_MaxElementAreaPy: public SMESH_HypothesisPy<StdMeshers_MaxElementAreaPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxElementAreaPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_MaxElementAreaPy();

    Py::Object setMaxArea(const Py::Tuple& args);
    Py::Object getMaxArea(const Py::Tuple& args);
};

class StdMeshers_QuadranglePreferencePy
    : public SMESH_HypothesisPy<StdMeshers_QuadranglePreferencePy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_QuadranglePreferencePy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_QuadranglePreferencePy();
};

class StdMeshers_Quadrangle_2DPy: public SMESH_HypothesisPy<StdMeshers_Quadrangle_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Quadrangle_2DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Quadrangle_2DPy();
};

class StdMeshers_Regular_1DPy: public SMESH_HypothesisPy<StdMeshers_Regular_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Regular_1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Regular_1DPy();
};

class StdMeshers_UseExisting_1DPy: public SMESH_HypothesisPy<StdMeshers_UseExisting_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_UseExisting_1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_UseExisting_1DPy();
};

class StdMeshers_UseExisting_2DPy: public SMESH_HypothesisPy<StdMeshers_UseExisting_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_UseExisting_2DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_UseExisting_2DPy();
};

class StdMeshers_CompositeSegment_1DPy: public SMESH_HypothesisPy<StdMeshers_CompositeSegment_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_CompositeSegment_1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_CompositeSegment_1DPy();
};

class StdMeshers_Deflection1DPy: public SMESH_HypothesisPy<StdMeshers_Deflection1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Deflection1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Deflection1DPy();

    Py::Object setDeflection(const Py::Tuple& args);
};

class StdMeshers_Hexa_3DPy: public SMESH_HypothesisPy<StdMeshers_Hexa_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Hexa_3DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Hexa_3DPy();
};

class StdMeshers_StartEndLengthPy: public SMESH_HypothesisPy<StdMeshers_StartEndLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_StartEndLengthPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_StartEndLengthPy();
    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_SegmentLengthAroundVertexPy
    : public SMESH_HypothesisPy<StdMeshers_SegmentLengthAroundVertexPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_SegmentLengthAroundVertexPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_SegmentLengthAroundVertexPy();
    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_SegmentAroundVertex_0DPy
    : public SMESH_HypothesisPy<StdMeshers_SegmentAroundVertex_0DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_SegmentAroundVertex_0DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_SegmentAroundVertex_0DPy();
};

class StdMeshers_RadialPrism_3DPy: public SMESH_HypothesisPy<StdMeshers_RadialPrism_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_RadialPrism_3DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_RadialPrism_3DPy();
};

class StdMeshers_QuadraticMeshPy: public SMESH_HypothesisPy<StdMeshers_QuadraticMeshPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_QuadraticMeshPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_QuadraticMeshPy();
};

class StdMeshers_ProjectionSource3DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource3DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource3DPy();
};

class StdMeshers_ProjectionSource2DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource2DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource2DPy();
};

class StdMeshers_ProjectionSource1DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource1DPy();
};

class StdMeshers_Projection_3DPy: public SMESH_HypothesisPy<StdMeshers_Projection_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_3DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Projection_3DPy();
};

class StdMeshers_Projection_2DPy: public SMESH_HypothesisPy<StdMeshers_Projection_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_2DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Projection_2DPy();
};

class StdMeshers_Projection_1DPy: public SMESH_HypothesisPy<StdMeshers_Projection_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_1DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Projection_1DPy();
};

class StdMeshers_Prism_3DPy: public SMESH_HypothesisPy<StdMeshers_Prism_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Prism_3DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_Prism_3DPy();
};

class StdMeshers_NumberOfSegmentsPy: public SMESH_HypothesisPy<StdMeshers_NumberOfSegmentsPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NumberOfSegmentsPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_NumberOfSegmentsPy();
    Py::Object setNumSegm(const Py::Tuple& args);
    Py::Object getNumSegm(const Py::Tuple& args);
};

class StdMeshers_NumberOfLayersPy: public SMESH_HypothesisPy<StdMeshers_NumberOfLayersPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NumberOfLayersPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_NumberOfLayersPy();
    Py::Object setNumLayers(const Py::Tuple& args);
    Py::Object getNumLayers(const Py::Tuple& args);
};

#if SMESH_VERSION_MAJOR <= 9 && SMESH_VERSION_MINOR < 10
class StdMeshers_MEFISTO_2DPy: public SMESH_HypothesisPy<StdMeshers_MEFISTO_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MEFISTO_2DPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_MEFISTO_2DPy();
};
#endif

class StdMeshers_MaxElementVolumePy: public SMESH_HypothesisPy<StdMeshers_MaxElementVolumePy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxElementVolumePy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_MaxElementVolumePy();
    Py::Object setMaxVolume(const Py::Tuple& args);
    Py::Object getMaxVolume(const Py::Tuple& args);
};

class StdMeshers_LengthFromEdgesPy: public SMESH_HypothesisPy<StdMeshers_LengthFromEdgesPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LengthFromEdgesPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_LengthFromEdgesPy();
    Py::Object setMode(const Py::Tuple& args);
    Py::Object getMode(const Py::Tuple& args);
};

class StdMeshers_LayerDistributionPy: public SMESH_HypothesisPy<StdMeshers_LayerDistributionPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LayerDistributionPy(int hypId, SMESH_Gen* gen);
    ~StdMeshers_LayerDistributionPy();
    Py::Object setLayerDistribution(const Py::Tuple& args);
    Py::Object getLayerDistribution(const Py::Tuple& args);
};
#else
class StdMeshers_Arithmetic1DPy: public SMESH_HypothesisPy<StdMeshers_Arithmetic1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Arithmetic1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Arithmetic1DPy() override;

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_AutomaticLengthPy: public SMESH_HypothesisPy<StdMeshers_AutomaticLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_AutomaticLengthPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_AutomaticLengthPy() override;

    Py::Object setFineness(const Py::Tuple& args);
    Py::Object getFineness(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_NotConformAllowedPy: public SMESH_HypothesisPy<StdMeshers_NotConformAllowedPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NotConformAllowedPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_NotConformAllowedPy() override;
};

class StdMeshers_MaxLengthPy: public SMESH_HypothesisPy<StdMeshers_MaxLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxLengthPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_MaxLengthPy() override;

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
    Py::Object havePreestimatedLength(const Py::Tuple& args);
    Py::Object getPreestimatedLength(const Py::Tuple& args);
    Py::Object setPreestimatedLength(const Py::Tuple& args);
    Py::Object setUsePreestimatedLength(const Py::Tuple& args);
    Py::Object getUsePreestimatedLength(const Py::Tuple& args);
};

class StdMeshers_LocalLengthPy: public SMESH_HypothesisPy<StdMeshers_LocalLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LocalLengthPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_LocalLengthPy() override;

    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
    Py::Object setPrecision(const Py::Tuple& args);
    Py::Object getPrecision(const Py::Tuple& args);
};

class StdMeshers_MaxElementAreaPy: public SMESH_HypothesisPy<StdMeshers_MaxElementAreaPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxElementAreaPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_MaxElementAreaPy() override;

    Py::Object setMaxArea(const Py::Tuple& args);
    Py::Object getMaxArea(const Py::Tuple& args);
};

class StdMeshers_QuadranglePreferencePy
    : public SMESH_HypothesisPy<StdMeshers_QuadranglePreferencePy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_QuadranglePreferencePy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_QuadranglePreferencePy() override;
};

class StdMeshers_Quadrangle_2DPy: public SMESH_HypothesisPy<StdMeshers_Quadrangle_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Quadrangle_2DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Quadrangle_2DPy() override;
};

class StdMeshers_Regular_1DPy: public SMESH_HypothesisPy<StdMeshers_Regular_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Regular_1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Regular_1DPy() override;
};

class StdMeshers_UseExisting_1DPy: public SMESH_HypothesisPy<StdMeshers_UseExisting_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_UseExisting_1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_UseExisting_1DPy() override;
};

class StdMeshers_UseExisting_2DPy: public SMESH_HypothesisPy<StdMeshers_UseExisting_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_UseExisting_2DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_UseExisting_2DPy() override;
};

class StdMeshers_CompositeSegment_1DPy: public SMESH_HypothesisPy<StdMeshers_CompositeSegment_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_CompositeSegment_1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_CompositeSegment_1DPy() override;
};

class StdMeshers_Deflection1DPy: public SMESH_HypothesisPy<StdMeshers_Deflection1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Deflection1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Deflection1DPy() override;

    Py::Object setDeflection(const Py::Tuple& args);
};

class StdMeshers_Hexa_3DPy: public SMESH_HypothesisPy<StdMeshers_Hexa_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Hexa_3DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Hexa_3DPy() override;
};
#if SMESH_VERSION_MAJOR < 7  // -----------------------------------------------
class StdMeshers_TrianglePreferencePy: public SMESH_HypothesisPy<StdMeshers_TrianglePreferencePy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_TrianglePreferencePy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_TrianglePreferencePy();
};
#endif                       // --------------------------------------------------------------------

class StdMeshers_StartEndLengthPy: public SMESH_HypothesisPy<StdMeshers_StartEndLengthPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_StartEndLengthPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_StartEndLengthPy() override;
    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_SegmentLengthAroundVertexPy
    : public SMESH_HypothesisPy<StdMeshers_SegmentLengthAroundVertexPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_SegmentLengthAroundVertexPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_SegmentLengthAroundVertexPy() override;
    Py::Object setLength(const Py::Tuple& args);
    Py::Object getLength(const Py::Tuple& args);
};

class StdMeshers_SegmentAroundVertex_0DPy
    : public SMESH_HypothesisPy<StdMeshers_SegmentAroundVertex_0DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_SegmentAroundVertex_0DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_SegmentAroundVertex_0DPy() override;
};

class StdMeshers_RadialPrism_3DPy: public SMESH_HypothesisPy<StdMeshers_RadialPrism_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_RadialPrism_3DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_RadialPrism_3DPy() override;
};

class StdMeshers_QuadraticMeshPy: public SMESH_HypothesisPy<StdMeshers_QuadraticMeshPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_QuadraticMeshPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_QuadraticMeshPy() override;
};

class StdMeshers_ProjectionSource3DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource3DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource3DPy() override;
    // Py::Object setSource3DShape(const Py::Tuple& args);
    // Py::Object getSource3DShape(const Py::Tuple& args);
    // Py::Object setSourceMesh(const Py::Tuple& args);
    // Py::Object getSourceMesh(const Py::Tuple& args);
    // Py::Object setVertexAssociation(const Py::Tuple& args);
    // Py::Object getSourceVertex(const Py::Tuple& args);
    // Py::Object getTargetVertex(const Py::Tuple& args);
    // Py::Object hasVertexAssociation(const Py::Tuple& args);
    // Py::Object getStoreParams(const Py::Tuple& args);
    // Py::Object restoreParams(const Py::Tuple& args);
};

class StdMeshers_ProjectionSource2DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource2DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource2DPy() override;
    // Py::Object setSourceFace(const Py::Tuple& args);
    // Py::Object getSourceFace(const Py::Tuple& args);
    // Py::Object isCompoundSource(const Py::Tuple& args);
    // Py::Object setSourceMesh(const Py::Tuple& args);
    // Py::Object getSourceMesh(const Py::Tuple& args);
    // Py::Object setVertexAssociation(const Py::Tuple& args);
    // Py::Object getSourceVertex(const Py::Tuple& args);
    // Py::Object getTargetVertex(const Py::Tuple& args);
    // Py::Object hasVertexAssociation(const Py::Tuple& args);
    // Py::Object getStoreParams(const Py::Tuple& args);
    // Py::Object restoreParams(const Py::Tuple& args);
};

class StdMeshers_ProjectionSource1DPy: public SMESH_HypothesisPy<StdMeshers_ProjectionSource1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_ProjectionSource1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_ProjectionSource1DPy() override;
    // Py::Object setSourceEdge(const Py::Tuple& args);
    // Py::Object getSourceEdge(const Py::Tuple& args);
    // Py::Object isCompoundSource(const Py::Tuple& args);
    // Py::Object setSourceMesh(const Py::Tuple& args);
    // Py::Object getSourceMesh(const Py::Tuple& args);
    // Py::Object setVertexAssociation(const Py::Tuple& args);
    // Py::Object getSourceVertex(const Py::Tuple& args);
    // Py::Object getTargetVertex(const Py::Tuple& args);
    // Py::Object hasVertexAssociation(const Py::Tuple& args);
    // Py::Object getStoreParams(const Py::Tuple& args);
    // Py::Object restoreParams(const Py::Tuple& args);
};

class StdMeshers_Projection_3DPy: public SMESH_HypothesisPy<StdMeshers_Projection_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_3DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Projection_3DPy() override;
};

class StdMeshers_Projection_2DPy: public SMESH_HypothesisPy<StdMeshers_Projection_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_2DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Projection_2DPy() override;
};

class StdMeshers_Projection_1DPy: public SMESH_HypothesisPy<StdMeshers_Projection_1DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Projection_1DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Projection_1DPy() override;
};

class StdMeshers_Prism_3DPy: public SMESH_HypothesisPy<StdMeshers_Prism_3DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_Prism_3DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_Prism_3DPy() override;
};

class StdMeshers_NumberOfSegmentsPy: public SMESH_HypothesisPy<StdMeshers_NumberOfSegmentsPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NumberOfSegmentsPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_NumberOfSegmentsPy() override;
    Py::Object setNumSegm(const Py::Tuple& args);
    Py::Object getNumSegm(const Py::Tuple& args);
};

class StdMeshers_NumberOfLayersPy: public SMESH_HypothesisPy<StdMeshers_NumberOfLayersPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_NumberOfLayersPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_NumberOfLayersPy() override;
    Py::Object setNumLayers(const Py::Tuple& args);
    Py::Object getNumLayers(const Py::Tuple& args);
};

#if SMESH_VERSION_MAJOR <= 9 && SMESH_VERSION_MINOR < 10
class StdMeshers_MEFISTO_2DPy: public SMESH_HypothesisPy<StdMeshers_MEFISTO_2DPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MEFISTO_2DPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_MEFISTO_2DPy() override;
};
#endif

class StdMeshers_MaxElementVolumePy: public SMESH_HypothesisPy<StdMeshers_MaxElementVolumePy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_MaxElementVolumePy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_MaxElementVolumePy() override;
    Py::Object setMaxVolume(const Py::Tuple& args);
    Py::Object getMaxVolume(const Py::Tuple& args);
};

class StdMeshers_LengthFromEdgesPy: public SMESH_HypothesisPy<StdMeshers_LengthFromEdgesPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LengthFromEdgesPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_LengthFromEdgesPy() override;
    Py::Object setMode(const Py::Tuple& args);
    Py::Object getMode(const Py::Tuple& args);
};

class StdMeshers_LayerDistributionPy: public SMESH_HypothesisPy<StdMeshers_LayerDistributionPy>
{
public:
    static void init_type(PyObject*);
    StdMeshers_LayerDistributionPy(int hypId, int studyId, SMESH_Gen* gen);
    ~StdMeshers_LayerDistributionPy() override;
    Py::Object setLayerDistribution(const Py::Tuple& args);
    Py::Object getLayerDistribution(const Py::Tuple& args);
};
#endif

}  // namespace Fem

#endif  // FEM_HYPOTHESISPY_H
