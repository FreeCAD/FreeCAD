#pragma once

#include "ASMTItemIJ.h"

namespace MbD {
    class Joint;

    class ASMTConstraintSet : public ASMTItemIJ
    {
        //
    public:
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        virtual std::shared_ptr<Joint> mbdClassNew();
        void updateFromMbD() override;
        void compareResults(AnalysisType type) override;

    };
}

