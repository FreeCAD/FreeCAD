/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynItem.h"

namespace MbD {

	class MBDynMarker : public MBDynItem
	{
	public:
		void parseMBDyn(std::vector<std::string>& args);
		void createASMT() override;

		std::string nodeStr;
		FColDsptr rPmP; //part to marker
		FMatDsptr aAPm;
	};
}
