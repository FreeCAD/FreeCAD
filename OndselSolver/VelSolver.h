/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>
#include <memory>

#include "Solver.h"
#include "FullColumn.h"
#include "SparseMatrix.h"

namespace MbD {
	class MatrixSolver;
	class SystemSolver;

	class VelSolver : public Solver
	{
		//system n x errorVector jacobian matrixSolver 
	public:
		void basicSolveEquations();
		void handleSingularMatrix() override;
		void logSingularMatrixMessage();
		std::shared_ptr<MatrixSolver> matrixSolverClassNew();
		void solveEquations();
		void setSystem(Solver* sys) override;

		SystemSolver* system = nullptr;
		size_t n = SIZE_MAX;
		FColDsptr x, errorVector;
		SpMatDsptr jacobian;
		std::shared_ptr<MatrixSolver> matrixSolver;

	};
}

