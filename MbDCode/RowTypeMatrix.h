#pragma once

#include "Array.h"
#include "FullRow.h"

namespace MbD {

	template<typename T>
	class RowTypeMatrix : public Array<T>
	{
	public:
		RowTypeMatrix() {}
		RowTypeMatrix(int m) : Array<T>(m) {}
		RowTypeMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void copyFrom(std::shared_ptr<RowTypeMatrix<T>> x);
		virtual void zeroSelf() = 0;
		int nrow() {
			return (int) this->size();
		}
		int ncol() {
			return this->at(0)->numberOfElements();
		}
		int numberOfElements() override;
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copyFrom(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i)->copyFrom(x->at(i));
		}
	}
	template<typename T>
	inline int RowTypeMatrix<T>::numberOfElements()
	{
		return this->nrow() * this->ncol();
	}
}

