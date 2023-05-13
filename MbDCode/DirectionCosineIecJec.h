#pragma once
#include <memory>

#include "KinematicIeJe.h"
//#include "EndFramec.h"
#include "FullColumn.h"

namespace MbD {
    class DirectionCosineIecJec : public KinematicIeJe
    {
        //aAijIeJe axisI axisJ aAjOIe aAjOJe 
    public:
        DirectionCosineIecJec();
        DirectionCosineIecJec(EndFrmcptr frmI, EndFrmcptr frmJ, int axisI, int axisJ);

        int axisI, axisJ;
        double aAijIeJe;
        std::shared_ptr<FullColumn<double>> aAjOIe, aAjOJe;
    };
}

