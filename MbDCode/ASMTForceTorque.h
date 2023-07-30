#pragma once

#include "ASMTItemIJ.h"

namespace MbD {
    class ASMTForceTorque : public ASMTItemIJ
    {
        //
    public:
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;


    };
}

