#pragma once
#include "Joint.h"

namespace MbD {
    class PrescribedMotion : public Joint
    {
        //xBlk yBlk zBlk phiBlk theBlk psiBlk 
    public:
        PrescribedMotion();
        PrescribedMotion(const char* str);
        void initialize();
        void connectsItoJ(std::shared_ptr<EndFramec> frmI, std::shared_ptr<EndFramec> frmJ) override;

        std::shared_ptr<Variable> xBlk;
        std::shared_ptr<Variable> yBlk;
        std::shared_ptr<Variable> zBlk;
        std::shared_ptr<Variable> phiBlk;
        std::shared_ptr<Variable> theBlk;
        std::shared_ptr<Variable> psiBlk;
    };
}

