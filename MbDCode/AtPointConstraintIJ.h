#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DispCompIecJecO;

    class AtPointConstraintIJ : public ConstraintIJ
    {
        //axis riIeJeO 
    public:
        AtPointConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj, int axisi);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        virtual void initriIeJeO();
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        ConstraintType type() override;
        

        int axis;
        std::shared_ptr<DispCompIecJecO> riIeJeO;
    };
}

