#pragma once

#include "Constraint.h"
#include "EndFramec.h"  //EndFrmcptr is defined

namespace MbD {
    class ConstraintIJ : public Constraint
    {
        //frmI frmJ aConstant 
    public:
        ConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);
        void initialize() override;

        EndFrmcptr frmI, frmJ;
        double aConstant;
    };
}

