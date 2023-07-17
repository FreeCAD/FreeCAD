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

        void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;
        void initialize() override;
        virtual void initMotions();

        Symsptr xBlk;
        Symsptr yBlk;
        Symsptr zBlk;
        Symsptr phiBlk;
        Symsptr theBlk;
        Symsptr psiBlk;
    };
}

