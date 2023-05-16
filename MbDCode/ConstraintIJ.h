#pragma once

//#include "typedef.h"
#include "Constraint.h"
#include "EndFramec.h"

namespace MbD {
    class ConstraintIJ : public Constraint
    {
        //frmI frmJ aConstant 
    public:
        ConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);

        EndFrmcptr frmI, frmJ;
        double aConstant;
    };
}

