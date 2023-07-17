#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecIe.h"

namespace MbD {
    class DistancexyConstraintIJ : public ConstraintIJ
    {
        //xIeJeIe yIeJeIe 
    public:
        DistancexyConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void init_xyIeJeIe();
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



        std::shared_ptr<DispCompIecJecIe> xIeJeIe, yIeJeIe;
        //ToDo: Use DistxyIecJec instead of xIeJeIe, yIeJeIe

    };
}

