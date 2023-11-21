/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FullRow.h"
#include "LinearMultiStepMethod.h"

namespace MbD {
    class StableBackwardDifference : public LinearMultiStepMethod
    {
        //
    public:
        FColDsptr derivativepresentpast(int order, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast) override;
        void instantiateTaylorMatrix() override;
        void formTaylorRowwithTimeNodederivative(int i, int ii, int k) override;
        void formTaylorMatrix() override;
        double pvdotpv() override;
        FColDsptr derivativepresentpastpresentDerivativepastDerivative(int n,
            FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast,
            FColDsptr ydot, std::shared_ptr<std::vector<FColDsptr>> ydotpast) override;
        FColDsptr derivativewith(int deriv, std::shared_ptr<std::vector<FColDsptr>> series);

    };
}

