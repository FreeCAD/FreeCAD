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

#include "FemMesh.h"
#include "FemMeshObject.h"
#include <App/DocumentObject.h>
#include "FemResultObject.h"

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>

#include <cstring>

namespace Fem
{
    /*!
     utitly class to import/export read/write vtk mesh and result
     */
    class AppFemExport FemVTKTools
    {
    public:
        /*!
         FemMesh import from vtkUnstructuredGrid instance
         */
        static void importVTKMesh(vtkSmartPointer<vtkDataSet> grid, FemMesh* mesh, float scale = 1.0);
        /*!
         FemMesh read from vtkUnstructuredGrid data file
         */
        static FemMesh* readVTKMesh(const char* filename, FemMesh* mesh);
        /*!
         FemMesh export to vtkUnstructuredGrid instance
         */
        static void exportVTKMesh(const FemMesh* mesh, vtkSmartPointer<vtkUnstructuredGrid> grid, float scale = 1.0);
        /*!
         FemMesh write to vtkUnstructuredGrid data file
         */
        static void writeVTKMesh(const char* Filename, const FemMesh* mesh);

        /*!
         * FemResult import from vtkUnstructuredGrid object
         */
        static void importFluidicResult(vtkSmartPointer<vtkDataSet> dataset, App::DocumentObject* res);
        static void importMechanicalResult(vtkSmartPointer<vtkDataSet> dataset, App::DocumentObject* res);

        /*!
         * FemResult export to vtkUnstructuredGrid object
         */
        static void exportFluidicResult(const App::DocumentObject* res, vtkSmartPointer<vtkDataSet> grid);
        static void exportMechanicalResult(const App::DocumentObject* res, vtkSmartPointer<vtkDataSet> grid);

        /*!
         * FemResult (activeObject or created if res= NULL) read from vtkUnstructuredGrid dataset file
         */
        static App::DocumentObject* readResult(const char* Filename, App::DocumentObject* res = NULL);

        /*!
         * write FemResult (activeObject if res= NULL) to vtkUnstructuredGrid dataset file
         */
        static void writeResult(const char* filename, const App::DocumentObject* res = NULL);

    };
}

#endif //FEM_VTK_TOOLS_H

