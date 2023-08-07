#pragma once

#include "DirectionCosineConstraintIJ.h"

namespace MbD {
	class DirectionCosineConstraintIqcJc : public DirectionCosineConstraintIJ
	{
		//pGpEI ppGpEIpEI iqEI 
	public:
		DirectionCosineConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

		void addToJointTorqueI(FColDsptr col) override;
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void initaAijIeJe() override;
		void useEquationNumbers() override;

		FRowDsptr pGpEI;
		FMatDsptr ppGpEIpEI;
		int iqEI = -1;
	};
}

