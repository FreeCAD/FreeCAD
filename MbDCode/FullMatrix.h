#pragma once
#include <memory>

#include "RowTypeMatrix.h"
#include "FullColumn.h"
#include "FullRow.h"

namespace MbD {
	//class FullColumn<double>;

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
	};

	typedef std::initializer_list<std::initializer_list<double>> ListListD;
	typedef std::initializer_list<FullRowDptr> ListFRD;
	typedef std::shared_ptr<FullMatrix<double>> FullMatDptr;
	//typedef std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> FMatFColDptr;
	typedef std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>> FMatFMatDptr;

}

