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
        void initialize();
        void calcPostDynCorrectorIteration() override;

        int axisI, axisJ;
        double aAijIeJe;
        std::shared_ptr<FullColumn<double>> aAjOIe, aAjOJe;
    };
}

