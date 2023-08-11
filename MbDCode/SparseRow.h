/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>
#include <cmath>

#include "SparseVector.h"
#include "FullRow.h"

namespace MbD {
	template<typename T>
	class SparseRow;
	template<typename T>
	using SpRowsptr = std::shared_ptr<SparseRow<T>>;
	using SpRowDsptr = std::shared_ptr<SparseRow<double>>;

	template<typename T>
	class SparseRow : public SparseVector<T>
	{
	public:
		SparseRow(){}
		SparseRow(int n) : SparseVector<T>(n) {}
		SparseRow(std::initializer_list<std::pair<const int, T>> list) : SparseVector<T>{ list } {}
		SparseRow(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
		SpRowDsptr timesconditionedWithTol(double scaling, double tol);
		SpRowDsptr conditionedWithTol(double tol);
		void atiplusFullRow(int j, FRowsptr<T> fullRow);
		void atiminusFullRow(int j, FRowsptr<T> fullRow);
		void atiplusFullRowtimes(int j, FRowsptr<T> fullRow, double factor);
		T timesFullColumn(FColsptr<T> fullCol);

	};
	template<>
	inline SpRowDsptr SparseRow<double>::timesconditionedWithTol(double scaling, double tol)
	{
		auto answer = std::make_shared<SparseRow<double>>(this->numberOfElements());
		for (auto const& keyValue : *this)
		{
			auto val = keyValue.second * scaling;
			if (std::abs(val) >= tol) (*answer)[keyValue.first] = val;
		}
		return answer;
	}
	template<>
	inline SpRowDsptr SparseRow<double>::conditionedWithTol(double tol)
	{
		auto answer = std::make_shared<SparseRow<double>>(this->numberOfElements());
		for (auto const& keyValue : *this)
		{
			auto val = keyValue.second;
			if (std::abs(val) >= tol) (*answer)[keyValue.first] = val;
		}
		return answer;
	}
	template<typename T>
	inline void SparseRow<T>::atiplusFullRow(int j, FRowsptr<T> fullRow)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			(*this)[j + jj] += fullRow->at(jj);
		}
	}
	template<typename T>
	inline void SparseRow<T>::atiminusFullRow(int j, FRowsptr<T> fullRow)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			(*this)[j + jj] -= fullRow->at(jj);
		}
	}
	template<typename T>
	inline void SparseRow<T>::atiplusFullRowtimes(int j, FRowsptr<T> fullRow, double factor)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			(*this)[j + jj] += fullRow->at(jj) * factor;
		}
	}
	template<typename T>
	inline T SparseRow<T>::timesFullColumn(FColsptr<T> fullCol)
	{
		T sum = 0.0;
		for (auto const& keyValue : *this) {
			sum += fullCol->at(keyValue.first) * keyValue.second;
		}
		return sum;
	}
}

