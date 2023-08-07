#pragma once

#include "AtPointJoint.h"

namespace MbD {
    class ConstantVelocityJoint : public AtPointJoint
    {
        //
    public:
        ConstantVelocityJoint();
        ConstantVelocityJoint(const char* str);
        void initializeGlobally() override;
        void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ) override;


    };
}

