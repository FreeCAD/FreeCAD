#pragma once

#include <memory>

#include "KinematicIeJe.h"

namespace MbD {
    template<typename T>
    class FullColumn;

    class DirectionCosineIecJec : public KinematicIeJe
    {
        //aAijIeJe axisI axisJ aAjOIe aAjOJe 
    public:
        DirectionCosineIecJec();
        DirectionCosineIecJec(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        double value() override;

        int axisI, axisJ;   //0, 1, 2 = x, y, z
        double aAijIeJe;
        std::shared_ptr<FullColumn<double>> aAjOIe, aAjOJe;
    };
}

