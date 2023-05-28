#pragma once
#include "Joint.h"
namespace MbD {
    class RevoluteJoint : public Joint
    {
        //
    public:
        static std::shared_ptr<RevoluteJoint> Create(const char* name);
        RevoluteJoint();
        RevoluteJoint(const char* str);
    };
}

