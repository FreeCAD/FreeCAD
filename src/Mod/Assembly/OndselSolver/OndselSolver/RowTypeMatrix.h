/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Array.h"

namespace MbD {

	template<typename T>
	class RowTypeMatrix : public Array<T>
	{
	public:
		RowTypeMatrix() {}
		RowTypeMatrix(size_t m) : Array<T>(m) {}
		RowTypeMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
		void copyFrom(std::shared_ptr<RowTypeMatrix<T>> x);
		virtual void zeroSelf() override = 0;
		//double maxMagnitude() override;
		size_t numberOfElements() override;
        size_t nrow() {
            return this->size();
        }
        size_t ncol() {
            return this->at(0)->numberOfElements();
        }
	};

	template<typename T>
	inline void RowTypeMatrix<T>::copyFrom(std::shared_ptr<RowTypeMatrix<T>> x)
	{
		for (size_t i = 0; i < x->size(); i++) {
			this->at(i)->copyFrom(x->at(i));
		}
	}
	//template<typename T>
	//inline double RowTypeMatrix<T>::maxMagnitude()
	//{
	//	double max = 0.0;
	//	for (size_t i = 0; i < this->size(); i++)
	//	{
	//		double element = this->at(i)->maxMagnitude();
	//		if (max < element) max = element;
	//	}
	//	return max;
	//}
	template<typename T>
	inline size_t RowTypeMatrix<T>::numberOfElements()
	{
		return this->nrow() * this->ncol();
	}
}

