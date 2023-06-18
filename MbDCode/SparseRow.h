#pragma once
#include <memory>
#include <cmath>

#include "SparseVector.h"
#include "FullRow.h"

namespace MbD {
	template<typename T>
	class SparseRow : public SparseVector<T>
	{
	public:
		SparseRow(){}
		SparseRow(int n) : SparseVector<T>(n) {}
		SparseRow(std::initializer_list<std::pair<const int, T>> list) : SparseVector<T>{ list } {}
		SparseRow(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
		std::shared_ptr<SparseRow<double>> timesconditionedWithTol(double scaling, double tol);
		std::shared_ptr<SparseRow<double>> conditionedWithTol(double tol);
		void atiplusFullRow(int j, std::shared_ptr<FullRow<T>> fullRow);
		void atiplusFullRowtimes(int j, std::shared_ptr<FullRow<T>> fullRow, double factor);
	};
	using SpRowDsptr = std::shared_ptr<SparseRow<double>>;
	template<>
	inline std::shared_ptr<SparseRow<double>> SparseRow<double>::timesconditionedWithTol(double scaling, double tol)
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
	inline std::shared_ptr<SparseRow<double>> SparseRow<double>::conditionedWithTol(double tol)
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
	inline void SparseRow<T>::atiplusFullRow(int j, std::shared_ptr<FullRow<T>> fullRow)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			(*this)[j + jj] += fullRow->at(jj);
		}
	}
	template<typename T>
	inline void SparseRow<T>::atiplusFullRowtimes(int j, std::shared_ptr<FullRow<T>> fullRow, double factor)
	{
		for (int jj = 0; jj < fullRow->size(); jj++)
		{
			(*this)[j + jj] += fullRow->at(jj) * factor;
		}
	}
}

