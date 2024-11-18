/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef FEM_VTK_TOOLS_H
#define FEM_VTK_TOOLS_H

#include <vtkDataSet.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <App/DocumentObject.h>

#include "FemMeshObject.h"


namespace Fem
{
// utility class to import/export read/write vtk mesh and result
class FemExport FemVTKTools
{
public:
    // extract data from vtkUnstructuredGrid instance and fill a FreeCAD FEM mesh object with that
    // data
    static void importVTKMesh(vtkSmartPointer<vtkDataSet> grid, FemMesh* mesh, float scale = 1.0);

    // extract data from FreCAD FEM mesh and fill a vtkUnstructuredGrid instance with that data
    static void exportVTKMesh(const FemMesh* mesh,
                              vtkSmartPointer<vtkUnstructuredGrid> grid,
                              float scale = 1.0);

    // extract data from vtkUnstructuredGrid object and fill a FreeCAD FEM result object with that
    // data (needed by readResult)
    static void importFreeCADResult(vtkSmartPointer<vtkDataSet> dataset,
                                    App::DocumentObject* result);

    // extract data from a FreeCAD FEM result object and fill a vtkUnstructuredGrid object with that
    // data (needed by writeResult)
    static void exportFreeCADResult(const App::DocumentObject* result,
                                    vtkSmartPointer<vtkDataSet> grid);

    // FemMesh read from vtkUnstructuredGrid data file
    static FemMesh* readVTKMesh(const char* filename, FemMesh* mesh);

    // FemMesh write to vtkUnstructuredGrid data file
    static void writeVTKMesh(const char* Filename, const FemMesh* mesh);

    // FemResult (activeObject or created if res= NULL) read from vtkUnstructuredGrid dataset file
    static App::DocumentObject* readResult(const char* Filename,
                                           App::DocumentObject* res = nullptr);

    // write FemResult (activeObject if res= NULL) to vtkUnstructuredGrid dataset file
    static void writeResult(const char* filename, const App::DocumentObject* res = nullptr);
};
}  // namespace Fem

#endif  // FEM_VTK_TOOLS_H
