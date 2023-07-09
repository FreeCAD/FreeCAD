#pragma once

#include "DispCompIeqcJecIe.h"

namespace MbD {
    class DispCompIeqcJeqcIe : public DispCompIeqcJecIe
    {
        //priIeJeIepXJ priIeJeIepEJ ppriIeJeIepEIpXJ ppriIeJeIepEIpEJ ppriIeJeIepEJpEJ 
    public:
        DispCompIeqcJeqcIe();
        DispCompIeqcJeqcIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

        FRowDsptr priIeJeIepXJ, priIeJeIepEJ;
        FMatDsptr ppriIeJeIepEIpXJ, ppriIeJeIepEIpEJ, ppriIeJeIepEJpEJ;
    };
}

