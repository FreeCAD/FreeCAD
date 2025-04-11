/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "TranslationConstraintIqcJc.h"

namespace MbD {
    class TranslationConstraintIqcJqc : public TranslationConstraintIqcJc
    {
        //pGpXJ pGpEJ ppGpEIpXJ ppGpEIpEJ ppGpEJpEJ iqXJ iqEJ 
    public:
        TranslationConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

        void calcPostDynCorrectorIteration() override;
        void initriIeJeIe() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void useEquationNumbers() override;

        FRowDsptr pGpXJ, pGpEJ;
        FMatDsptr ppGpEIpXJ, ppGpEIpEJ, ppGpEJpEJ;
        size_t iqXJ = SIZE_MAX, iqEJ = SIZE_MAX;
    };
}

