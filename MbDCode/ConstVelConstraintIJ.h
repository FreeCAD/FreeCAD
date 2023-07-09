#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DirectionCosineIecJec;

    class ConstVelConstraintIJ : public ConstraintIJ
    {
        //aA01IeJe aA10IeJe 
    public:
        ConstVelConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void initA01IeJe() = 0;
        virtual void initA10IeJe() = 0;
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

        std::shared_ptr<DirectionCosineIecJec> aA01IeJe, aA10IeJe;
    };
}

