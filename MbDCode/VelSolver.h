#pragma once

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
		void handleSingularMatrix();
		void logSingularMatrixMessage();
		std::shared_ptr<MatrixSolver> matrixSolverClassNew();
		void solveEquations();
		void setSystem(Solver* sys) override;

		SystemSolver* system;
		int n;
		FColDsptr x, errorVector;
		SpMatDsptr jacobian;
		std::shared_ptr<MatrixSolver> matrixSolver;

	};
}

