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
		DiagonalMatrix(int count) : Array<T>(count) {}
		DiagonalMatrix(int count, const T& value) : Array<T>(count, value) {}
		DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void atiputDiagonalMatrix(int i, DiagMatsptr<T> diagMat);
		DiagMatsptr<T> times(T factor);
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		FMatsptr<T> timesFullMatrix(FMatsptr<T> fullMat);
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

		static DiagMatDsptr Identity3by3;
		static DiagMatDsptr Identity4by4;

	};
	template<>
	DiagMatDsptr DiagonalMatrix<double>::Identity3by3 = []() {
		auto identity3by3 = std::make_shared<DiagonalMatrix<double>>(3);
		for (int i = 0; i < 3; i++)
		{
			identity3by3->at(i) = 1.0;
		}
		return identity3by3;
	}();
	template<>
	DiagMatDsptr DiagonalMatrix<double>::Identity4by4 = []() {
		auto identity4by4 = std::make_shared<DiagonalMatrix<double>>(4);
		for (int i = 0; i < 4; i++)
		{
			identity4by4->at(i) = 1.0;
		}
		return identity4by4;
	}();

	template<typename T>
	inline void DiagonalMatrix<T>::atiputDiagonalMatrix(int i, DiagMatsptr<T> diagMat)
	{
		for (int ii = 0; ii < diagMat->size(); ii++)
		{
			this->at(i + ii) = diagMat->at(ii);
		}
	}
	template<>
	inline DiagMatDsptr DiagonalMatrix<double>::times(double factor)
	{
		auto nrow = (int)this->size();
		auto answer = std::make_shared<DiagonalMatrix<double>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * factor;
		}
		return answer;
	}
	template<typename T>
	inline DiagMatsptr<T> DiagonalMatrix<T>::times(T factor)
	{
		assert(false);
	}
	template<typename T>
	inline FColsptr<T> DiagonalMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		//"a*b = a(i,j)b(j) sum j."

		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullColumn<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FMatsptr<T> DiagonalMatrix<T>::timesFullMatrix(FMatsptr<T> fullMat)
	{
		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullMatrix<T>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->at(i) = fullMat->at(i)->times(this->at(i));
		}
		return answer;
	}
	template<>
	inline double DiagonalMatrix<double>::sumOfSquares()
	{
		double sum = 0.0;
		for (int i = 0; i < this->size(); i++)
		{
			double element = this->at(i);
			sum += element * element;
		}
		return sum;
	}
	template<typename T>
	inline int DiagonalMatrix<T>::numberOfElements()
	{
		auto n = (int)this->size();
		return n * n;
	}
	template<>
	inline void DiagonalMatrix<double>::zeroSelf()
	{
		for (int i = 0; i < this->size(); i++) {
			this->at(i) = 0.0;
		}
	}
	template<>
	inline double DiagonalMatrix<double>::maxMagnitude()
	{
		double max = 0.0;
		for (int i = 0; i < this->size(); i++)
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
		for (int i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "]";
		return s;
	}
}

