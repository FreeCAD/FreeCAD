#pragma once
#include "Joint.h"

namespace MbD {
    class Symbolic;

    class PrescribedMotion : public Joint
    {
        //xBlk yBlk zBlk phiBlk theBlk psiBlk 
    public:
        PrescribedMotion();
        PrescribedMotion(const char* str);
        void initialize();
        void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;

        std::shared_ptr<Symbolic> xBlk;
        std::shared_ptr<Symbolic> yBlk;
        std::shared_ptr<Symbolic> zBlk;
        std::shared_ptr<Symbolic> phiBlk;
        std::shared_ptr<Symbolic> theBlk;
        std::shared_ptr<Symbolic> psiBlk;
    };
}

