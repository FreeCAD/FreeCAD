#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecKec : public KinematicIeJe
    {
        //efrmK axisK riIeJeKe aAjOKe rIeJeO 
    public:
        DispCompIecJecKec();
        DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, size_t axisk);
        
        virtual double value();
        EndFrmcptr efrmK;
        size_t axisK;
        double riIeJeKe;
        FColDsptr aAjOKe;
        FColDsptr rIeJeO;
    };
}

