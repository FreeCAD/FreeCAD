#pragma once

#include "AnyGeneralSpline.h"

namespace MbD {
    class DifferentiatedGeneralSpline : public AnyGeneralSpline
    {
        //derivativeOrder
    public:
        DifferentiatedGeneralSpline() = default;
        DifferentiatedGeneralSpline(Symsptr arg, Symsptr spline, int derivOrder);
        double getValue() override;
        Symsptr differentiateWRTx() override;
        Symsptr clonesptr() override;

        std::ostream& printOn(std::ostream& s) const override;

        Symsptr generalSpline;
        int derivativeOrder;
    };
}
