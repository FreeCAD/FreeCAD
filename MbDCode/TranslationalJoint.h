#pragma once

#include "InLineJoint.h"

namespace MbD {
    class TranslationalJoint : public InLineJoint
    {
        //
    public:
        TranslationalJoint();
        TranslationalJoint(const char* str);
        void initializeGlobally() override;


    };
}

