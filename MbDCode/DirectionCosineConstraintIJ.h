#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DirectionCosineIecJec;

    class DirectionCosineConstraintIJ : public ConstraintIJ
    {
        //axisI axisJ aAijIeJe 
    public:
        DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initaAijIeJe();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC() override;
        void postPosICIteration() override;
        MbD::ConstraintType type() override;

        size_t axisI, axisJ;
        std::shared_ptr<DirectionCosineIecJec> aAijIeJe;
    };
}

