#pragma once

#include "ConstraintIJ.h"
#include "DistIecJec.h"

namespace MbD {
    class DistanceConstraintIJ : public ConstraintIJ
    {
        //distIeJe
    public:
        DistanceConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void init_distIeJe() = 0;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;
        ConstraintType type() override;

        std::shared_ptr<DistIecJec> distIeJe;

    };
}

