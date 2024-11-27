/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "ConstVelConstraintIJ.h"

namespace MbD {
    class ConstVelConstraintIqcJc : public ConstVelConstraintIJ
    {
        //pGpEI ppGpEIpEI iqEI 
    public:
        ConstVelConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj);
        
        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void initA01IeJe() override;
        void initA10IeJe() override;
        void initialize() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEI;
        FMatDsptr ppGpEIpEI;
        size_t iqEI = SIZE_MAX;
    };
}
