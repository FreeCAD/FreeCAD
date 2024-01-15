/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistancexyConstraintIqcJc.h"

namespace MbD {
	class DistancexyConstraintIqcJqc : public DistancexyConstraintIqcJc
	{
		//pGpXJ pGpEJ ppGpXIpXJ ppGpEIpXJ ppGpXJpXJ ppGpXIpEJ ppGpEIpEJ ppGpXJpEJ ppGpEJpEJ iqXJ iqEJ 
	public:
		DistancexyConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj);

		void calc_pGpXJ();
		void calc_pGpEJ();
		void calc_ppGpXIpXJ();
		void calc_ppGpEIpXJ();
		void calc_ppGpXJpXJ();
		void calc_ppGpXIpEJ();
		void calc_ppGpEIpEJ();
		void calc_ppGpXJpEJ();
		void calc_ppGpEJpEJ();
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void init_xyIeJeIe() override;
		void useEquationNumbers() override;

		FRowDsptr pGpXJ, pGpEJ;
		FMatDsptr ppGpXIpXJ, ppGpEIpXJ, ppGpXJpXJ, ppGpXIpEJ, ppGpEIpEJ, ppGpXJpEJ, ppGpEJpEJ;
		size_t iqXJ, iqEJ;
	};
}

