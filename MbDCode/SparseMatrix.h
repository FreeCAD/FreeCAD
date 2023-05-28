#pragma once
#include "RowTypeMatrix.h"
#include "SparseRow.h"

namespace MbD {
	template <typename T>
	class SparseMatrix : public RowTypeMatrix<std::shared_ptr<SparseRow<T>>>
	{
	public:
		SparseMatrix(int m, int n) {
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
	};
	using SpMatDptr = std::shared_ptr<SparseMatrix<double>>;
}