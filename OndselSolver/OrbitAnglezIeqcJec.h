/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "OrbitAngleZIecJec.h"

namespace MbD {
	class OrbitAngleZIeqcJec : public OrbitAngleZIecJec
	{
		//pthezpXI pthezpEI ppthezpXIpXI ppthezpXIpEI ppthezpEIpEI 
	public:
		OrbitAngleZIeqcJec();
		OrbitAngleZIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj);

		void calc_ppthezpEIpEI();
		void calc_ppthezpXIpEI();
		void calc_ppthezpXIpXI();
		void calc_pthezpEI();
		void calc_pthezpXI();
		void calcPostDynCorrectorIteration() override;
		void init_xyIeJeIe() override;
		void initialize() override;
		FMatDsptr ppvaluepEIpEI() override;
		FMatDsptr ppvaluepXIpEI() override;
		FMatDsptr ppvaluepXIpXI() override;
		FRowDsptr pvaluepEI() override;
		FRowDsptr pvaluepXI() override;

		FRowDsptr pthezpXI, pthezpEI;
		FMatDsptr ppthezpXIpXI, ppthezpXIpEI, ppthezpEIpEI;
	};
}

