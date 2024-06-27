/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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

        EndFrmsptr frmI, frmJ;
    };
}

