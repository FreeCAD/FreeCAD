#pragma once

#include "KinematicIeJe.h"
#include "DirectionCosineIeqcJec.h"

namespace MbD {
    class AngleZIecJec : public KinematicIeJe
    {
        //thez aA00IeJe aA10IeJe cosOverSSq sinOverSSq twoCosSinOverSSqSq dSqOverSSqSq 
    public:
        void calcPostDynCorrectorIteration() override;
        virtual void init_aAijIeJe() = 0;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;
        double value() override;

        double thez, cosOverSSq, sinOverSSq, twoCosSinOverSSqSq, dSqOverSSqSq;
        std::shared_ptr<DirectionCosineIeqcJec> aA00IeJe, aA10IeJe;
    };
}

