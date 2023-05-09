#pragma once

#include "Constraint.h"
#include "EndFramec.h"

namespace MbD {
    class ConstraintIJ : public Constraint
    {
        //frmI frmJ aConstant 
    public:
        ConstraintIJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj);

        std::shared_ptr<EndFramec> frmI, frmJ;
        double aConstant;
    };
}

