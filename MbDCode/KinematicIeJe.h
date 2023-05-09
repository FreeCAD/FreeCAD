#pragma once

#include "Item.h"
#include "EndFramec.h"

namespace MbD {
    class KinematicIeJe : public Item
    {
        //frmI frmJ 
    public:
        KinematicIeJe();
        KinematicIeJe(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj);

        std::shared_ptr<EndFramec> frmI, frmJ;
    };
}

