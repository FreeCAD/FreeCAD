#pragma once

#include "Array.h"
#include "FullRow.h"

namespace MbD {

	template <typename T>
	class RowTypeMatrix : public Array<T>
	{
	public:
		RowTypeMatrix() {}
		RowTypeMatrix(size_t m) : Array<T>(m) {}
		RowTypeMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void copyFrom(std::shared_ptr<RowTypeMatrix<T>> x);
		virtual void zeroSelf() = 0;
		virtual void atijplusNumber(size_t i, size_t j, double value) = 0;
		size_t nrow() {
			return this->size();
		}
		size_t ncol() {
			return this->at(0)->size();
		}
		size_t numberOfElements() override;
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copyFrom(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i)->copyFrom(x->at(i));
		}
	}
	template<typename T>
	inline size_t RowTypeMatrix<T>::numberOfElements()
	{
		return this->nrow() * this->ncol();
	}
}

