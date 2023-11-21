/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>

#include "FullColumn.h"
#include "KinematicIeJe.h"

namespace MbD {

    class DirectionCosineIecJec : public KinematicIeJe
    {
        //aAijIeJe axisI axisJ aAjOIe aAjOJe 
    public:
        DirectionCosineIecJec();
        DirectionCosineIecJec(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        double value() override;

        int axisI{}, axisJ{};   //0, 1, 2 = x, y, z
        double aAijIeJe{};
        FColDsptr aAjOIe, aAjOJe;
    };
}

