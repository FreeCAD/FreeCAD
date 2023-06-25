#pragma once

#include "DirectionCosineConstraintIqcJqc.h"

namespace MbD {
	class DirectionCosineConstraintIqctJqc : public DirectionCosineConstraintIqcJqc
	{
		//pGpt ppGpEIpt ppGpEJpt ppGptpt 
	public:
		DirectionCosineConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);
		void initaAijIeJe() override;
		MbD::ConstraintType type() override;
		void preVelIC() override;
		void fillVelICError(FColDsptr col) override;
		void preAccIC() override;
		void fillAccICIterError(FColDsptr col) override;

		double pGpt;
		FRowDsptr ppGpEIpt;
		FRowDsptr ppGpEJpt;
		double ppGptpt;
	};
}
