/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "Array.h"
#include "FullColumn.h"
#include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class DiagonalMatrix;
	template<typename T>
	using DiagMatsptr = std::shared_ptr<DiagonalMatrix<T>>;
	using DiagMatDsptr = std::shared_ptr<DiagonalMatrix<double>>;

	template<typename T>
	class DiagonalMatrix : public Array<T>
	{
		//
	public:
		DiagonalMatrix() : Array<T>() {}
		DiagonalMatrix(size_t count) : Array<T>(count) {}
		DiagonalMatrix(size_t count, const T& value) : Array<T>(count, value) {}
		DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void atiputDiagonalMatrix(size_t i, std::shared_ptr<DiagonalMatrix<T>> diagMat);
		DiagMatsptr<T> times(T factor);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FMatsptr<T> timesFullMatrix(FMatsptr<T> fullMat);
		size_t nrow() {
			return this->size();
		}
		size_t ncol() {
			return this->size();
		}
		double sumOfSquares() override;
		size_t numberOfElements() override;
		void zeroSelf() override;
		double maxMagnitude() override;

		std::ostream& printOn(std::ostream& s) const override;

	};
	template<>
	inline DiagMatDsptr DiagonalMatrix<double>::times(double factor)
	{
		auto nrow = this->size();
		auto answer = std::make_shared<DiagonalMatrix<double>>(nrow);
		for (size_t i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * factor;
		}
		return answer;
	}
	template<typename T>
	inline void DiagonalMatrix<T>::atiputDiagonalMatrix(size_t i, std::shared_ptr<DiagonalMatrix<T>> diagMat)
	{
		for (size_t ii = 0; ii < diagMat->size(); ii++)
		{
			this->at(i + ii) = diagMat->at(ii);
		}
	}
	template<typename T>
	inline DiagMatsptr<T> DiagonalMatrix<T>::times(T)
	{
		assert(false);
	}
	template<typename T>
	inline FColsptr<T> DiagonalMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."

		auto nrow = this->size();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (size_t i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> DiagonalMatrix<T>::timesFullMatrix(FMatsptr<T> fullMat)
	{
		auto nrow = this->size();
		auto answer = std::make_shared<FullMatrix<T>>(nrow);
		for (size_t i = 0; i < nrow; i++)
		{
			answer->at(i) = fullMat->at(i)->times(this->at(i));
		}
		return answer;
	}
	template<>
	inline double DiagonalMatrix<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			sum += element * element;
		}
		return sum;
	}
	template<typename T>
	inline size_t DiagonalMatrix<T>::numberOfElements()
	{
		auto n = this->size();
		return n * n;
	}
	template<>
	inline void DiagonalMatrix<double>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;
		}
	}
	template<>
	inline double DiagonalMatrix<double>::maxMagnitude()
	{
		double max = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			if (element < 0.0) element = -element;
			if (max < element) max = element;
		}
		return max;
	}
	template<typename T>
	inline double DiagonalMatrix<T>::maxMagnitude()
	{
		assert(false);
		return 0.0;
	}
	template<typename T>
	inline std::ostream& DiagonalMatrix<T>::printOn(std::ostream& s) const
	{
		s << "DiagMat[";
		s << this->at(0);
		for (size_t i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "]";
		return s;
	}
}

