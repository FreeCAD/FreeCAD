#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DispCompIecJecO;

    class AtPointConstraintIJ : public ConstraintIJ
    {
        //axis riIeJeO 
    public:
        static std::shared_ptr<AtPointConstraintIJ> Create(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        AtPointConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeO();
        void postInput() override;
        void calcPostDynCorrectorIteration() override;

        int axis;
        std::shared_ptr<DispCompIecJecO> riIeJeO;
    };
}

