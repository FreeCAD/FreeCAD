/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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

// Exporting of App classes
#ifdef FC_OS_WIN32
# define AppFemExport   __declspec(dllexport)
# define FemExport      __declspec(dllexport)
# define PartExport     __declspec(dllimport)
# define MeshExport     __declspec(dllimport)
# define BaseExport     __declspec(dllimport)
#else // for Linux
# define AppFemExport
# define FemExport
# define PartExport
# define MeshExport
# define BaseExport
#endif

#ifdef _MSC_VER
# pragma warning(disable : 4290)
# pragma warning(disable : 4275)
#endif

#ifdef _PreComp_

// standard
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <bitset>
#include <cstdlib>
#include <memory>
#include <cmath>

#include <math.h>

#include <algorithm>
#include <stdexcept>
// Python
#include <Python.h>

// Boost
#include <boost/assign/list_of.hpp>
#include <boost/tokenizer.hpp>

// Salomesh
#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDS_MeshNode.hxx>
#include <StdMeshers_MaxLength.hxx>
#include <StdMeshers_LocalLength.hxx>
#include <StdMeshers_NumberOfSegments.hxx>
#include <StdMeshers_AutomaticLength.hxx>
#include <StdMeshers_MEFISTO_2D.hxx>
#include <StdMeshers_Deflection1D.hxx>
#include <StdMeshers_MaxElementArea.hxx>
#include <StdMeshers_Regular_1D.hxx>
#include <StdMeshers_QuadranglePreference.hxx>
#include <StdMeshers_Quadrangle_2D.hxx>
#include <StdMeshers_LengthFromEdges.hxx>
#include <StdMeshers_NotConformAllowed.hxx>
#include <StdMeshers_Arithmetic1D.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESH_MeshEditor.hxx>
#include <SMDS_MeshGroup.hxx>
#include <SMESHDS_GroupBase.hxx>
#include <SMESHDS_Group.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMDS_VolumeTool.hxx>
#include <StdMeshers_StartEndLength.hxx>
#include <StdMeshers_QuadraticMesh.hxx>
#include <SMDSAbs_ElementType.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMESH_Gen.hxx>
#include <StdMeshers_UseExisting_1D2D.hxx>
#include <StdMeshers_CompositeSegment_1D.hxx>
#include <StdMeshers_Hexa_3D.hxx>
#include <StdMeshers_LayerDistribution.hxx>
#include <StdMeshers_MaxElementVolume.hxx>
#include <StdMeshers_NumberOfLayers.hxx>
#include <StdMeshers_Prism_3D.hxx>
#include <StdMeshers_Projection_1D.hxx>
#include <StdMeshers_Projection_2D.hxx>
#include <StdMeshers_Projection_3D.hxx>
#include <StdMeshers_RadialPrism_3D.hxx>
#include <StdMeshers_SegmentAroundVertex_0D.hxx>
#include <StdMeshers_ProjectionSource1D.hxx>
#include <StdMeshers_ProjectionSource2D.hxx>
#include <StdMeshers_ProjectionSource3D.hxx>
#include <StdMeshers_SegmentLengthAroundVertex.hxx>
#include <StdMeshers_CompositeHexa_3D.hxx>

#if SMESH_VERSION_MAJOR < 7
# include <StdMeshers_TrianglePreference.hxx>
#endif

// Opencascade
#include <Standard_Real.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shape.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>
#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Precision.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TopoDS_Edge.hxx>
#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <gp_Pnt.hxx>

// VTK
#include <vtkFieldData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSetReader.h>
#include <vtkGeometryFilter.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkAppendFilter.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
#include <vtkStructuredGrid.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkCellTypes.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticQuad.h>
#include <vtkTetra.h>
#include <vtkPyramid.h>
#include <vtkWedge.h>
#include <vtkHexahedron.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticWedge.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkPolyData.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkUniformGrid.h>
#include <vtkCompositeDataSet.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLImageDataReader.h>

// Netgen
#ifdef FCWithNetgen
# include <NETGENPlugin_SimpleHypothesis_3D.hxx>
# include <NETGENPlugin_Hypothesis.hxx>
# include <NETGENPlugin_Mesher.hxx>
#endif

#endif // _PreComp_
#endif

