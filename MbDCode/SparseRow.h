#pragma once
#include <memory>
#include <cmath>

#include "SparseVector.h"

namespace MbD {
	template <typename T>
	class SparseRow : public SparseVector<T>
	{
	public:
		SparseRow() {}
		SparseRow(std::initializer_list<std::pair<const int, T>> list) : SparseVector<T>{ list } {}
		SparseRow(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
	};
	typedef std::shared_ptr<SparseRow<double>> SpRowDptr;
}

