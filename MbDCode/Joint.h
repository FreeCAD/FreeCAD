#pragma once
#include <memory>
#include <vector>

#include "Item.h"
#include "EndFramec.h"
#include "Constraint.h"

namespace MbD {
    class EndFramec;
    class Constraint;

    class Joint : public Item
    {
        //frmI frmJ constraints friction 
    public:
        Joint();
        std::shared_ptr<EndFramec> frmI;
        std::shared_ptr<EndFramec> frmJ;
        std::vector<std::shared_ptr<Constraint>> constraints;

    };
}

