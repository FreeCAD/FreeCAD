#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecKec : public KinematicIeJe
    {
        //efrmK axisK riIeJeKe aAjOKe rIeJeO 
    public:
        DispCompIecJecKec();
        DispCompIecJecKec(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        double value() override;

        EndFrmsptr efrmK;
        int axisK;
        double riIeJeKe;
        FColDsptr aAjOKe;
        FColDsptr rIeJeO;
    };
}

