/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistancexyConstraintIJ.h"

namespace MbD {
	class DistancexyConstraintIqcJc : public DistancexyConstraintIJ
	{
		//pGpXI pGpEI ppGpXIpXI ppGpXIpEI ppGpEIpEI iqXI iqEI 
	public:
		DistancexyConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj);

		void addToJointForceI(FColDsptr col) override;
		void addToJointTorqueI(FColDsptr col) override;
		void calc_pGpEI();
		void calc_pGpXI();
		void calc_ppGpEIpEI();
		void calc_ppGpXIpEI();
		void calc_ppGpXIpXI();
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void init_xyIeJeIe() override;
		void useEquationNumbers() override;


		FRowDsptr pGpXI, pGpEI;
		FMatDsptr ppGpXIpXI, ppGpXIpEI, ppGpEIpEI;
		size_t iqXI, iqEI;
	};
}

