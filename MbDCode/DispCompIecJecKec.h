#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecKec : public KinematicIeJe
    {
        //efrmK axisK riIeJeKe aAjOKe rIeJeO 
    public:

        EndFrmcptr efrmK;
        int axisK;
        double riIeJeKe;
        FColDsptr aAjOKe;
        FColDsptr rIeJeO;
    };
}

