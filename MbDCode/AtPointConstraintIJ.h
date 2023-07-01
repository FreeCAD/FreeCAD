#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DispCompIecJecO;

    class AtPointConstraintIJ : public ConstraintIJ
    {
        //axis riIeJeO 
    public:
        AtPointConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initialize() override;
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeO();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;
        void prePosIC() override;
        ConstraintType type() override;
        void postPosICIteration() override;
        void preVelIC() override;
        void preAccIC() override;
        

        int axis;
        std::shared_ptr<DispCompIecJecO> riIeJeO;
    };
}

