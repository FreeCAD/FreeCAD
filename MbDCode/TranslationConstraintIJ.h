#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecKec.h"

namespace MbD {
    class TranslationConstraintIJ : public ConstraintIJ
    {
        //riIeJeIe
    public:
        TranslationConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeIe();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC()override;
        MbD::ConstraintType type() override;
        void postPosICIteration() override;

        size_t axisI;
        std::shared_ptr<DispCompIecJecKec> riIeJeIe;
    };
}

