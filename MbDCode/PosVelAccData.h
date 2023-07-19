#pragma once

#include "StateData.h"

namespace MbD {
    class PosVelAccData : public StateData
    {
        //refData rFfF aAFf vFfF omeFfF aFfF alpFfF 
    public:
        std::ostream& printOn(std::ostream& s) const override;

        std::shared_ptr<PosVelAccData> refData;
        FColDsptr rFfF, vFfF, omeFfF, aFfF, alpFfF;
        FMatDsptr aAFf;
    };
}

