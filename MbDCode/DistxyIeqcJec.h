/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistxyIecJec.h"

namespace MbD {
	class DistxyIeqcJec : public DistxyIecJec
	{
		//pdistxypXI pdistxypEI ppdistxypXIpXI ppdistxypXIpEI ppdistxypEIpEI

	public:
		DistxyIeqcJec();
		DistxyIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj);

		void calc_ppdistxypEIpEI();
		void calc_ppdistxypXIpEI();
		void calc_ppdistxypXIpXI();
		void calc_pdistxypEI();
		void calc_pdistxypXI();
		void calcPostDynCorrectorIteration() override;
		void init_xyIeJeIe() override;
		void initialize() override;
		FMatDsptr ppvaluepEIpEI() override;
		FMatDsptr ppvaluepXIpEI() override;
		FMatDsptr ppvaluepXIpXI() override;
		FRowDsptr pvaluepEI() override;
		FRowDsptr pvaluepXI() override;

		FRowDsptr pdistxypXI, pdistxypEI;
		FMatDsptr ppdistxypXIpXI, ppdistxypXIpEI, ppdistxypEIpEI;
	};
}

