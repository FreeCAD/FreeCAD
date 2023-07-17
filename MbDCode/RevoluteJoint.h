#pragma once

#include "AtPointJoint.h"

namespace MbD {
    class RevoluteJoint : public AtPointJoint
    {
        //
    public:
        RevoluteJoint();
        RevoluteJoint(const char* str);
        void initializeGlobally() override;
    };
}

