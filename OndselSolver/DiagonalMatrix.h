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

namespace MbD {
	class DiagonalMatrix : public Array<double>
	{
		//
	public:
		DiagonalMatrix() : Array<double>() {}
		explicit DiagonalMatrix(int count) : Array<double>(count) {}
		DiagonalMatrix(int count, const double& value) : Array<double>(count, value) {}
		DiagonalMatrix(std::initializer_list<double> list) : Array<double>{ list } {}
		void atiputDiagonalMatrix(int i, std::shared_ptr<DiagonalMatrix> diagMat);
        DiagMatDsptr times(double factor);
		FColsptr<double> timesFullColumn(FColsptr<double> fullCol);
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
}

