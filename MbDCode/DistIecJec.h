#pragma once

#include "KinematicIeJe.h"

namespace MbD {
    class DistIecJec : public KinematicIeJe
    {
        //rIeJe rIeJeO uIeJeO muIeJeO 
    public:
        DistIecJec();
        DistIecJec(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void calcPrivate();
        double value() override;

        double rIeJe;
        FColDsptr rIeJeO, uIeJeO, muIeJeO;
    };
}

