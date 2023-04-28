#pragma once
#include "Array.h"
namespace MbD {
	template <typename T>
	class RowTypeMatrix : public Array<T>
	{
	public:
		RowTypeMatrix() {}
		RowTypeMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void copy(RowTypeMatrix<T>* x);
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copy(RowTypeMatrix<T>* x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i)->copy(x->at(i));
		}
	}
}

