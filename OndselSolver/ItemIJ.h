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
	class EndFramec;
	using EndFrmsptr = std::shared_ptr<EndFramec>;

	class ItemIJ : public Item
	{
		//
	public:
		ItemIJ();
		ItemIJ(const std::string& str);
		ItemIJ(EndFrmsptr frmi, EndFrmsptr frmj);
		virtual void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ);

		EndFrmsptr frmI, frmJ;

	};
}
