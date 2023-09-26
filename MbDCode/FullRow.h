/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FullVector.h"
//#include "FullColumn.h"

namespace MbD {
	template<typename T>
	class FullRow;
	template<typename T>
	using FRowsptr = std::shared_ptr<FullRow<T>>;
	using FRowDsptr = std::shared_ptr<FullRow<double>>;
	template<typename T>
	class FullMatrix;
	template<typename T>
	using FMatsptr = std::shared_ptr<FullMatrix<T>>;
	template<typename T>
	class FullColumn;
	template<typename T>
	using FColsptr = std::shared_ptr<FullColumn<T>>;
	using ListFRD = std::initializer_list<FRowDsptr>;

	template<typename T>
	class FullRow : public FullVector<T>
	{
	public:
		FullRow() : FullVector<T>() {}
		FullRow(std::vector<T> vec) : FullVector<T>(vec) {}
		FullRow(int count) : FullVector<T>(count) {}
		FullRow(int count, const T& value) : FullVector<T>(count, value) {}
		FullRow(typename std::vector<T>::const_iterator begin, typename std::vector<T>::const_iterator end) : FullVector<T>(begin, end) {}
		FullRow(std::initializer_list<T> list) : FullVector<T>{ list } {}
		FRowsptr<T> times(T a);
		FRowsptr<T> negated();
		FRowsptr<T> plusFullRow(FRowsptr<T> fullRow);
		FRowsptr<T> minusFullRow(FRowsptr<T> fullRow);
		T timesFullColumn(FColsptr<T> fullCol);
		T timesFullColumn(FullColumn<T>* fullCol);
		FRowsptr<T> timesFullMatrix(FMatsptr<T> fullMat);
		FRowsptr<T> timesTransposeFullMatrix(FMatsptr<T> fullMat);
		void equalSelfPlusFullRowTimes(FRowsptr<T> fullRow, double factor);
		void equalFullRow(FRowsptr<T> fullRow);
		FColsptr<T> transpose();
		FRowsptr<T> copy();
		void atiplusFullRow(int j, FRowsptr<T> fullRow);
		FMatsptr<T> transposeTimesFullRow(FRowsptr<T> fullRow);
		std::ostream& printOn(std::ostream& s) const override;

	};

	template<>
	inline FRowDsptr FullRow<double>::times(double a)
	{
		int n = (int)this->size();
		auto answer = std::make_shared<FullRow<double>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) * a;
		}
		return answer;
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::times(T a)
	{
		assert(false);
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::negated()
	{
		return this->times(-1.0);
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::plusFullRow(FRowsptr<T> fullRow)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) + fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::minusFullRow(FRowsptr<T> fullRow)
	{
		int n = (int) this->size();
		auto answer = std::make_shared<FullRow<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i) - fullRow->at(i);
		}
		return answer;
	}
	template<typename T>
	inline T FullRow<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		return this->timesFullColumn(fullCol.get());
	}
	template<typename T>
	inline T FullRow<T>::timesFullColumn(FullColumn<T>* fullCol)
	{
		auto answer = this->at(0) * fullCol->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			answer += this->at(i) * fullCol->at(i);
		}
		return answer;
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::timesTransposeFullMatrix(FMatsptr<T> fullMat)
	{
		//"a*bT = a(1,j)b(k,j)"
		int ncol = fullMat->nrow();
		auto answer = std::make_shared<FullRow<T>>(ncol);
		for (int k = 0; k < ncol; k++) {
			answer->at(k) = this->dot(fullMat->at(k));
		}
		return answer;
	}
	template<typename T>
	inline void FullRow<T>::equalSelfPlusFullRowTimes(FRowsptr<T> fullRow, double factor)
	{
		this->equalSelfPlusFullVectortimes(fullRow, factor);
	}
	template<typename T>
	inline void FullRow<T>::equalFullRow(FRowsptr<T> fullRow)
	{
		this->equalArrayAt(fullRow, 0);
	}
	template<typename T>
	inline FColsptr<T> FullRow<T>::transpose()
	{
		return std::make_shared<FullColumn<T>>(*this);
	}
	template<>
	inline FRowDsptr FullRow<double>::copy()
	{
		auto n = (int)this->size();
		auto answer = std::make_shared<FullRow<double>>(n);
		for (int i = 0; i < n; i++)
		{
			answer->at(i) = this->at(i);
		}
		return answer;
	}
	template<typename T>
	inline void FullRow<T>::atiplusFullRow(int j1, FRowsptr<T> fullRow)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			auto j = j1 + jj;
			this->at(j) += fullRow->at(jj);
		}
	}
	template<typename T>
	inline FMatsptr<T> FullRow<T>::transposeTimesFullRow(FRowsptr<T> fullRow)
	{
		//"a*b = a(i)b(j)"
		auto nrow = (int)this->size();
		auto answer = std::make_shared<FullMatrix<double>>(nrow);
		for (int i = 0; i < nrow; i++)
		{
			answer->atiput(i, fullRow->times(this->at(i)));
		}
		return answer;
	}
	template<typename T>
	inline std::ostream& FullRow<T>::printOn(std::ostream& s) const
	{
		s << "FullRow{";
		s << this->at(0);
		for (int i = 1; i < this->size(); i++)
		{
			s << ", " << this->at(i);
		}
		s << "}";
		return s;
	}
	template<typename T>
	inline FRowsptr<T> FullRow<T>::timesFullMatrix(FMatsptr<T> fullMat)
	{
		FRowsptr<T> answer = fullMat->at(0)->times(this->at(0));
		for (int j = 1; j < (int) this->size(); j++)
		{
			answer->equalSelfPlusFullRowTimes(fullMat->at(j), this->at(j));
		}
		return answer;
			//return FRowsptr<T>();
	}
}

