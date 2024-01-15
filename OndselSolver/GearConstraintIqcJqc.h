/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "GearConstraintIqcJc.h"

namespace MbD {
	class GearConstraintIqcJqc : public GearConstraintIqcJc
	{
		//pGpXJ pGpEJ ppGpXIpXJ ppGpXIpEJ ppGpEIpXJ ppGpEIpEJ ppGpXJpXJ ppGpXJpEJ ppGpEJpEJ iqXJ iqEJ 
	public:
		GearConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj);

		void calc_pGpEJ();
		void calc_pGpXJ();
		void calc_ppGpEIpEJ();
		void calc_ppGpEIpXJ();
		void calc_ppGpEJpEJ();
		void calc_ppGpXIpEJ();
		void calc_ppGpXIpXJ();
		void calc_ppGpXJpEJ();
		void calc_ppGpXJpXJ();
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void initorbitsIJ() override;
		void useEquationNumbers() override;

		FRowDsptr pGpXJ, pGpEJ;
		FMatDsptr ppGpXIpXJ, ppGpXIpEJ, ppGpEIpXJ, ppGpEIpEJ, ppGpXJpXJ, ppGpXJpEJ, ppGpEJpEJ;
		size_t iqXJ, iqEJ;

	};
}

