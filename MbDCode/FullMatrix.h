#pragma once
#include <memory>

#include "FullColumn.h"
#include "RowTypeMatrix.h"
#include "FullRow.h"

namespace MbD {

	template <typename T>
	class FullMatrix : public RowTypeMatrix<std::shared_ptr<FullRow<T>>>
	{
	public:
		FullMatrix() {}
		FullMatrix(int m, int n) {
			for (int i = 0; i < m; i++) {
				auto row = std::make_shared<FullRow<T>>(n);
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::shared_ptr<FullRow<T>>> listOfRows) {
			for (auto row : listOfRows)
			{
				this->push_back(row);
			}
		}
		FullMatrix(std::initializer_list<std::initializer_list<T>> list2D) {
			for (auto rowList : list2D)
			{
				auto row = std::make_shared<FullRow<T>>(rowList);
				this->push_back(row);
			}
		}
		void identity();
	};
	template <>
	inline void FullMatrix<double>::identity() {
		this->zeroSelf();
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->at(i) = 1.0;
		}
	}
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	//using FMatFColDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>>;
	using FMatFMatDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>>;
}

