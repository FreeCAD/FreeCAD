#pragma once

#include "AngleZIecJec.h"

namespace MbD {
	class AngleZIeqcJec : public AngleZIecJec
	{
		//pthezpEI ppthezpEIpEI pcthezpEI psthezpEI 
	public:
		AngleZIeqcJec();
		AngleZIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj);

		void calcPostDynCorrectorIteration() override;
		void init_aAijIeJe() override;
		void initialize() override;
		FMatDsptr ppvaluepEIpEI() override;
		FRowDsptr pvaluepEI() override;

		FRowDsptr pthezpEI, pcthezpEI, psthezpEI;
		FMatDsptr ppthezpEIpEI;
	};
}

