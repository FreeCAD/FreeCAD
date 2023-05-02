#pragma once
#include "RowTypeMatrix.h"
#include "SparseRow.h"

namespace MbD {
	template <typename T>
	class SparseMatrix : public RowTypeMatrix<std::shared_ptr<SparseRow<T>>>
	{
	public:
		SparseMatrix(std::initializer_list<std::initializer_list<std::initializer_list<double>>> list2D) {
			for (auto rowList : list2D)
			{
				auto row = std::make_shared<SparseRow<T>>(rowList);
				this->push_back(row);
			}
		}
	};
}

typedef std::shared_ptr<MbD::SparseMatrix<double>> SpMatDptr;
typedef std::initializer_list<std::initializer_list<std::initializer_list<double>>> ListListPairD;

