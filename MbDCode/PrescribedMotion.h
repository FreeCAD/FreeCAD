#pragma once
#include <memory>

#include "Joint.h"
//#include "Symbolic.h"

namespace MbD {
    class Symbolic;

    class PrescribedMotion : public Joint
    {
        //xBlk yBlk zBlk phiBlk theBlk psiBlk 
    public:
        PrescribedMotion();
        PrescribedMotion(const char* str);

        void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;
        void initialize() override;
        virtual void initMotions();

        //Why the following fails?
        //Symsptr xBlk;
        //Symsptr yBlk;
        //Symsptr zBlk;
        //Symsptr phiBlk;
        //Symsptr theBlk;
        //Symsptr psiBlk;
        std::shared_ptr<Symbolic> xBlk;
        std::shared_ptr<Symbolic> yBlk;
        std::shared_ptr<Symbolic> zBlk;
        std::shared_ptr<Symbolic> phiBlk;
        std::shared_ptr<Symbolic> theBlk;
        std::shared_ptr<Symbolic> psiBlk;
    };
}

