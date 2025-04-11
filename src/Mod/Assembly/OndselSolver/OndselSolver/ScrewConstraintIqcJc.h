/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ScrewConstraintIJ.h"

namespace MbD {
    class ScrewConstraintIqcJc : public ScrewConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
		ScrewConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj);

		void initzIeJeIe() override;
		void initthezIeJe() override;
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
		void init_zthez() override;
		void useEquationNumbers() override;

		FRowDsptr pGpXI, pGpEI;
		FMatDsptr ppGpXIpEI, ppGpEIpEI;
		size_t iqXI, iqEI;


    };
}

