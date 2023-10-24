/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "corecrt_math_defines.h"
#include <memory>

#include "FullRow.h"
#include "RowTypeMatrix.h"
#include "FullColumn.h"

namespace MbD {
    template<typename T>
    class FullMatrix;

	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;

	template<typename T>
	using FMatsptr = std::shared_ptr<FullMatrix<T>>;

	template<typename T>
	class FullColumn;
	using FColDsptr = std::shared_ptr<FullColumn<double>>;

	template<typename T>
	class FullRow;

    template<typename T>
    class RowTypeMatrix;

	template<typename T>
	class EulerParameters;

	template<typename T>
	class DiagonalMatrix;

	using FMatFColDsptr = std::shared_ptr<FullMatrix<FColDsptr>>;
	using FMatFMatDsptr = std::shared_ptr<FullMatrix<FMatDsptr>>;
	using FColFMatDsptr = std::shared_ptr<FullColumn<FMatDsptr>>;

	template<typename T>
	class FullMatrix : public RowTypeMatrix<FRowsptr<T>>
	{
	public:
		FullMatrix() {}
		FullMatrix(int m) : RowTypeMatrix<FRowsptr<T>>(m)
		{
		}
		FullMatrix(int m, int n) {
			for (int i = 0; i < m; i++) {
				auto row = std::make_shared<FullRow<T>>(n);
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<FRowsptr<T>> listOfRows) {
			for (auto& row : listOfRows)
			{
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::initializer_list<T>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<FullRow<T>>(rowList);
				this->push_back(row);
			}
		}
		static FMatsptr<T> rotatex(T angle);
		static FMatsptr<T> rotatey(T angle);
		static FMatsptr<T> rotatez(T angle);
		static FMatsptr<T> rotatexrotDot(T angle, T angledot);
		static FMatsptr<T> rotateyrotDot(T angle, T angledot);
		static FMatsptr<T> rotatezrotDot(T angle, T angledot);
		static FMatsptr<T> rotatexrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> rotateyrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> rotatezrotDotrotDDot(T angle, T angleDot, T angleDDot);
		static FMatsptr<T> identitysptr(int n);
		static FMatsptr<T> tildeMatrix(FColDsptr col);
		void identity();
		FColsptr<T> column(int j);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FColsptr<T> timesFullColumn(FullColumn<T>* fullCol);
		FMatsptr<T> timesFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> timesTransposeFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> times(T a);
		FMatsptr<T> transposeTimesFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> plusFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> minusFullMatrix(FMatsptr<T> fullMat);
		FMatsptr<T> transpose();
		FMatsptr<T> negated();
		void symLowerWithUpper();
		void atiput(int i, FRowsptr<T> fullRow) override;
		void atijput(int i, int j, T value);
		void atijputFullColumn(int i, int j, FColsptr<T> fullCol);
		void atijplusFullRow(int i, int j, FRowsptr<T> fullRow);
		void atijplusNumber(int i, int j, T value);
		void atijminusNumber(int i, int j, T value);
		double sumOfSquares() override;
		void zeroSelf() override;
		FMatsptr<T> copy();
		FullMatrix<T> operator+(const FullMatrix<T> fullMat);
		FColsptr<T> transposeTimesFullColumn(const FColsptr<T> fullCol);
		void magnifySelf(T factor);
		std::shared_ptr<EulerParameters<T>> asEulerParameters();
		T trace();
		double maxMagnitude() override;
		FColsptr<T> bryantAngles();
		bool isDiagonal();
		bool isDiagonalToWithin(double ratio);
		std::shared_ptr<DiagonalMatrix<T>> asDiagonalMatrix();
		void conditionSelfWithTol(double tol);

		std::ostream& printOn(std::ostream& s) const override;
	};
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatex(T the)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, 1.0);
		row0->atiput(1, 0.0);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, cthe);
		row1->atiput(2, -sthe);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, sthe);
		row2->atiput(2, cthe);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatey(T the)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, cthe);
		row0->atiput(1, 0.0);
		row0->atiput(2, sthe);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, 1.0);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, -sthe);
		row2->atiput(1, 0.0);
		row2->atiput(2, cthe);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatez(T the)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, cthe);
		row0->atiput(1, -sthe);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, sthe);
		row1->atiput(1, cthe);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, 0.0);
		row2->atiput(2, 1.0);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatexrotDot(T the, T thedot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, 0.0);
		row0->atiput(1, 0.0);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, cthedot);
		row1->atiput(2, -sthedot);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, sthedot);
		row2->atiput(2, cthedot);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotateyrotDot(T the, T thedot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, cthedot);
		row0->atiput(1, 0.0);
		row0->atiput(2, sthedot);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, 0.0);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, -sthedot);
		row2->atiput(1, 0.0);
		row2->atiput(2, cthedot);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatezrotDot(T the, T thedot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, cthedot);
		row0->atiput(1, -sthedot);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, sthedot);
		row1->atiput(1, cthedot);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, 0.0);
		row2->atiput(2, 0.0);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatexrotDotrotDDot(T the, T thedot, T theddot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto stheddot = cthedot * thedot + (cthe * theddot);
		auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, 0.0);
		row0->atiput(1, 0.0);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, ctheddot);
		row1->atiput(2, -stheddot);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, stheddot);
		row2->atiput(2, ctheddot);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotateyrotDotrotDDot(T the, T thedot, T theddot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto stheddot = cthedot * thedot + (cthe * theddot);
		auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, ctheddot);
		row0->atiput(1, 0.0);
		row0->atiput(2, stheddot);
		auto row1 = rotMat->at(1);
		row1->atiput(0, 0.0);
		row1->atiput(1, 0.0);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, -stheddot);
		row2->atiput(1, 0.0);
		row2->atiput(2, ctheddot);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::rotatezrotDotrotDDot(T the, T thedot, T theddot)
	{
		auto sthe = std::sin(the);
		auto cthe = std::cos(the);
		auto sthedot = cthe * thedot;
		auto cthedot = -sthe * thedot;
		auto stheddot = cthedot * thedot + (cthe * theddot);
		auto ctheddot = -(sthedot * thedot) - (sthe * theddot);
		auto rotMat = std::make_shared<FullMatrix<T>>(3, 3);
		auto row0 = rotMat->at(0);
		row0->atiput(0, ctheddot);
		row0->atiput(1, -stheddot);
		row0->atiput(2, 0.0);
		auto row1 = rotMat->at(1);
		row1->atiput(0, stheddot);
		row1->atiput(1, ctheddot);
		row1->atiput(2, 0.0);
		auto row2 = rotMat->at(2);
		row2->atiput(0, 0.0);
		row2->atiput(1, 0.0);
		row2->atiput(2, 0.0);
		return rotMat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::identitysptr(int n)
	{
		auto mat = std::make_shared<FullMatrix<T>>(n, n);
		mat->identity();
		return mat;
	}
}

