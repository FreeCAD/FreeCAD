/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIecJecKec.h"

namespace MbD {
    class DispCompIecJecKeqc : public DispCompIecJecKec
    {
        //priIeJeKepEK ppriIeJeKepEKpEK pAjOKepEKT ppAjOKepEKpEK 
    public:
        DispCompIecJecKeqc();
        DispCompIecJecKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEKpEK() override;
        FRowDsptr pvaluepEK() override;

        FRowDsptr priIeJeKepEK;
        FMatDsptr ppriIeJeKepEKpEK;
        FMatDsptr pAjOKepEKT;
        FMatFColDsptr ppAjOKepEKpEK;
    };
}

