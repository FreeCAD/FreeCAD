/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistanceConstraintIqcJqc.h"

namespace MbD {
    class DistanceConstraintIqctJqc : public DistanceConstraintIqcJqc
    {
        //pGpt ppGpXIpt ppGpEIpt ppGpXJpt ppGpEJpt ppGptpt 
    public:
        DistanceConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj);
        ConstraintType type() override;

        double pGpt, ppGptpt;
        FRowDsptr ppGpXIpt, ppGpEIpt, ppGpXJpt, ppGpEJpt;
            
    };
}

