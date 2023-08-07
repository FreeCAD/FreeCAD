#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecIe : public KinematicIeJe
    {
        //axis riIeJeIe aAjOIe rIeJeO 
    public:
        DispCompIecJecIe();
        DispCompIecJecIe(EndFrmsptr frmi, EndFrmsptr frmj, int axis);

        void calc_value() override;
        void calcPostDynCorrectorIteration() override;
        double value() override;

        int axis;
        double riIeJeIe;
        FColDsptr aAjOIe, rIeJeO;
    };
}

