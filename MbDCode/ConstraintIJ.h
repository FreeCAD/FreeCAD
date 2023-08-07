#pragma once

#include "Constraint.h"
//#include "EndFramec.h"  //EndFrmsptr is defined

namespace MbD {
    class EndFramec;
    using EndFrmsptr = std::shared_ptr<EndFramec>;
    
    class ConstraintIJ : public Constraint
    {
        //frmI frmJ aConstant 
    public:
        ConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj);

        void initialize() override;
        void setConstant(double value) override;

        EndFrmsptr frmI, frmJ;
        double aConstant;
    };
}

