#pragma once

#include "GearConstraintIJ.h"

namespace MbD {
	class GearConstraintIqcJc : public GearConstraintIJ
	{
		//pGpXI pGpEI ppGpXIpXI ppGpXIpEI ppGpEIpEI iqXI iqEI 
	public:
		GearConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj);

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
		void initorbitsIJ() override;
		void useEquationNumbers() override;

		FRowDsptr pGpXI, pGpEI;
		FMatDsptr ppGpXIpXI, ppGpXIpEI, ppGpEIpEI;
		int iqXI, iqEI;
	};
}

