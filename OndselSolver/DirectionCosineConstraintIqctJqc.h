/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DirectionCosineConstraintIqcJqc.h"

namespace MbD {
	class DirectionCosineConstraintIqctJqc : public DirectionCosineConstraintIqcJqc
	{
		//pGpt ppGpEIpt ppGpEJpt ppGptpt 
	public:
		DirectionCosineConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

		void fillAccICIterError(FColDsptr col) override;
		void fillVelICError(FColDsptr col) override;
		void initaAijIeJe() override;
		void preAccIC() override;
		void preVelIC() override;
		ConstraintType type() override;

		double pGpt;
		FRowDsptr ppGpEIpt;
		FRowDsptr ppGpEJpt;
		double ppGptpt;
	};
}
