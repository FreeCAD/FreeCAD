#pragma once
#include "Joint.h"
#include "Symbolic.h"

namespace MbD {

    class PrescribedMotion : public Joint
    {
        //xBlk yBlk zBlk phiBlk theBlk psiBlk 
    public:
        PrescribedMotion();
        PrescribedMotion(const char* str);
        void initialize() override;
        void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;

        Symsptr xBlk;
        Symsptr yBlk;
        Symsptr zBlk;
        Symsptr phiBlk;
        Symsptr theBlk;
        Symsptr psiBlk;
    };
}

