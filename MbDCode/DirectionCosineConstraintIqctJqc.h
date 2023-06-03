#pragma once

#include "DirectionCosineConstraintIqcJqc.h"

namespace MbD {
	class DirectionCosineConstraintIqctJqc : public DirectionCosineConstraintIqcJqc
	{
		//pGpt ppGpEIpt ppGpEJpt ppGptpt 
	public:
		DirectionCosineConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj);
		void initaAijIeJe() override;
		MbD::ConstraintType type() override;

		double pGpt;
		FRowDsptr ppGpEIpt;
		FRowDsptr ppGpEJpt;
		double ppGptpt;
	};
}
