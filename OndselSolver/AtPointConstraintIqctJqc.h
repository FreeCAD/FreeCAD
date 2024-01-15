/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AtPointConstraintIqcJqc.h"

namespace MbD {
    class AtPointConstraintIqctJqc : public AtPointConstraintIqcJqc
    {
        //pGpt ppGpEIpt ppGptpt 
    public:
        AtPointConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillVelICError(FColDsptr col) override;
        void initializeGlobally() override;
        void initriIeJeO() override;
        void preAccIC() override;
        void preVelIC() override;
        ConstraintType type() override;

        double pGpt;
        FRowDsptr ppGpEIpt;
        double ppGptpt;

    };
}

