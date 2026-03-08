// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <vtkSmartPointer.h>
#include <vtkUnstructuredGridAlgorithm.h>

class vtkInformation;
class vtkInformationVector;


namespace Fem
{

// algorithm that allows multi frame handling: if data is stored in MultiBlock dataset
// this source enables the downstream filters to query the blocks as different time frames
class vtkFemFrameSourceAlgorithm: public vtkUnstructuredGridAlgorithm
{
public:
    static vtkFemFrameSourceAlgorithm* New();
    vtkTypeMacro(vtkFemFrameSourceAlgorithm, vtkUnstructuredGridAlgorithm);

    bool isValid();
    void setDataObject(vtkSmartPointer<vtkDataObject> data);
    std::vector<double> getFrameValues();

protected:
    vtkFemFrameSourceAlgorithm();
    ~vtkFemFrameSourceAlgorithm() override;

    vtkSmartPointer<vtkDataObject> m_data;

    int RequestInformation(
        vtkInformation* reqInfo,
        vtkInformationVector** inVector,
        vtkInformationVector* outVector
    ) override;
    int RequestData(
        vtkInformation* reqInfo,
        vtkInformationVector** inVector,
        vtkInformationVector* outVector
    ) override;
};

}  // namespace Fem
