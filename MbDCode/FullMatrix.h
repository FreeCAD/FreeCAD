#pragma once
#include <memory>

#include "RowTypeMatrix.h"

namespace MbD {
	template <typename T>
	class FullColumn;
	template <typename T>
	class FullRow;

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
		std::shared_ptr<FullColumn<T>> column(int j);
		std::shared_ptr<FullMatrix<T>> times(double a);
		void symLowerWithUpper();
	};
	template <>
	inline void FullMatrix<double>::identity() {
		this->zeroSelf();
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->at(i) = 1.0;
		}
	}
	template<typename T>
	inline std::shared_ptr<FullColumn<T>> FullMatrix<T>::column(int j) {
		size_t n = this->size();
		auto answer = std::make_shared<FullColumn<T>>(n);
		for (int i = 0; i < n; i++) {
			answer->at(i) = this->at(i)->at(j);
		}
		return answer;
	}
	template<typename T>
	inline std::shared_ptr<FullMatrix<T>> FullMatrix<T>::times(double a)
	{
		int m = this->nRow();
		auto answer = std::make_shared<FullMatrix<T>>(m);
		for (int i = 0; i < m; i++) {
			answer->at(i) = this->at(i)->times(a);
		}
		return answer;
	}
	template<typename T>
	inline void FullMatrix<T>::symLowerWithUpper()
	{
		size_t n = this->size();
		for (int i = 0; i < n; i++) {
			for (int j = i+1; j < n; j++) {
				this->at(j)->at(i) = this->at(i)->at(j);
			}
		}
	}
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatDsptr = std::shared_ptr<FullMatrix<double>>;
	using FMatFColDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>>;
	using FMatFMatDsptr = std::shared_ptr<FullMatrix<std::shared_ptr<FullMatrix<double>>>>;
	using FColFMatDsptr = std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>>;
}

