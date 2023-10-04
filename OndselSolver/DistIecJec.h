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
    class DistIecJec : public KinematicIeJe
    {
        //rIeJe rIeJeO uIeJeO muIeJeO 
    public:
        DistIecJec();
        DistIecJec(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void calcPrivate();
        double value() override;

        double rIeJe;
        FColDsptr rIeJeO, uIeJeO, muIeJeO;
    };
}

