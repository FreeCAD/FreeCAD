#pragma once

#include "DispCompIeqcJeqcKeqct.h"

namespace MbD {
    class DispCompIeqctJeqcKeqct : public DispCompIeqcJeqcKeqct
    {
        //
    public:
        DispCompIeqctJeqcKeqct();
        DispCompIeqctJeqcKeqct(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        void preAccIC() override;
        void preVelIC() override;

    };
}

