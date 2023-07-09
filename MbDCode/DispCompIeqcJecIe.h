#pragma once

#include "DispCompIecJecIe.h"

namespace MbD {
    class DispCompIeqcJecIe : public DispCompIecJecIe
    {
        //priIeJeIepXI priIeJeIepEI ppriIeJeIepXIpEI ppriIeJeIepEIpEI pAjOIepEIT ppAjOIepEIpEI 
    public:
        DispCompIeqcJecIe();
        DispCompIeqcJecIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

        FRowDsptr priIeJeIepXI, priIeJeIepEI;
        FMatDsptr ppriIeJeIepXIpEI, ppriIeJeIepEIpEI, pAjOIepEIT;
        FMatFColDsptr ppAjOIepEIpEI;
    };
}

