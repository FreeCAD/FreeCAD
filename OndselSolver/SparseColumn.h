/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "SparseVector.h"

namespace MbD {
	template<typename T>
	class SparseColumn : public SparseVector<T>
	{
	public:
		SparseColumn() {}
		SparseColumn(std::initializer_list<std::pair<const size_t, T>> list) : SparseVector<T>{ list } {}
		SparseColumn(std::initializer_list<std::initializer_list<T>> list) : SparseVector<T>{ list } {}
	};
}

