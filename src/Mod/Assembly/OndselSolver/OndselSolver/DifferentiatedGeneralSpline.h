/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AnyGeneralSpline.h"

namespace MbD {
    class DifferentiatedGeneralSpline : public AnyGeneralSpline
    {
        //derivativeOrder
    public:
        DifferentiatedGeneralSpline() = default;
        DifferentiatedGeneralSpline(Symsptr arg, Symsptr spline, size_t derivOrder);
        double getValue() override;
        Symsptr differentiateWRTx() override;
        Symsptr clonesptr() override;

        std::ostream& printOn(std::ostream& s) const override;

        Symsptr generalSpline;
        size_t derivativeOrder;
    };
}
