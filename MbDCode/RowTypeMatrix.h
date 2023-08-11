/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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
		//double maxMagnitude() override;
		int numberOfElements() override;

		int nrow() {
			return (int) this->size();
		}
		int ncol() {
			return this->at(0)->numberOfElements();
		}
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copyFrom(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (int i = 0; i < x->size(); i++) {
			this->at(i)->copyFrom(x->at(i));
		}
	}
	//template<typename T>
	//inline double RowTypeMatrix<T>::maxMagnitude()
	//{
	//	auto max = 0.0;
	//	for (int i = 0; i < this->size(); i++)
	//	{
	//		auto element = this->at(i)->maxMagnitude();
	//		if (max < element) max = element;
	//	}
	//	return max;
	//}
	template<typename T>
	inline int RowTypeMatrix<T>::numberOfElements()
	{
		return this->nrow() * this->ncol();
	}
}

