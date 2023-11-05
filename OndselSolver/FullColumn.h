/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <string>
#include <sstream> 

#include "FullVector.h"
#include "FullColumn.ref.h"
#include "FullRow.ref.h"
#include "FullMatrix.ref.h"

namespace MbD {
	class Symbolic;

	template<typename T>
	class FullColumn : public FullVector<T>
	{
	public:
		FullColumn() : FullVector<T>() {}
		FullColumn(std::vector<T> vec) : FullVector<T>(vec) {}
		FullColumn(int count) : FullVector<T>(count) {}
		FullColumn(int count, const T& value) : FullVector<T>(count, value) {}
		FullColumn(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end) : FullVector<T>(begin, end) {}
		FullColumn(std::initializer_list<T> list) : FullVector<T>{ list } {}
		FColsptr<T> plusFullColumn(FColsptr<T> fullCol);
		FColsptr<T> minusFullColumn(FColsptr<T> fullCol);
		FColsptr<T> times(T a);
		FColsptr<T> negated();
		void atiputFullColumn(int i, FColsptr<T> fullCol);
		void atiplusFullColumn(int i, FColsptr<T> fullCol);
		void equalSelfPlusFullColumnAt(FColsptr<T> fullCol, int i);
		void atiminusFullColumn(int i, FColsptr<T> fullCol);
		void equalFullColumnAt(FColsptr<T> fullCol, int i);
		FColsptr<T> copy();
		FRowsptr<T> transpose();
		void atiplusFullColumntimes(int i, FColsptr<T> fullCol, T factor);
		T transposeTimesFullColumn(const FColsptr<T> fullCol);		
		void equalSelfPlusFullColumntimes(FColsptr<T> fullCol, T factor);
		FColsptr<T> cross(FColsptr<T> fullCol);
		FColsptr<T> simplified();

		std::ostream& printOn(std::ostream& s) const override;
	};
    template class FullVector<double>;
}

