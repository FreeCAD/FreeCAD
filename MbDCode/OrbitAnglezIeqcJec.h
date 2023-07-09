#pragma once

#include "OrbitAnglezIecJec.h"

namespace MbD {
	class OrbitAnglezIeqcJec : public OrbitAnglezIecJec
	{
		//pthezpXI pthezpEI ppthezpXIpXI ppthezpXIpEI ppthezpEIpEI 
	public:
		void calc_ppthezpEIpEI();
		void calc_ppthezpXIpEI();
		void calc_ppthezpXIpXI();
		void calc_pthezpEI();
		void calc_pthezpXI();
		void calcPostDynCorrectorIteration() override;
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

