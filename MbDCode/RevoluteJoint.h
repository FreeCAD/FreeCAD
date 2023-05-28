#pragma once
#include "Joint.h"
namespace MbD {
    class RevoluteJoint : public Joint
    {
        //
    public:
        RevoluteJoint();
        RevoluteJoint(const char* str);
        void initializeGlobally() override;
    };
}

