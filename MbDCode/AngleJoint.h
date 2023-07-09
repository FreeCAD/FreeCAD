#pragma once

#include "Joint.h"

namespace MbD {
    class AngleJoint : public Joint
    {
        //theIzJz
    public:
        AngleJoint();
        AngleJoint(const char* str);
        void initializeGlobally() override;

        double theIzJz = 0.0;
    };
}

