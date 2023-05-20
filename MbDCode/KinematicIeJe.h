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

        EndFrmcptr frmI, frmJ;
    };
}

