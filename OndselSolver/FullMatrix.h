/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <cmath>
#include <cstdint>
#include <memory>

#include "RowTypeMatrix.h"
#include "FullColumn.h"
#include "FullRow.h"

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
	class EulerParameters;
	template<typename T>
	class EulerAngleszxz;
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
		FullMatrix(size_t m) : RowTypeMatrix<FRowsptr<T>>(m)
		{
		}
		FullMatrix(size_t m, size_t n) {
			for (size_t i = 0; i < m; i++) {
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
		static FMatsptr<T> identitysptr(size_t n);
		static FMatsptr<T> tildeMatrix(FColDsptr col);
		void identity();
		FColsptr<T> column(size_t j);
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
		void atiput(size_t i, FRowsptr<T> fullRow);
		void atijput(size_t i, size_t j, T value);
		void atijputFullColumn(size_t i, size_t j, FColsptr<T> fullCol);
		void atijplusFullRow(size_t i, size_t j, FRowsptr<T> fullRow);
		void atijplusNumber(size_t i, size_t j, T value);
		void atijminusNumber(size_t i, size_t j, T value);
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
		std::shared_ptr<EulerAngleszxz<T>> eulerAngleszxz();
		bool isDiagonal();
		bool isDiagonalToWithin(double ratio);
		bool equaltol(FMatsptr<T> mat, double ratio);
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
	inline FMatsptr<T> FullMatrix<T>::identitysptr(size_t n)
	{
		auto mat = std::make_shared<FullMatrix<T>>(n, n);
		mat->identity();
		return mat;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::tildeMatrix(FColDsptr col)
	{
		//"tildeMatrix is skew symmetric matrix related to angular velocity and cross product."
		if (col->size() != 3) throw std::runtime_error("Column is not of dimension 3");
		auto tilde = std::make_shared<FullMatrix<double>>(3, 3);
		auto c0 = col->at(0);
		auto c1 = col->at(1);
		auto c2 = col->at(2);
		tilde->atijput(0, 0, 0.0);
		tilde->atijput(1, 1, 0.0);
		tilde->atijput(2, 2, 0.0);
		tilde->atijput(1, 2, -c0);
		tilde->atijput(0, 2, c1);
		tilde->atijput(0, 1, -c2);
		tilde->atijput(1, 0, c2);
		tilde->atijput(2, 0, -c1);
		tilde->atijput(2, 1, c0);
		return tilde;
	}
	template<>
	inline void FullMatrix<double>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
	template<>
	inline void FullMatrix<double>::identity() {
		this->zeroSelf();
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->at(i) = 1.0;
		}
	}
	template<typename T>
	inline FColsptr<T> FullMatrix<T>::column(size_t j) {
		auto n = this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->at(j);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::timesFullMatrix(FMatsptr<T> fullMat)
	{
		size_t m = this->nrow();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (size_t i = 0; i < m; i++) {
			answer->at(i) = this->at(i)->timesFullMatrix(fullMat);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::timesTransposeFullMatrix(FMatsptr<T> fullMat)
	{
		size_t nrow = this->nrow();
		auto answer = std::make_shared<FullMatrix<T>>(nrow);
		for (size_t i = 0; i < nrow; i++) {
			answer->at(i) = this->at(i)->timesTransposeFullMatrix(fullMat);
		}
		return answer;
	}
	template<>
	inline FMatDsptr FullMatrix<double>::times(double a)
	{
		size_t m = this->nrow();
		auto answer = std::make_shared<FullMatrix<double>>(m);
		for (size_t i = 0; i < m; i++) {
			answer->at(i) = this->at(i)->times(a);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::times(T)
	{
		assert(false);
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::transposeTimesFullMatrix(FMatsptr<T> fullMat)
	{
		return this->transpose()->timesFullMatrix(fullMat);
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::plusFullMatrix(FMatsptr<T> fullMat)
	{
		auto n = this->size();
		auto answer = std::make_shared<FullMatrix<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->plusFullRow(fullMat->at(i));
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::minusFullMatrix(FMatsptr<T> fullMat)
	{
		auto n = this->size();
		auto answer = std::make_shared<FullMatrix<T>>(n);
		for (size_t i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->minusFullRow(fullMat->at(i));
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::transpose()
	{
		size_t nrow = this->nrow();
		auto ncol = this->ncol();
		auto answer = std::make_shared<FullMatrix<T>>(ncol, nrow);
		for (size_t i = 0; i < nrow; i++) {
			auto& row = this->at(i);
			for (size_t j = 0; j < ncol; j++) {
				answer->at(j)->at(i) = row->at(j);
			}
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline void FullMatrix<T>::symLowerWithUpper()
	{
		auto n = this->size();
		for (size_t i = 0; i < n; i++) {
			for (size_t j = i + 1; j < n; j++) {
				this->at(j)->at(i) = this->at(i)->at(j);
			}
		}
	}
	template<typename T>
	inline void FullMatrix<T>::atiput(size_t i, FRowsptr<T> fullRow)
	{
		this->at(i) = fullRow;
	}
	template<typename T>
	inline void FullMatrix<T>::atijput(size_t i, size_t j, T value)
	{
		this->at(i)->atiput(j, value);
	}
	template<typename T>
	inline void FullMatrix<T>::atijputFullColumn(size_t i1, size_t j1, FColsptr<T> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->at(i1 + ii)->at(j1) = fullCol->at(ii);
		}
	}
	template<typename T>
	inline void FullMatrix<T>::atijplusFullRow(size_t i, size_t j, FRowsptr<T> fullRow)
	{
		this->at(i)->atiplusFullRow(j, fullRow);
	}
	template<typename T>
	inline void FullMatrix<T>::atijplusNumber(size_t i, size_t j, T value)
	{
		auto rowi = this->at(i);
		rowi->at(j) += value;
	}
	template<typename T>
	inline void FullMatrix<T>::atijminusNumber(size_t i, size_t j, T value)
	{
		auto rowi = this->at(i);
		rowi->at(j) -= value;
	}
	template<>
	inline double FullMatrix<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			sum += this->at(i)->sumOfSquares();
		}
		return sum;
	}
	template<typename T>
	inline double FullMatrix<T>::sumOfSquares()
	{
		assert(false);
		return 0.0;
	}
	template<typename T>
	inline void FullMatrix<T>::zeroSelf()
	{
		assert(false);
	}
	template<typename T>
	inline FMatsptr<T> FullMatrix<T>::copy()
	{
		auto m = this->size();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (size_t i = 0; i < m; i++)
		{
			answer->at(i) = this->at(i)->copy();
		}
		return answer;
	}
	template<typename T>
	inline FullMatrix<T> FullMatrix<T>::operator+(const FullMatrix<T> fullMat)
	{
		auto n = this->size();
		auto answer = FullMatrix<T>(n);
		for (size_t i = 0; i < n; i++) {
			answer.at(i) = this->at(i)->plusFullRow(fullMat.at(i));
		}
		return answer;
	}
	template<typename T>
	inline FColsptr<T> FullMatrix<T>::transposeTimesFullColumn(FColsptr<T> fullCol)
	{
		auto sptr = std::make_shared<FullMatrix<T>>(*this);
		return fullCol->transpose()->timesFullMatrix(sptr)->transpose();
	}
	template<typename T>
	inline void FullMatrix<T>::magnifySelf(T factor)
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->magnifySelf(factor);
		}
	}
	template<typename T>
	inline std::ostream& FullMatrix<T>::printOn(std::ostream& s) const
	{
		s << "FullMat[" << std::endl;
		for (size_t i = 0; i < this->size(); i++)
		{
			s << *(this->at(i)) << std::endl;
		}
		s << "]";
		return s;
	}
	template<typename T>
	inline std::shared_ptr<EulerParameters<T>> FullMatrix<T>::asEulerParameters()
	{
		//"Given [A], compute Euler parameter."

		auto traceA = this->trace();
		T dum = 0.0;
		T dumSq = 0.0;
		//auto qE = CREATE<EulerParameters<double>>::With(4); //Cannot use CREATE.h in subclasses of std::vector. Why?
		auto qE = std::make_shared<EulerParameters<T>>(4);
		qE->initialize();
		auto OneMinusTraceDivFour = (1.0 - traceA) / 4.0;
		for (size_t i = 0; i < 3; i++)
		{
			dumSq = this->at(i)->at(i) / 2.0 + OneMinusTraceDivFour;
			dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
			qE->atiput(i, dum);
		}
		dumSq = (1.0 + traceA) / 4.0;
		dum = (dumSq > 0.0) ? std::sqrt(dumSq) : 0.0;
		qE->atiput(3, dum);
		T max = 0.0;
		size_t maxE = SIZE_MAX;
		for (size_t i = 0; i < 4; i++)
		{
			auto num = qE->at(i);
			if (max < num) {
				max = num;
				maxE = i;
			}
		}

		if (maxE == 0) {
			auto FourE = 4.0 * qE->at(0);
			qE->atiput(1, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
			qE->atiput(2, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
			qE->atiput(3, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
		}
		else if (maxE == 1) {
			auto FourE = 4.0 * qE->at(1);
			qE->atiput(0, (this->at(0)->at(1) + this->at(1)->at(0)) / FourE);
			qE->atiput(2, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
			qE->atiput(3, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
		}
		else if (maxE == 2) {
			auto FourE = 4.0 * qE->at(2);
			qE->atiput(0, (this->at(0)->at(2) + this->at(2)->at(0)) / FourE);
			qE->atiput(1, (this->at(1)->at(2) + this->at(2)->at(1)) / FourE);
			qE->atiput(3, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
		}
		else if (maxE == 3) {
			auto FourE = 4.0 * qE->at(3);
			qE->atiput(0, (this->at(2)->at(1) - this->at(1)->at(2)) / FourE);
			qE->atiput(1, (this->at(0)->at(2) - this->at(2)->at(0)) / FourE);
			qE->atiput(2, (this->at(1)->at(0) - this->at(0)->at(1)) / FourE);
		}
		qE->conditionSelf();
		qE->calc();
		return qE;
	}
	template<typename T>
	inline T FullMatrix<T>::trace()
	{
		T trace = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			trace += this->at(i)->at(i);
		}
		return trace;
	}
	template<typename T>
	inline double FullMatrix<T>::maxMagnitude()
	{
		double max = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i)->maxMagnitude();
			if (max < element) max = element;
		}
		return max;
	}
	template<typename T>
	inline FColsptr<T> FullMatrix<T>::bryantAngles()
	{
		auto answer = std::make_shared<FullColumn<T>>(3);
		auto sthe1y = this->at(0)->at(2);
		T the0x, the1y, the2z, cthe0x, sthe0x, y, x;
		if (std::abs(sthe1y) > 0.9999) {
			if (sthe1y > 0.0) {
				the0x = std::atan2(this->at(1)->at(0), this->at(1)->at(1));
				the1y = M_PI / 2.0;
				the2z = 0.0;
			}
			else {
				the0x = std::atan2(this->at(2)->at(1), this->at(2)->at(0));
				the1y = M_PI / -2.0;
				the2z = 0.0;
			}
		}
		else {
			the0x = std::atan2(-this->at(1)->at(2), this->at(2)->at(2));
			cthe0x = std::cos(the0x);
			sthe0x = std::sin(the0x);
			y = sthe1y;
			if (std::abs(cthe0x) > std::abs(sthe0x)) {
				x = this->at(2)->at(2) / cthe0x;
			}
			else {
				x = this->at(1)->at(2) / -sthe0x;
			}
			the1y = std::atan2(y, x);
			the2z = std::atan2(-this->at(0)->at(1), this->at(0)->at(0));
		}
		answer->atiput(0, the0x);
		answer->atiput(1, the1y);
		answer->atiput(2, the2z);
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<EulerAngleszxz<T>> FullMatrix<T>::eulerAngleszxz()
	{
		auto answer = std::make_shared<EulerAngleszxz<T>>();
		T cthe1x, the0z, the1x, the2z, cthe2z, sthe2z;
		cthe1x = this->at(2)->at(2);
		if (Numeric::equaltol(std::abs(cthe1x), 1.0, 1.0e-6)) {
			the0z = std::atan2(this->at(1)->at(0), this->at(0)->at(0));
			if (cthe1x > 0.0) {
				the1x = 0.0;
				the2z = 0.0;
			}
			else {
				the1x = M_PI;
				the2z = 0.0;
			}
		}
		else {
			the2z = std::atan2(this->at(2)->at(0), this->at(2)->at(1));
			cthe2z = std::cos(the2z);
			sthe2z = std::sin(the2z);
			if (std::abs(cthe2z) > std::abs(sthe2z)) {
				the1x = std::atan2(this->at(2)->at(1), cthe2z * this->at(2)->at(2));
			}
			else {
				the1x = std::atan2(this->at(2)->at(0), sthe2z * this->at(2)->at(2));
			}
			the0z = std::atan2(this->at(0)->at(2), this->at(1)->at(2));
			auto aaaa = std::atan2(this->at(0)->at(2), -this->at(1)->at(2));	//Check missing minus is needed above. Smalltalk has missing minus too.
			assert(Numeric::equaltol(the0z, aaaa, 1.0e-9));
		}
		answer->atiput(0, the0z);
		answer->atiput(1, the1x);
		answer->atiput(2, the2z);
		return answer;
	}
	template<typename T>
	inline bool FullMatrix<T>::isDiagonal()
	{
		auto m = this->nrow();
		auto n = this->ncol();
		if (m != n) return false;
		for (size_t i = 0; i < m; i++)
		{
			auto rowi = this->at(i);
			for (size_t j = 0; j < n; j++)
			{
				if (i != j && rowi->at(j) != 0) return false;
			}
		}
		return true;
	}
	template<typename T>
	inline bool FullMatrix<T>::isDiagonalToWithin(double ratio)
	{
		double maxMag = this->maxMagnitude();
		auto tol = ratio * maxMag;
		auto nrow = this->nrow();
		if (nrow == this->ncol()) {
			for (size_t i = 0; i < 3; i++)
			{
				for (size_t j = i + 1; j < 3; j++)
				{
					if (std::abs(this->at(i)->at(j)) > tol) return false;
					if (std::abs(this->at(j)->at(i)) > tol) return false;
				}
			}
			return true;
		}
		else {
			return false;
		}
	}
	template<typename T>
	inline bool FullMatrix<T>::equaltol(FMatsptr<T> mat2, double tol)
	{
		if (this->size() != mat2->size()) return false;
		for (size_t i = 0; i < this->size(); i++)
		{
			auto& rowi = this->at(i);
			auto& rowi2 = mat2->at(i);
			if (rowi->size() != rowi2->size()) return false;
			for (size_t j = 0; j < rowi->size(); j++)
			{
				if (!Array<double>::equaltol((double)rowi->at(j), (double)rowi2->at(j), tol)) return false;
			}
		}
		return true;
	}
	template<typename T>
	inline std::shared_ptr<DiagonalMatrix<T>> FullMatrix<T>::asDiagonalMatrix()
	{
		size_t nrow = this->nrow();
		auto diagMat = std::make_shared<DiagonalMatrix<T>>(nrow);
		for (size_t i = 0; i < nrow; i++)
		{
			diagMat->atiput(i, this->at(i)->at(i));
		}
		return diagMat;
	}
	template<typename T>
	inline void FullMatrix<T>::conditionSelfWithTol(double tol)
	{
		for (auto row : *this) {
			row->conditionSelfWithTol(tol);
		}
	}
	template<typename T>
	inline FColsptr<T> FullMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		return this->timesFullColumn(fullCol.get());
	}
	template<typename T>
	inline FColsptr<T> FullMatrix<T>::timesFullColumn(FullColumn<T>* fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."
		auto nrow = this->nrow();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (size_t i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i)->timesFullColumn(fullCol);
		}
		return answer;
	}
}

