/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef FEM_PRECOMPILED_H
#define FEM_PRECOMPILED_H

#include <FCConfig.h>

#ifdef _MSC_VER
#pragma warning(disable : 4290)
#pragma warning(disable : 4275)
#endif

#ifdef _PreComp_

// standard
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Boost
#include <boost/assign/list_of.hpp>
#include <boost/tokenizer.hpp>

#include <Python.h>
#include <QFileInfo>

// Salomesh
#include <SMDSAbs_ElementType.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_MeshGroup.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMESHDS_Group.hxx>
#include <SMESHDS_GroupBase.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#include <SMESH_Version.h>

#include <StdMeshers_Arithmetic1D.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_CompositeSegment_1D.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_Hexa_3D.hxx>
#include <StdMeshers_LayerDistribution.hxx>
#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_LocalLength.hxx>
#if SMESH_VERSION_MAJOR <= 9 && SMESH_VERSION_MINOR < 10
#include <StdMeshers_MEFISTO_2D.hxx>
#endif
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_MaxElementVolume.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#include <StdMeshers_NumberOfLayers.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_Prism_3D.hxx>
#include <StdMeshers_ProjectionSource1D.hxx>
#include <StdMeshers_ProjectionSource2D.hxx>
#include <StdMeshers_ProjectionSource3D.hxx>
#include <StdMeshers_Projection_1D.hxx>
#include <StdMeshers_Projection_2D.hxx>
#include <StdMeshers_Projection_3D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_QuadraticMesh.hxx>
#include <StdMeshers_RadialPrism_3D.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_SegmentAroundVertex_0D.hxx>
#include <StdMeshers_SegmentLengthAroundVertex.hxx>
#include <StdMeshers_StartEndLength.hxx>
#include <StdMeshers_UseExisting_1D2D.hxx>

// Opencascade
#include <Adaptor3d_IsoCurve.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HSurface.hxx>
#endif
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepTools.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Precision.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <Standard_Real.hxx>
#include <Standard_Version.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

// VTK
#include <vtkAppendFilter.h>
#include <vtkCellArray.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataArray.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
#include <vtkDoubleArray.h>
#include <vtkHexahedron.h>
#include <vtkIdList.h>
#include <vtkImageData.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPyramid.h>
#include <vtkQuad.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticWedge.h>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWedge.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

// Netgen
#ifdef FCWithNetgen
#include <NETGENPlugin_Hypothesis.hxx>
#include <NETGENPlugin_Mesher.hxx>
#include <NETGENPlugin_SimpleHypothesis_3D.hxx>
#endif

#endif  // _PreComp_
#endif
