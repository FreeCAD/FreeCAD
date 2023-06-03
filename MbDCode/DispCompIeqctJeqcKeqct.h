#pragma once

#include "DispCompIeqcJeqcKeqct.h"

namespace MbD {
    class DispCompIeqctJeqcKeqct : public DispCompIeqcJeqcKeqct
    {
        //
    public:
        DispCompIeqctJeqcKeqct();
        DispCompIeqctJeqcKeqct(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, size_t axisk);

    };
}

