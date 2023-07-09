#pragma once

#include "DistanceConstraintIqcJqc.h"

namespace MbD {
    class DistanceConstraintIqctJqc : public DistanceConstraintIqcJqc
    {
        //pGpt ppGpXIpt ppGpEIpt ppGpXJpt ppGpEJpt ppGptpt 
    public:
        DistanceConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj);

        double pGpt, ppGptpt;
        FRowDsptr ppGpXIpt, ppGpEIpt, ppGpXJpt, ppGpEJpt;
            
    };
}

