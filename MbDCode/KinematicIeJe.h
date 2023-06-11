#pragma once

#include "Item.h"
#include "EndFramec.h"

namespace MbD {
    class KinematicIeJe : public Item
    {
        //frmI frmJ 
    public:
        KinematicIeJe();
        KinematicIeJe(EndFrmcptr frmi, EndFrmcptr frmj);
        virtual FRowDsptr pvaluepXJ() = 0;
        virtual FRowDsptr pvaluepEJ() = 0;
        virtual FMatDsptr ppvaluepXJpEK() = 0;
        virtual FMatDsptr ppvaluepEJpEK() = 0;
        virtual FMatDsptr ppvaluepEJpEJ() = 0;


        EndFrmcptr frmI, frmJ;
    };
}

