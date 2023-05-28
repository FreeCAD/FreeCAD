#pragma once
#include "Joint.h"

namespace MbD {
    class CylindricalJoint : public Joint
    {
        //frmI frmJ constraints friction 
    public:
        static std::shared_ptr<CylindricalJoint> Create(const char* name);
        CylindricalJoint();
        CylindricalJoint(const char* str);
        void initializeGlobally();
    };
}

