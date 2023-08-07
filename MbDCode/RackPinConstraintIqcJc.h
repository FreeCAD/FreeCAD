#pragma once

#include "RackPinConstraintIJ.h"

namespace MbD {
    class RackPinConstraintIqcJc : public RackPinConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        RackPinConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj);
        
		void addToJointForceI(FColDsptr col) override;
		void addToJointTorqueI(FColDsptr col) override;
		void calc_pGpEI();
		void calc_pGpXI();
		void calc_ppGpEIpEI();
		void calc_ppGpXIpEI();
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void init_xthez() override;
		void useEquationNumbers() override;

        FRowDsptr pGpXI, pGpEI;
        FMatDsptr ppGpXIpEI, ppGpEIpEI;
        int iqXI, iqEI;

    };
}

