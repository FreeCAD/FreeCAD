/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistanceConstraintIJ.h"

namespace MbD {
    class DistanceConstraintIqcJc : public DistanceConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpXI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        DistanceConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj);

        void addToJointForceI(FColDsptr col) override;
        void addToJointTorqueI(FColDsptr col) override;
        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void init_distIeJe() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXI, pGpEI;
        FMatDsptr ppGpXIpXI, ppGpXIpEI, ppGpEIpEI;
        size_t iqXI, iqEI;
    };
}

