#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DispCompIecJecO;

    class AtPointConstraintIJ : public ConstraintIJ
    {
        //axis riIeJeO 
    public:
        AtPointConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeO();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC() override;
        MbD::ConstraintType type() override;
        void postPosICIteration() override;
        void preVelIC() override;

        int axis;
        std::shared_ptr<DispCompIecJecO> riIeJeO;
    };
}

