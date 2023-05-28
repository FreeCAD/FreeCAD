#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DirectionCosineIecJec;

    class DirectionCosineConstraintIJ : public ConstraintIJ
    {
        //axisI axisJ aAijIeJe 
    public:
        DirectionCosineConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initaAijIeJe();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC() override;

        int axisI, axisJ;
        std::shared_ptr<DirectionCosineIecJec> aAijIeJe;
    };
}

