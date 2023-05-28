#pragma once

#include "DirectionCosineConstraintIqcJqc.h"

namespace MbD {
	class DirectionCosineConstraintIqctJqc : public DirectionCosineConstraintIqcJqc
	{
		//pGpt ppGpEIpt ppGpEJpt ppGptpt 
	public:
		DirectionCosineConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);
		void initialize();
		void initaAijIeJe() override;
		void calcPostDynCorrectorIteration() override;

		double pGpt;
		FRowDsptr ppGpEIpt;
		FRowDsptr ppGpEJpt;
		double ppGptpt;
	};
}
