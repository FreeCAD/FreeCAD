/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "DirectionCosineConstraintIJ.h"

namespace MbD {
	class DirectionCosineConstraintIqcJc : public DirectionCosineConstraintIJ
	{
		//pGpEI ppGpEIpEI iqEI 
	public:
		DirectionCosineConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj);

		void addToJointTorqueI(FColDsptr col) override;
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void initaAijIeJe() override;
		void useEquationNumbers() override;

		FRowDsptr pGpEI;
		FMatDsptr ppGpEIpEI;
		size_t iqEI = SIZE_MAX;
	};
}

