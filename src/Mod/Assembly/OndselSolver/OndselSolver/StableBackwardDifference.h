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
        FColDsptr derivativepresentpast(size_t order, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast) override;
        void instantiateTaylorMatrix() override;
        void formTaylorRowwithTimeNodederivative(size_t i, size_t ii, size_t k) override;
        void formTaylorMatrix() override;
        double pvdotpv() override;
        FColDsptr derivativepresentpastpresentDerivativepastDerivative(size_t n,
            FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast,
            FColDsptr ydot, std::shared_ptr<std::vector<FColDsptr>> ydotpast) override;
        FColDsptr derivativewith(size_t deriv, std::shared_ptr<std::vector<FColDsptr>> series);

    };
}

