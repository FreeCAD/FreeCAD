/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DispCompIecJecKec : public KinematicIeJe
    {
        //efrmK axisK riIeJeKe aAjOKe rIeJeO 
    public:
        DispCompIecJecKec();
        DispCompIecJecKec(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk);

        double value() override;

        EndFrmsptr efrmK;
        size_t axisK;
        double riIeJeKe;
        FColDsptr aAjOKe;
        FColDsptr rIeJeO;
    };
}

