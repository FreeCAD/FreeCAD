// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCleanUnstructuredGrid
 * @brief   merge duplicate points
 *
 *
 * vtkCleanUnstructuredGrid is a filter that takes unstructured grid data as
 * input and generates unstructured grid data as output. vtkCleanUnstructuredGrid can
 * merge duplicate points (with coincident coordinates) using the vtkMergePoints object
 * to merge points.
 *
 * @sa
 * vtkCleanPolyData
 */


#pragma once

#include "vtkFiltersGeneralModule.h"  // For export macro
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridAlgorithm.h"

/*VTK_ABI_NAMESPACE_BEGIN*/

class vtkIncrementalPointLocator;
class vtkDataSet;

class /*VTKFILTERSGENERAL_EXPORT*/ vtkCleanUnstructuredGrid: public vtkUnstructuredGridAlgorithm
{
public:
    static vtkCleanUnstructuredGrid* New();
    vtkTypeMacro(vtkCleanUnstructuredGrid, vtkUnstructuredGridAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    ///@{
    /**
     * By default ToleranceIsAbsolute is false and Tolerance is
     * a fraction of Bounding box diagonal, if true, AbsoluteTolerance is
     * used when adding points to locator (merging)
     */
    vtkSetMacro(ToleranceIsAbsolute, bool);
    vtkBooleanMacro(ToleranceIsAbsolute, bool);
    vtkGetMacro(ToleranceIsAbsolute, bool);
    ///@}

    ///@{
    /**
     * Specify tolerance in terms of fraction of bounding box length.
     * Default is 0.0.
     */
    vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
    vtkGetMacro(Tolerance, double);
    ///@}

    ///@{
    /**
     * Specify tolerance in absolute terms. Default is 1.0.
     */
    vtkSetClampMacro(AbsoluteTolerance, double, 0.0, VTK_DOUBLE_MAX);
    vtkGetMacro(AbsoluteTolerance, double);
    ///@}

    ///@{
    /**
     * Set/Get a spatial locator for speeding the search process. By
     * default an instance of vtkMergePoints is used.
     */
    virtual void SetLocator(vtkIncrementalPointLocator* locator);
    virtual vtkIncrementalPointLocator* GetLocator();
    ///@}

    /**
     * Create default locator. Used to create one when none is specified.
     */
    void CreateDefaultLocator(vtkDataSet* input = nullptr);

    /**
     * Release locator
     */
    void ReleaseLocator()
    {
        this->SetLocator(nullptr);
    }

    ///@{
    /**
     * Set/get the desired precision for the output types. See the documentation
     * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
     * the available precision settings.
     */
    vtkSetMacro(OutputPointsPrecision, int);
    vtkGetMacro(OutputPointsPrecision, int);
    ///@}

    ///@{
    /**
     * Set/Get whether to remove points that do not
     * have any cells associated with it.
     * Default is false
     */
    vtkSetMacro(RemovePointsWithoutCells, bool);
    vtkGetMacro(RemovePointsWithoutCells, bool);
    vtkBooleanMacro(RemovePointsWithoutCells, bool);
    ///@}

    ///@{
    /**
     * Set/Get the strategy used to weigh point data on merging points
     *
     * Possibilities:
     * - FIRST_POINT (int(0), default): the point with the lowest index imposes its data on to the
     * merged point
     * - AVERAGING (int(1)): a number average is performed on all the duplicate points
     * - SPATIAL_DENSITY (int(2)): an average by attached cell volume (i.e. for every cell the point
     * is connected to sum cell_volume/number_cell_points) is performed on the point data
     */
    vtkGetMacro(PointDataWeighingStrategy, int);
    vtkSetClampMacro(PointDataWeighingStrategy, int, FIRST_POINT, NUMBER_OF_WEIGHING_TYPES - 1);
    ///@}

    enum DataWeighingType
    {
        FIRST_POINT = 0,
        AVERAGING,
        SPATIAL_DENSITY,
        NUMBER_OF_WEIGHING_TYPES
    };

protected:
    vtkCleanUnstructuredGrid();
    ~vtkCleanUnstructuredGrid() override;

    bool ToleranceIsAbsolute = false;
    double Tolerance = 0.0;
    double AbsoluteTolerance = 1.0;
    bool RemovePointsWithoutCells = false;
    vtkSmartPointer<vtkIncrementalPointLocator> Locator;
    int OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
    int PointDataWeighingStrategy = FIRST_POINT;

    int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
    int FillInputPortInformation(int port, vtkInformation* info) override;

private:
    vtkCleanUnstructuredGrid(const vtkCleanUnstructuredGrid&) = delete;
    void operator=(const vtkCleanUnstructuredGrid&) = delete;
};
/*VTK_ABI_NAMESPACE_END*/
// VTK-HeaderTest-Exclude: vtkCleanUnstructuredGrid.h
