#pragma once
#include "SparseVector.h"

namespace MbD {
	template <typename T>
	class SparseColumn : public SparseVector<T>
	{
	public:
		SparseColumn() {}
		SparseColumn(std::initializer_list<std::pair<const int, T>> list) : SparseVector<T>{ list } {}
		SparseColumn(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
	};
}

