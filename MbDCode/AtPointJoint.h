#pragma once

#include "Joint.h"

namespace MbD {
    class AtPointJoint : public Joint
    {
        //
    public:
        AtPointJoint();
        AtPointJoint(const char* str);

        void createAtPointConstraints();


    };
}

