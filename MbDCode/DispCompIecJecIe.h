#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecIe : public KinematicIeJe
    {
        //axis riIeJeIe aAjOIe rIeJeO 
    public:
        DispCompIecJecIe();
        DispCompIecJecIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

        int axis;
        double riIeJeIe;
        FColDsptr aAjOIe, rIeJeO;
    };
}

