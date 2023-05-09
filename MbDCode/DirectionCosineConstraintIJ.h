#pragma once

#include "ConstraintIJ.h"
#include "DirectionCosineIecJec.h"

namespace MbD {
    class DirectionCosineConstraintIJ : public ConstraintIJ
    {
        //axisI axisJ aAijIeJe 
    public:
        //	self owns : (MbDDirectionCosineConstraintIJ withFrmI : frmI frmJ : frmJ axisI : 2 axisJ : 1).
        DirectionCosineConstraintIJ(std::shared_ptr<EndFramec> frmI, std::shared_ptr<EndFramec> frmJ, int axisI, int axisJ);

        int axisI, axisJ;
        std::shared_ptr<DirectionCosineIecJec> aAijIeJe;
    };
}

