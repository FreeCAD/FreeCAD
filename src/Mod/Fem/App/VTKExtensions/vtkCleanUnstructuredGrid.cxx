// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileNotice: Part of the FreeCAD project.


#include "PreCompiled.h"

#ifndef _PreComp_
# include <vtkArrayDispatch.h>
# include <vtkArrayDispatchArrayList.h>
# include <vtkBitArray.h>
# include <vtkCell.h>
# include <vtkCellData.h>
# include <vtkCellSizeFilter.h>
# include <vtkCellTypes.h>
# include <vtkCollection.h>
# include <vtkDataArrayRange.h>
# include <vtkDataSet.h>
# include <vtkDoubleArray.h>
# include <vtkIncrementalPointLocator.h>
# include <vtkInformation.h>
# include <vtkInformationVector.h>
# include <vtkIntArray.h>
# include <vtkMergePoints.h>
# include <vtkObjectFactory.h>
# include <vtkPointData.h>
# include <vtkPointSet.h>
# include <vtkPoints.h>
# include <vtkRectilinearGrid.h>
# include <vtkSMPThreadLocalObject.h>
# include <vtkStringArray.h>
# include <vtkUnstructuredGrid.h>

# include <unordered_set>
#endif

#include "vtkCleanUnstructuredGrid.h"
#include "vtkSMPTools.h"

namespace
{

int GetDimension(unsigned char type)
{
    // For the most common cell types, this is a fast call. If the cell type is
    // more exotic, then the cell must be grabbed and queried directly, which is
    // slow.
    switch (type) {
        case VTK_EMPTY_CELL:
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
            return 0;
        case VTK_LINE:
        case VTK_POLY_LINE:
        case VTK_QUADRATIC_EDGE:
        case VTK_CUBIC_LINE:
        case VTK_LAGRANGE_CURVE:
        case VTK_BEZIER_CURVE:
            return 1;
        case VTK_TRIANGLE:
        case VTK_QUAD:
        case VTK_PIXEL:
        case VTK_POLYGON:
        case VTK_TRIANGLE_STRIP:
        case VTK_QUADRATIC_TRIANGLE:
        case VTK_QUADRATIC_QUAD:
        case VTK_QUADRATIC_POLYGON:
        case VTK_BIQUADRATIC_QUAD:
        case VTK_BIQUADRATIC_TRIANGLE:
        case VTK_LAGRANGE_TRIANGLE:
        case VTK_LAGRANGE_QUADRILATERAL:
        case VTK_BEZIER_TRIANGLE:
        case VTK_BEZIER_QUADRILATERAL:
            return 2;
        case VTK_TETRA:
        case VTK_VOXEL:
        case VTK_HEXAHEDRON:
        case VTK_WEDGE:
        case VTK_PYRAMID:
        case VTK_PENTAGONAL_PRISM:
        case VTK_HEXAGONAL_PRISM:
        case VTK_QUADRATIC_TETRA:
        case VTK_QUADRATIC_HEXAHEDRON:
        case VTK_QUADRATIC_WEDGE:
        case VTK_QUADRATIC_PYRAMID:
        case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
        case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
        case VTK_TRIQUADRATIC_HEXAHEDRON:
        case VTK_TRIQUADRATIC_PYRAMID:
        case VTK_LAGRANGE_TETRAHEDRON:
        case VTK_LAGRANGE_HEXAHEDRON:
        case VTK_LAGRANGE_WEDGE:
        case VTK_BEZIER_TETRAHEDRON:
        case VTK_BEZIER_HEXAHEDRON:
        case VTK_BEZIER_WEDGE:
            return 3;
        default:
            vtkNew<vtkGenericCell> cell;
            cell->SetCellType(type);
            return cell->GetCellDimension();
    }
}

constexpr unsigned char MAX_CELL_DIM = 3;

void AllocatePointAttributes(vtkPointData* inPD, vtkPointData* outPD, vtkIdType nPoints)
{
    for (vtkIdType iArr = 0; iArr < inPD->GetNumberOfArrays(); ++iArr) {
        auto inArr = inPD->GetAbstractArray(iArr);
        auto outArr = outPD->GetAbstractArray(inArr->GetName());
        if (!outArr) {
            vtkGenericWarningMacro(<< inArr->GetName() << " output array is nullptr");
            continue;
        }
        outArr->SetNumberOfComponents(inArr->GetNumberOfComponents());
        outArr->SetNumberOfTuples(nPoints);
    }
}

unsigned char GetTopologicalDimension(vtkDataSet* ds)
{
    vtkNew<vtkCellTypes> cTypes;
    ds->GetCellTypes(cTypes);
    unsigned char topoDim = 0;
    for (vtkIdType iC = 0; iC < cTypes->GetNumberOfTypes(); ++iC) {
        unsigned char dimC = static_cast<unsigned char>(GetDimension(cTypes->GetCellType(iC)));
        topoDim = std::max(topoDim, dimC);
        if (topoDim >= MAX_CELL_DIM) {
            break;
        }
    }
    if (topoDim > MAX_CELL_DIM) {
        vtkErrorWithObjectMacro(
            nullptr,
            "Topological dimension of data set is larger than the maximal cell dimension"
        );
        return MAX_CELL_DIM;
    }
    return topoDim;
}

struct WeighingStrategy
{
    virtual ~WeighingStrategy() = default;
    virtual vtkSmartPointer<vtkDoubleArray> ComputeWeights(
        vtkDataSet* ds,
        const std::vector<vtkIdType>& ptMap
    ) = 0;
};

struct FirstPointStrategy: public WeighingStrategy
{
    vtkSmartPointer<vtkDoubleArray> ComputeWeights(
        vtkDataSet* ds,
        const std::vector<vtkIdType>& ptMap
    ) override
    {
        if (ds->GetNumberOfPoints() != static_cast<vtkIdType>(ptMap.size())) {
            vtkGenericWarningMacro(
                "Number of points in dataset and number of entries in point map don't line up."
            );
            return nullptr;
        }
        vtkNew<vtkDoubleArray> weights;
        weights->SetNumberOfComponents(1);
        weights->SetNumberOfTuples(ds->GetNumberOfPoints());
        weights->Fill(0.0);

        std::unordered_set<vtkIdType> firstPoints;
        for (vtkIdType iP = 0; iP < ds->GetNumberOfPoints(); ++iP) {
            if (firstPoints.find(ptMap[iP]) != firstPoints.end()) {
                continue;
            }
            firstPoints.insert(ptMap[iP]);
            weights->SetValue(iP, 1.0);
        }
        return weights;
    }
};

struct AveragingStrategy: public WeighingStrategy
{
    vtkSmartPointer<vtkDoubleArray> ComputeWeights(
        vtkDataSet* ds,
        const std::vector<vtkIdType>& ptMap
    ) override
    {
        if (ds->GetNumberOfPoints() != static_cast<vtkIdType>(ptMap.size())) {
            vtkGenericWarningMacro(
                "Number of points in dataset and number of entries in point map don't line up."
            );
            return nullptr;
        }
        std::vector<double> counts(ds->GetNumberOfPoints(), 0.0);
        for (vtkIdType iP = 0; iP < ds->GetNumberOfPoints(); ++iP) {
            if (ptMap[iP] < 0) {
                continue;
            }
            counts[ptMap[iP]] += 1.0;
        }

        vtkNew<vtkDoubleArray> weights;
        weights->SetNumberOfComponents(1);
        weights->SetNumberOfTuples(ds->GetNumberOfPoints());
        weights->Fill(0.0);

        auto wRange = vtk::DataArrayValueRange<1>(weights);
        for (vtkIdType iP = 0; iP < ds->GetNumberOfPoints(); ++iP) {
            if (ptMap[iP] < 0) {
                continue;
            }
            wRange[iP] = (counts[ptMap[iP]] ? 1.0 / counts[ptMap[iP]] : 0.0);
        }

        return weights;
    }
};

struct SpatialDensityStrategy: public WeighingStrategy
{
    vtkSmartPointer<vtkDoubleArray> ComputeWeights(
        vtkDataSet* ds,
        const std::vector<vtkIdType>& ptMap
    ) override
    {
        if (ds->GetNumberOfPoints() != static_cast<vtkIdType>(ptMap.size())) {
            vtkGenericWarningMacro(
                "Number of points in dataset and number of entries in point map don't line up."
            );
            return nullptr;
        }
        // Get topological dimension of data set
        auto topoDim = GetTopologicalDimension(ds);
        // Calculate cell measures
        vtkSmartPointer<vtkDataArray> measures;
        {
            // this scope is so that any extra memory related to the cell size filter gets released
            // when no longer needed
            vtkNew<vtkCellSizeFilter> cellSizeFilter;
            cellSizeFilter->SetInputData(ds);
            cellSizeFilter->Update();
            auto output = vtkDataSet::SafeDownCast(cellSizeFilter->GetOutputDataObject(0));
            auto cData = output->GetCellData();
            if (!cData || !cData->HasArray("VertexCount") || !cData->HasArray("Length")
                || !cData->HasArray("Area") || !cData->HasArray("Volume")) {
                vtkErrorWithObjectMacro(
                    nullptr,
                    "Could not find correct cell data in output of cell size filter"
                );
                return nullptr;
            }
            switch (topoDim) {
                case 0:
                    measures = cData->GetArray("VertexCount");
                    break;
                case 1:
                    measures = cData->GetArray("Length");
                    break;
                case 2:
                    measures = cData->GetArray("Area");
                    break;
                case 3:
                    measures = cData->GetArray("Volume");
                    break;
                default:
                    vtkErrorWithObjectMacro(
                        nullptr,
                        "Topological dimension of data set is higher than 3. "
                        "Cannot deal with that."
                    );
                    return nullptr;
            }
        }
        // Distribute spatial density to points
        vtkNew<vtkDoubleArray> density;
        density->SetNumberOfComponents(1);
        density->SetNumberOfTuples(ds->GetNumberOfPoints());
        density->Fill(0.0);
        auto dRange = vtk::DataArrayValueRange<1>(density);
        auto mRange = vtk::DataArrayValueRange<1>(measures);
        if (ds->GetNumberOfCells() > 0) {
            vtkNew<vtkIdList> pointIdList;
            ds->GetCellPoints(0, pointIdList);
        }
        vtkSMPThreadLocalObject<vtkIdList> localPointIds;
        auto distribute = [&](vtkIdType begin, vtkIdType end) {
            for (vtkIdType iC = begin; iC < end; ++iC) {
                ds->GetCellPoints(iC, localPointIds.Local());
                double participation = mRange[iC] / localPointIds.Local()->GetNumberOfIds();
                for (vtkIdType iP = 0; iP < localPointIds.Local()->GetNumberOfIds(); ++iP) {
                    dRange[localPointIds.Local()->GetId(iP)] += participation;
                }
            }
        };
        // For thread safety
        if (ds->GetNumberOfCells() > 0) {
            vtkNew<vtkIdList> buffer;
            ds->GetCellPoints(0, buffer);
        }
        distribute(0, ds->GetNumberOfCells());
        // Merits a dedicated struct with a reduce operation
        // collisions occurring in the += operation
        // vtkSMPTools::For(0, ds->GetNumberOfCells(), distribute);
        // Normalize spatial densities with respect to point map
        {
            std::vector<double> masses(*std::max_element(ptMap.begin(), ptMap.end()) + 1, 0);
            auto computeMasses = [&dRange, &masses, &ptMap](vtkIdType begin, vtkIdType end) {
                for (vtkIdType iP = begin; iP < end; ++iP) {
                    if (ptMap[iP] < 0) {
                        dRange[iP] = 0.0;
                        continue;
                    }
                    masses[ptMap[iP]] += dRange[iP];
                }
            };
            computeMasses(0, ds->GetNumberOfPoints());
            // Merits a dedicated struct with a reduce operation
            // collisions occurring in the += operation
            // vtkSMPTools::For(0, ds->GetNumberOfPoints(), computeMasses);
            vtkSMPTools::For(
                0,
                ds->GetNumberOfPoints(),
                [&dRange, &masses, &ptMap](vtkIdType begin, vtkIdType end) {
                    for (vtkIdType iP = begin; iP < end; ++iP) {
                        if (ptMap[iP] < 0) {
                            continue;
                        }
                        dRange[iP] = (masses[ptMap[iP]] != 0 ? dRange[iP] / masses[ptMap[iP]] : 0.0);
                    }
                }
            );
        }
        return density;
    }
};

struct WeighingStrategyFactory
{
    std::shared_ptr<WeighingStrategy> operator()(vtkCleanUnstructuredGrid::DataWeighingType dwt)
    {
        switch (dwt) {
            case vtkCleanUnstructuredGrid::FIRST_POINT:
                return std::make_shared<FirstPointStrategy>();
            case vtkCleanUnstructuredGrid::AVERAGING:
                return std::make_shared<AveragingStrategy>();
            case vtkCleanUnstructuredGrid::SPATIAL_DENSITY:
                return std::make_shared<SpatialDensityStrategy>();
            default:
                vtkGenericWarningMacro(
                    "Incorrect weighing strategy type passed to factory. "
                    "defaulting to FIRST_POINT."
                );
                return std::make_shared<FirstPointStrategy>();
        }
    }
};

struct WeighingWorklet
{
    template<typename ArrayTypeIn, typename ArrayTypeOut>
    void operator()(
        ArrayTypeIn* inArray,
        ArrayTypeOut* outArray,
        vtkDoubleArray* weights,
        const std::vector<vtkIdType>& ptMap
    )
    {
        outArray->Fill(0);
        auto inRange = vtk::DataArrayTupleRange(inArray);
        auto outRange = vtk::DataArrayTupleRange(outArray);
        auto wRange = vtk::DataArrayValueRange<1>(weights);
        auto weighing = [&](vtkIdType begin, vtkIdType end) {
            for (vtkIdType iP = begin; iP < end; ++iP) {
                if (ptMap[iP] < 0) {
                    continue;
                }
                auto inT = inRange[iP];
                auto outT = outRange[ptMap[iP]];
                for (vtkIdType iT = 0; iT < inArray->GetNumberOfComponents(); ++iT) {
                    outT[iT] += wRange[iP] * inT[iT];
                }
            }
        };
        weighing(0, inArray->GetNumberOfTuples());
        // Merits a dedicated struct with a reduce operation
        // collisions occurring in the += operation
        // vtkSMPTools::For(0, inArray->GetNumberOfTuples(), weighing);
    }
};

template<>
void WeighingWorklet::operator()(
    vtkBitArray* inArray,
    vtkBitArray* outArray,
    vtkDoubleArray* vtkNotUsed(weights),
    const std::vector<vtkIdType>& ptMap
)
{
    outArray->Fill(0);
    for (vtkIdType iP = 0; iP < inArray->GetNumberOfValues(); ++iP) {
        if (ptMap[iP] < 0) {
            continue;
        }
        outArray->SetValue(ptMap[iP], inArray->GetValue(iP));
    }
}

template<>
void WeighingWorklet::operator()(
    vtkStringArray* inArray,
    vtkStringArray* outArray,
    vtkDoubleArray* vtkNotUsed(weights),
    const std::vector<vtkIdType>& ptMap
)
{
    for (vtkIdType iP = 0; iP < inArray->GetNumberOfValues(); ++iP) {
        if (ptMap[iP] < 0) {
            continue;
        }
        outArray->SetValue(ptMap[iP], inArray->GetValue(iP));
    }
}

template<>
void WeighingWorklet::operator()(
    vtkAbstractArray* inArray,
    vtkAbstractArray* outArray,
    vtkDoubleArray* vtkNotUsed(weights),
    const std::vector<vtkIdType>& ptMap
)
{
    for (vtkIdType iP = 0; iP < inArray->GetNumberOfTuples(); ++iP) {
        if (ptMap[iP] < 0) {
            continue;
        }
        outArray->InsertTuple(ptMap[iP], iP, inArray);
    }
}

void WeightAttributes(
    vtkPointData* inPD,
    vtkPointData* outPD,
    vtkDoubleArray* weights,
    const std::vector<vtkIdType>& ptMap
)
{
    // better here to use a Dispatch2BySameArrayType, but that doesn't exist
    using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::AllTypes>;
    WeighingWorklet worker;
    for (vtkIdType iArr = 0; iArr < inPD->GetNumberOfArrays(); ++iArr) {
        auto inArr = inPD->GetArray(iArr);
        // if not data array check for abstract
        if (!inArr) {
            auto inAbsArr = inPD->GetAbstractArray(iArr);
            if (!inAbsArr) {
                vtkGenericWarningMacro("One of the arrays in the point data is nullptr.");
                continue;
            }
            auto outAbsArr = outPD->GetAbstractArray(inAbsArr->GetName());
            if (!outAbsArr) {
                vtkGenericWarningMacro("Output array " << inAbsArr->GetName() << " is nullptr.");
                continue;
            }
            // if string array go to dedicated path
            auto inStrArr = vtkStringArray::SafeDownCast(inAbsArr);
            if (inStrArr) {
                auto outStrArr = vtkStringArray::SafeDownCast(outAbsArr);
                if (!outStrArr) {
                    vtkGenericWarningMacro(
                        "Output array " << inStrArr->GetName()
                                        << " is not the same type as input string array."
                    );
                    continue;
                }
                worker(inStrArr, outStrArr, weights, ptMap);
                continue;
            }
            worker(inAbsArr, outAbsArr, weights, ptMap);
            continue;
        }
        auto outArr = outPD->GetArray(inArr->GetName());
        if (!outArr) {
            vtkGenericWarningMacro(
                "Output array " << inArr->GetName() << " is nullptr or not a vtkDataArray."
            );
            continue;
        }
        if (!Dispatcher::Execute(inArr, outArr, worker, weights, ptMap)) {
            auto inBitArr = vtkBitArray::SafeDownCast(inArr);
            auto outBitArr = vtkBitArray::SafeDownCast(outArr);
            if (inBitArr && outBitArr) {
                worker(inBitArr, outBitArr, weights, ptMap);
            }
            worker(inArr, outArr, weights, ptMap);
        }
    }
}

}  // namespace

vtkStandardNewMacro(vtkCleanUnstructuredGrid);
vtkCxxSetSmartPointerMacro(vtkCleanUnstructuredGrid, Locator, vtkIncrementalPointLocator);

//----------------------------------------------------------------------------
vtkCleanUnstructuredGrid::vtkCleanUnstructuredGrid() = default;

//----------------------------------------------------------------------------
vtkCleanUnstructuredGrid::~vtkCleanUnstructuredGrid() = default;

//----------------------------------------------------------------------------
void vtkCleanUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    if (this->Locator) {
        os << indent << "Locator: ";
        this->Locator->PrintSelf(os, indent.GetNextIndent());
    }
    else {
        os << indent << "Locator: none\n";
    }
    os << indent << "ToleranceIsAbsolute: " << (this->ToleranceIsAbsolute ? "On\n" : "Off\n");
    os << indent << "Tolerance: " << this->Tolerance << std::endl;
    os << indent << "AbsoluteTolerance: " << this->AbsoluteTolerance << std::endl;
    os << indent
       << "RemovePointsWithoutCells: " << (this->RemovePointsWithoutCells ? "On\n" : "Off\n");
    os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << std::endl;
}

//----------------------------------------------------------------------------
int vtkCleanUnstructuredGrid::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector
)
{
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT())
    );

    if (input->GetNumberOfCells() == 0) {
        // set up a ugrid with same data arrays as input, but
        // no points, cells or data.
        output->Allocate(1);
        output->GetPointData()->CopyAllocate(input->GetPointData(), VTK_CELL_SIZE);
        output->GetCellData()->CopyAllocate(input->GetCellData(), 1);
        vtkNew<vtkPoints> pts;
        output->SetPoints(pts);
        return 1;
    }

    output->GetPointData()->CopyAllocate(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    // First, create a new points array that eliminate duplicate points.
    // Also create a mapping from the old point id to the new.
    vtkNew<vtkPoints> newPts;

    // Set the desired precision for the points in the output.
    if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION) {
        // The logical behaviour would be to use the data type from the input.
        // However, input is a vtkDataSet, which has no point data type; only the
        // derived class vtkPointSet has a vtkPoints attribute, so only for that
        // the logical practice can be applied, while for others (currently
        // vtkImageData and vtkRectilinearGrid) the data type is the default
        // for vtkPoints - which is VTK_FLOAT.
        vtkPointSet* ps = vtkPointSet::SafeDownCast(input);
        if (ps) {
            newPts->SetDataType(ps->GetPoints()->GetDataType());
        }
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION) {
        newPts->SetDataType(VTK_FLOAT);
    }
    else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION) {
        newPts->SetDataType(VTK_DOUBLE);
    }

    vtkIdType num = input->GetNumberOfPoints();
    vtkIdType id;
    vtkIdType newId;
    std::vector<vtkIdType> ptMap(num);
    double pt[3];

    this->CreateDefaultLocator(input);
    if (this->ToleranceIsAbsolute) {
        this->Locator->SetTolerance(this->AbsoluteTolerance);
    }
    else {
        this->Locator->SetTolerance(this->Tolerance * input->GetLength());
    }
    double bounds[6];
    input->GetBounds(bounds);
    this->Locator->InitPointInsertion(newPts, bounds);

    vtkIdType progressStep = num / 100;
    if (progressStep == 0) {
        progressStep = 1;
    }

    vtkNew<vtkIdList> pointCells;
    for (id = 0; id < num; ++id) {
        if (id % progressStep == 0) {
            this->UpdateProgress(0.8 * ((float)id / num));
        }

        bool insert = true;
        if (this->RemovePointsWithoutCells) {
            input->GetPointCells(id, pointCells);
            if (pointCells->GetNumberOfIds() == 0) {
                insert = false;
            }
        }

        if (insert) {
            input->GetPoint(id, pt);
            this->Locator->InsertUniquePoint(pt, newId);
            ptMap[id] = newId;
        }
        else {
            // Strictly speaking, this is not needed
            // as this is never accessed, but better not let
            // an id undefined.
            ptMap[id] = -1;
        }
    }
    output->SetPoints(newPts);

    ::WeighingStrategyFactory factory;
    auto strategy = factory(static_cast<DataWeighingType>(this->PointDataWeighingStrategy));
    auto weights = strategy->ComputeWeights(input, ptMap);
    auto inPD = input->GetPointData();
    auto outPD = output->GetPointData();
    ::AllocatePointAttributes(inPD, outPD, output->GetNumberOfPoints());
    ::WeightAttributes(inPD, outPD, weights, ptMap);

    // Now copy the cells.
    vtkNew<vtkIdList> cellPoints;
    num = input->GetNumberOfCells();
    output->Allocate(num);
    for (id = 0; id < num; ++id) {
        if (id % progressStep == 0) {
            this->UpdateProgress(0.8 + 0.2 * (static_cast<float>(id) / num));
        }
        // special handling for polyhedron cells
        if (vtkUnstructuredGrid::SafeDownCast(input) && input->GetCellType(id) == VTK_POLYHEDRON) {
            vtkUnstructuredGrid::SafeDownCast(input)->GetFaceStream(id, cellPoints);
            vtkUnstructuredGrid::ConvertFaceStreamPointIds(cellPoints, ptMap.data());
        }
        else {
            input->GetCellPoints(id, cellPoints);
            for (int i = 0; i < cellPoints->GetNumberOfIds(); i++) {
                int cellPtId = cellPoints->GetId(i);
                newId = ptMap[cellPtId];
                cellPoints->SetId(i, newId);
            }
        }
        output->InsertNextCell(input->GetCellType(id), cellPoints);
    }

    output->Squeeze();
    return 1;
}

//----------------------------------------------------------------------------
int vtkCleanUnstructuredGrid::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
}

//------------------------------------------------------------------------------
vtkIncrementalPointLocator* vtkCleanUnstructuredGrid::GetLocator()
{
    return this->Locator;
}

//----------------------------------------------------------------------------
void vtkCleanUnstructuredGrid::CreateDefaultLocator(vtkDataSet* input)
{
    double tol;
    if (this->ToleranceIsAbsolute) {
        tol = this->AbsoluteTolerance;
    }
    else {
        if (input) {
            tol = this->Tolerance * input->GetLength();
        }
        else {
            tol = this->Tolerance;
        }
    }

    if (this->Locator.Get() == nullptr) {
        if (tol == 0.0) {
            this->Locator = vtkSmartPointer<vtkMergePoints>::New();
        }
        else {
            this->Locator = vtkSmartPointer<vtkPointLocator>::New();
        }
    }
    else {
        // check that the tolerance wasn't changed from zero to non-zero
        if ((tol > 0.0) && (this->GetLocator()->GetTolerance() == 0.0)) {
            this->Locator = vtkSmartPointer<vtkPointLocator>::New();
        }
    }
}
