/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include <sstream> 

#include "RowTypeMatrix.h"
#include "SparseRow.h"
#include "DiagonalMatrix.h"
#include "FullMatrix.h"

namespace MbD {
	template<typename T>
	class SparseMatrix;
	using SpMatDsptr = std::shared_ptr<SparseMatrix<double>>;
	template<typename T>
	using SpMatsptr = std::shared_ptr<SparseMatrix<T>>;
	class GESpMatParPvPrecise;

	template<typename T>
	class SparseMatrix : public RowTypeMatrix<SpRowsptr<T>>
	{
	public:
		SparseMatrix(size_t m) : RowTypeMatrix<SpRowsptr<T>>(m)
		{
		}
		SparseMatrix(size_t m, size_t n) {
			for (size_t i = 0; i < m; i++)
			{
				auto row = std::make_shared<SparseRow<T>>(n);
				this->push_back(row);
			}
		}
		SparseMatrix(std::initializer_list<std::initializer_list<std::initializer_list<double>>> list2D) {
			for (auto& rowList : list2D)
			{
				auto row = std::make_shared<SparseRow<T>>(rowList);
				this->push_back(row);
			}
		}
		void atiput(size_t i, SpRowsptr<T> spRow);
		void atijplusDiagonalMatrix(size_t i, size_t j, DiagMatDsptr diagMat);
		void atijminusDiagonalMatrix(size_t i, size_t j, DiagMatDsptr diagMat);
		double sumOfSquares() override;
		void zeroSelf() override;
		void atijplusFullRow(size_t i, size_t j, FRowsptr<T> fullRow);
		void atijplusFullColumn(size_t i, size_t j, FColsptr<T> fullCol);
		void atijplusFullMatrix(size_t i, size_t j, FMatDsptr fullMat);
		void atijminusFullMatrix(size_t i, size_t j, FMatDsptr fullMat);
		void atijplusTransposeFullMatrix(size_t i, size_t j, FMatDsptr fullMat);
		void atijplusFullMatrixtimes(size_t i, size_t j, FMatDsptr fullMat, T factor);
		void atijminusFullColumn(size_t i, size_t j, FColDsptr fullCol);
		void atijminusTransposeFullMatrix(size_t i, size_t j, FMatDsptr fullMat);
		void atijplusNumber(size_t i, size_t j, double value);
		void atijminusNumber(size_t i, size_t j, double value);
		void atijput(size_t i, size_t j, T value);
		double maxMagnitude() override;
		FColsptr<T> timesFullColumn(FColsptr<T> fullCol);
		SpMatsptr<T> plusSparseMatrix(SpMatsptr<T> spMat);
		std::shared_ptr<SparseMatrix<T>> clonesptr();
		void magnifySelf(T factor);

		std::ostream& printOn(std::ostream& s) const override;

	};

	template<typename T>
	inline void SparseMatrix<T>::atiput(size_t i, SpRowsptr<T> spRow)
	{
		this->at(i) = spRow;
	}

	template<typename T>
	inline void SparseMatrix<T>::atijplusDiagonalMatrix(size_t i, size_t j, DiagMatDsptr diagMat)
	{
		auto n = diagMat->nrow();
		for (size_t ii = 0; ii < n; ii++)
		{
			this->atijplusNumber(i + ii, j + ii, diagMat->at(ii));
		}
	}

	template<>
	inline void SparseMatrix<double>::atijminusDiagonalMatrix(size_t i1, size_t j1, DiagMatDsptr diagMat)
	{
		auto n = diagMat->nrow();
		for (size_t ii = 0; ii < n; ii++)
		{
			this->atijminusNumber(i1 + ii, j1 + ii, diagMat->at(ii));
		}
	}
	template<typename T>
	inline double SparseMatrix<T>::sumOfSquares()
	{
		double sum = 0.0;
		for (size_t i = 0; i < this->size(); i++)
		{
			sum += this->at(i)->sumOfSquares();
		}
		return sum;
	}
	template<>
	inline void SparseMatrix<double>::zeroSelf()
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullRow(size_t i, size_t j, FRowsptr<T> fullRow)
	{
		this->at(i)->atiplusFullRow(j, fullRow);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullColumn(size_t i, size_t j, FColsptr<T> fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->atijplusNumber(i + ii, j, fullCol->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusFullColumn(size_t i, size_t j, FColDsptr fullCol)
	{
		for (size_t ii = 0; ii < fullCol->size(); ii++)
		{
			this->atijminusNumber(i + ii, j, fullCol->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullMatrix(size_t i, size_t j, FMatDsptr fullMat)
	{
		for (size_t ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiplusFullRow(j, fullMat->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusFullMatrix(size_t i, size_t j, FMatDsptr fullMat)
	{
		for (size_t ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiminusFullRow(j, fullMat->at(ii));
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusTransposeFullMatrix(size_t i, size_t j, FMatDsptr fullMat)
	{
		for (size_t ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->atijplusFullColumn(i, j + ii, fullMat->at(ii)->transpose());
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusTransposeFullMatrix(size_t i, size_t j, FMatDsptr fullMat)
	{
		for (size_t ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->atijminusFullColumn(i, j + ii, fullMat->at(ii)->transpose());
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusFullMatrixtimes(size_t i, size_t j, FMatDsptr fullMat, T factor)
	{
		for (size_t ii = 0; ii < fullMat->nrow(); ii++)
		{
			this->at(i + ii)->atiplusFullRowtimes(j, fullMat->at(ii), factor);
		}
	}
	template<typename T>
	inline void SparseMatrix<T>::atijplusNumber(size_t i, size_t j, double value)
	{
		this->at(i)->atiplusNumber(j, value);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijminusNumber(size_t i, size_t j, double value)
	{
		this->at(i)->atiminusNumber(j, value);
	}
	template<typename T>
	inline void SparseMatrix<T>::atijput(size_t i, size_t j, T value)
	{
		this->at(i)->atiput(j, value);
	}
	template<typename T>
	inline double SparseMatrix<T>::maxMagnitude()
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
	inline std::ostream& SparseMatrix<T>::printOn(std::ostream& s) const
	{
		s << "SpMat[" << std::endl;
		for (size_t i = 0; i < this->size(); i++)
		{
			s << *(this->at(i)) << std::endl;
		}
		s << "]" << std::endl;
		return s;
	}
	template<typename T>
	inline FColsptr<T> SparseMatrix<T>::timesFullColumn(FColsptr<T> fullCol)
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
	template<typename T>
	inline SpMatsptr<T> SparseMatrix<T>::plusSparseMatrix(SpMatsptr<T> spMat)
	{
		//"a + b."
		//"Assume all checking of validity of this operation has been done."
		//"Just evaluate quickly."

		auto answer = clonesptr();
		for (size_t i = 0; i < answer->size(); i++)
		{
			answer->atiput(i, answer->at(i)->plusSparseRow(spMat->at(i)));
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<SparseMatrix<T>> SparseMatrix<T>::clonesptr()
	{
		return std::make_shared<SparseMatrix<T>>(*this);
	}
	template<typename T>
	inline void SparseMatrix<T>::magnifySelf(T factor)
	{
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->magnifySelf(factor);
		}
	}
}
