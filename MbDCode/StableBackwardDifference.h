/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "LinearMultiStepMethod.h"

namespace MbD {
    class StableBackwardDifference : public LinearMultiStepMethod
    {
        //
    public:
        void formTaylorMatrix() override;
        void instantiateTaylorMatrix() override;
        void formTaylorRowwithTimeNodederivative(int i, int ii, int k) override;
    };
}

