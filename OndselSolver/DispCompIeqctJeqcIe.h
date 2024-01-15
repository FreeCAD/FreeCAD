/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJeqcIe.h"

namespace MbD {
	class DispCompIeqctJeqcIe : public DispCompIeqcJeqcIe
	{
		//priIeJeIept ppriIeJeIepXIpt ppriIeJeIepEIpt ppriIeJeIepXJpt ppriIeJeIepEJpt ppriIeJeIeptpt 
	public:
		DispCompIeqctJeqcIe();
		DispCompIeqctJeqcIe(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

		void calc_ppvaluepEIpt() override;
		void calc_ppvaluepEJpt() override;
		void calc_ppvalueptpt() override;
		void calc_ppvaluepXIpt() override;
		void calc_ppvaluepXJpt() override;
		void calc_pvaluept() override;
		void calcPostDynCorrectorIteration() override;
		void initialize() override;
		void initializeGlobally() override;
		void preAccIC() override;
		void preVelIC() override;
		FRowDsptr ppvaluepEIpt() override;
		FRowDsptr ppvaluepEJpt() override;
		double ppvalueptpt() override;
		FRowDsptr ppvaluepXIpt() override;
		FRowDsptr ppvaluepXJpt() override;
		double pvaluept() override;

		double priIeJeIept, ppriIeJeIeptpt;
		FRowDsptr ppriIeJeIepXIpt, ppriIeJeIepEIpt, ppriIeJeIepXJpt, ppriIeJeIepEJpt;
	};
}

