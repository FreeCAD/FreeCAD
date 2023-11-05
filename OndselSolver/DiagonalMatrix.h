/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "FullColumn.ref.h"
#include "FullRow.ref.h"
#include "FullMatrix.ref.h"
#include "DiagonalMatrix.ref.h"
#include "Array.h"
#include "FullColumn.h"
//#include "FullRow.h"
// #include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class DiagonalMatrix : public Array<T>
	{
		//
	public:
		DiagonalMatrix() : Array<T>() {}
		DiagonalMatrix(int count) : Array<T>(count) {}
		DiagonalMatrix(int count, const T& value) : Array<T>(count, value) {}
		DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void atiputDiagonalMatrix(int i, std::shared_ptr<DiagonalMatrix<T>> diagMat);
		DiagMatsptr<T> times(T factor);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FMatDsptr timesFullMatrix(FMatDsptr fullMat);
		int nrow() {
			return (int)this->size();
		}
		int ncol() {
			return (int)this->size();
		}
		double sumOfSquares() override;
		int numberOfElements() override;
		void zeroSelf() override;
		double maxMagnitude() override;

		std::ostream& printOn(std::ostream& s) const override;
	};

    template class DiagonalMatrix<double>;
}

