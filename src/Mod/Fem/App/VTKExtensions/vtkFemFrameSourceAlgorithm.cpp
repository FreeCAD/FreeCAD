// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *  *
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <cmath>
# include <algorithm>
# include <iterator>
# include <vector>
# include <vtkUnstructuredGrid.h>
# include <vtkMultiBlockDataSet.h>
# include <vtkFieldData.h>
# include <vtkStreamingDemandDrivenPipeline.h>
# include <vtkFloatArray.h>
# include <vtkInformation.h>
# include <vtkInformationVector.h>
#endif

#include "vtkFemFrameSourceAlgorithm.h"

using namespace Fem;


vtkStandardNewMacro(vtkFemFrameSourceAlgorithm);

vtkFemFrameSourceAlgorithm::vtkFemFrameSourceAlgorithm()
{
    // we are a source
    SetNumberOfInputPorts(0);
    SetNumberOfOutputPorts(1);
}

vtkFemFrameSourceAlgorithm::~vtkFemFrameSourceAlgorithm() = default;

void vtkFemFrameSourceAlgorithm::setDataObject(vtkSmartPointer<vtkDataObject> data)
{
    m_data = data;
    Modified();
    Update();
}

bool vtkFemFrameSourceAlgorithm::isValid()
{
    return m_data.GetPointer() ? true : false;
}

std::vector<double> vtkFemFrameSourceAlgorithm::getFrameValues()
{

    // check if we have frame data
    if (!m_data || !m_data->IsA("vtkMultiBlockDataSet")) {
        return std::vector<double>();
    }

    // we have multiple frames! let's check the amount and times
    vtkSmartPointer<vtkMultiBlockDataSet> multiblock = vtkMultiBlockDataSet::SafeDownCast(m_data);

    unsigned long nblocks = multiblock->GetNumberOfBlocks();
    std::vector<double> tFrames(nblocks);

    for (unsigned long i = 0; i < nblocks; i++) {

        vtkDataObject* block = multiblock->GetBlock(i);
        // check if the TimeValue field is available
        if (!block->GetFieldData()->HasArray("TimeValue")) {
            // a frame with no valid value is a undefined state
            return std::vector<double>();
        }

        // store the time value!
        vtkDataArray* TimeValue = block->GetFieldData()->GetArray("TimeValue");
        if (!TimeValue->IsA("vtkFloatArray") || TimeValue->GetNumberOfTuples() < 1) {
            // a frame with no valid value is a undefined state
            return std::vector<double>();
        }

        tFrames[i] = vtkFloatArray::SafeDownCast(TimeValue)->GetValue(0);
    }

    return tFrames;
}

int vtkFemFrameSourceAlgorithm::RequestInformation(
    vtkInformation* reqInfo,
    vtkInformationVector** inVector,
    vtkInformationVector* outVector
)
{

    // setup default information
    if (!this->Superclass::RequestInformation(reqInfo, inVector, outVector)) {
        return 0;
    }

    if (!m_data) {
        // for the no data case we would return a empty data set in RequestData.
        return 1;
    }

    std::vector<double> frames = getFrameValues();

    if (frames.empty()) {
        // no frames, default info is sufficient
        return 1;
    }

    double tRange[2] = {frames.front(), frames.back()};

    // finally set the time info!
    vtkInformation* info = outVector->GetInformationObject(0);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &frames[0], frames.size());
    info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

    return 1;
}

int vtkFemFrameSourceAlgorithm::RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector* outVector
)
{
    vtkInformation* outInfo = outVector->GetInformationObject(0);
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT())
    );

    if (!output) {
        return 0;
    }

    if (!m_data) {
        outInfo->Set(vtkDataObject::DATA_OBJECT(), vtkUnstructuredGrid::New());
        return 1;
    }

    if (!m_data->IsA("vtkMultiBlockDataSet")) {
        // no multi frame data, return directly
        outInfo->Set(vtkDataObject::DATA_OBJECT(), m_data);
        return 1;
    }

    vtkSmartPointer<vtkMultiBlockDataSet> multiblock = vtkMultiBlockDataSet::SafeDownCast(m_data);
    // find the block asked for (lazy implementation)
    unsigned long idx = 0;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())) {
        auto time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
        auto frames = getFrameValues();

        // we have float values, so be aware of rounding errors. lets subtract the searched time and
        // then use the smallest value
        for (auto& frame : frames) {
            frame = std::abs(frame - time);
        }

        auto it = std::ranges::min_element(frames);
        idx = std::distance(frames.begin(), it);
    }

    auto block = multiblock->GetBlock(idx);
    output->ShallowCopy(block);
    return 1;
}
