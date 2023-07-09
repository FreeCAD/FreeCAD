#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DirectionCosineIecJec;

    class DirectionCosineConstraintIJ : public ConstraintIJ
    {
        //axisI axisJ aAijIeJe 
    public:
        DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        virtual void initaAijIeJe();
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

        int axisI, axisJ;
        std::shared_ptr<DirectionCosineIecJec> aAijIeJe;
    };
}

