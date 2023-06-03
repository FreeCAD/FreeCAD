#pragma once
#include <memory>
#include <cmath>

#include "SparseVector.h"

namespace MbD {
	template <typename T>
	class SparseRow : public SparseVector<T>
	{
	public:
		SparseRow(){}
		SparseRow(size_t n) : SparseVector<T>(n) {}
		SparseRow(std::initializer_list<std::pair<const size_t, T>> list) : SparseVector<T>{ list } {}
		SparseRow(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
		std::shared_ptr<SparseRow<double>> timesconditionedWithTol(double scaling, double tol);
	};
	using SpRowDsptr = std::shared_ptr<SparseRow<double>>;
	template<>
	inline std::shared_ptr<SparseRow<double>> SparseRow<double>::timesconditionedWithTol(double scaling, double tol)
	{
		auto answer = std::make_shared<SparseRow<double>>(this->size());
		for (auto const& keyValue : *this)
		{
			auto val = keyValue.second * scaling;
			if (std::abs(val) >= tol) (*answer)[keyValue.first] = val;
		}
		return answer;
	}
}

