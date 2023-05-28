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
		void copy(std::shared_ptr<RowTypeMatrix<T>> x);
		void zeroSelf();
		int nRow() {
			return this->size();
		}
		int nCol() {
			return this->at(0)->size();
		}
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copy(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i)->copy(x->at(i));
		}
	}
	template <>
	inline void RowTypeMatrix< std::shared_ptr<FullRow<double>>>::zeroSelf() {
		for (int i = 0; i < this->size(); i++) {
			this->at(i)->zeroSelf();
		}
	}
}

