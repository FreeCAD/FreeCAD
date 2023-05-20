#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecKec.h"

namespace MbD {
    class TranslationConstraintIJ : public ConstraintIJ
    {
        //riIeJeIe
    public:
        static std::shared_ptr<TranslationConstraintIJ> Create(EndFrmcptr frmi, EndFrmcptr frmj, int axisk);
        TranslationConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisk);
        void initialize();
        void initializeLocally() override;
        void initializeGlobally() override;
        virtual void initriIeJeIe();
        void postInput() override;

        int axisK;
        std::shared_ptr<DispCompIecJecKec> riIeJeIe;
    };
}

