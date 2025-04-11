/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "TranslationConstraintIJ.h"

namespace MbD {
    class TranslationConstraintIqcJc : public TranslationConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        TranslationConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

        void addToJointForceI(FColDsptr col) override;
        void addToJointTorqueI(FColDsptr col) override;
        void calcPostDynCorrectorIteration() override;
        void initriIeJeIe() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void useEquationNumbers() override;

        FRowDsptr pGpXI, pGpEI;
        FMatDsptr ppGpXIpEI, ppGpEIpEI;
        size_t iqXI, iqEI;
    };
}

