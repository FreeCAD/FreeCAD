#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecKec.h"

namespace MbD {
    class TranslationConstraintIJ : public ConstraintIJ
    {
        //riIeJeIe
    public:
        TranslationConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initialize() override;
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeIe();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC()override;
        ConstraintType type() override;
        void postPosICIteration() override;
        void preVelIC() override;
        void preAccIC() override;

        int axisI;
        std::shared_ptr<DispCompIecJecKec> riIeJeIe;
    };
}

