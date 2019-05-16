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


#ifndef FEMGUI_PRECOMPILED_H
#define FEMGUI_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define AppFemExport   __declspec(dllimport)
# define PartExport  __declspec(dllimport)
# define FemGuiExport   __declspec(dllexport)
#else // for Linux
# define PartExport
# define AppFemExport
# define FemGuiExport
#endif

#ifdef _MSC_VER
# pragma warning(disable : 4005)
# pragma warning(disable : 4290)
#endif

#ifdef _PreComp_

// Python
#include <Python.h>

// standard
#include <iostream>
#include <assert.h>
#include <cmath>

#include <math.h>

// STL
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <bitset>
#include <sstream>

// boost
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// OCC
#include <Standard_math.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax1.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Geom_Line.hxx>
#include <gp_Lin.hxx>
#include <Standard_PrimitiveTypes.hxx>
#include <TopoDS_Shape.hxx>

// Qt Toolkit
#ifndef __Qt4All__
# include <Gui/Qt4All.h>
#endif

#include <qobject.h>
#include <QRegExp>
#include <QTextCharFormat>
#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QAction>
#include <QString>
#include <QSlider>
#include <QKeyEvent>
#include <QTextStream>
#include <QString>
#include <QSlider>
#include <QPushButton>
#include <QMenu>
#include <QDockWidget>
#include <QStackedWidget>
#include <QFile>

// inventor
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoFontStyle.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoMultipleCopy.h>
#include <Inventor/nodes/SoText3.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/manips/SoCenterballManip.h>
#include <Inventor/manips/SoTransformerManip.h>
#include <Inventor/manips/SoTransformBoxManip.h>
#include <Inventor/manips/SoHandleBoxManip.h>
#include <Inventor/manips/SoTabBoxManip.h>
#include <Inventor/engines/SoDecomposeMatrix.h>
#include <Inventor/draggers/SoCenterballDragger.h>
#include <Inventor/draggers/SoTransformerDragger.h>
#include <Inventor/draggers/SoTransformBoxDragger.h>
#include <Inventor/draggers/SoHandleBoxDragger.h>

#include <Inventor/nodes/SoIndexedPointSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormal.h>


// Salomesh
#include <SMESH_Mesh.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMDSAbs_ElementType.hxx>

// VTK
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLookupTable.h>

#endif //_PreComp_

#endif // FEMGUI_PRECOMPILED_H
