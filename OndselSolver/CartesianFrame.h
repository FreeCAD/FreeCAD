/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Item.h"

namespace MbD {
	class CartesianFrame : public Item
	{
	public:
		CartesianFrame();
		CartesianFrame(const char* str);
		void initialize() override;
	};
}

