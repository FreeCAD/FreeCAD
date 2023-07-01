#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DirectionCosineIecJec;

    class DirectionCosineConstraintIJ : public ConstraintIJ
    {
        //axisI axisJ aAijIeJe 
    public:
        DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);
        void initialize() override;
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initaAijIeJe();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC() override;
        void postPosICIteration() override;
        ConstraintType type() override;
        void preVelIC() override;
        void preAccIC() override;

        int axisI, axisJ;
        std::shared_ptr<DirectionCosineIecJec> aAijIeJe;
    };
}

