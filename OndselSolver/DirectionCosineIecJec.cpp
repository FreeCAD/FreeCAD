/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <memory>

#include "DirectionCosineIecJec.h"
#include "EndFramec.h"

namespace MbD {
    DirectionCosineIecJec::DirectionCosineIecJec()
    = default;

    DirectionCosineIecJec::DirectionCosineIecJec(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj) :
            KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
    {

    }

    void DirectionCosineIecJec::calcPostDynCorrectorIteration()
    {
        aAjOIe = frmI->aAjOe(axisI);
        aAjOJe = frmJ->aAjOe(axisJ);
        aAijIeJe = aAjOIe->dotVec(aAjOJe);
    }

    double MbD::DirectionCosineIecJec::value()
    {
        return aAijIeJe;
    }
}

