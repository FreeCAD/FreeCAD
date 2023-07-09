#pragma once

#include "KinematicIeJe.h"
#include "DispCompIecJecIe.h"

namespace MbD {
    class OrbitAnglezIecJec : public KinematicIeJe
    {
        //thez xIeJeIe yIeJeIe cosOverSSq sinOverSSq twoCosSinOverSSqSq dSqOverSSqSq 
    public:
        void calcPostDynCorrectorIteration() override;
        virtual void init_xyIeJeIe() = 0;
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
        std::shared_ptr<DispCompIecJecIe> xIeJeIe, yIeJeIe;
    };
}

