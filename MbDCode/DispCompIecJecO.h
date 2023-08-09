/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "KinematicIeJe.h"

namespace MbD {
	class DispCompIecJecO : public KinematicIeJe
	{
		//axis riIeJeO 
	public:
		DispCompIecJecO();
		DispCompIecJecO(EndFrmsptr frmi, EndFrmsptr frmj, int axis);

		void calcPostDynCorrectorIteration() override;
		double value() override;

		int axis = -1;
		double riIeJeO;
	};
}

