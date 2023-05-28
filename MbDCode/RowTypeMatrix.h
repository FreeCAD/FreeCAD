#pragma once

#include "Array.h"
#include "FullRow.h"

namespace MbD {

	template <typename T>
	class RowTypeMatrix : public Array<T>
	{
	public:
		RowTypeMatrix() {}
		RowTypeMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void copyFrom(std::shared_ptr<RowTypeMatrix<T>> x);
		void zeroSelf();
		size_t nRow() {
			return this->size();
		}
		size_t nCol() {
			return this->at(0)->size();
		}
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copyFrom(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i)->copyFrom(x->at(i));
		}
	}
	template <>
	inline void RowTypeMatrix< std::shared_ptr<FullRow<double>>>::zeroSelf() {
		for (size_t i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
}

