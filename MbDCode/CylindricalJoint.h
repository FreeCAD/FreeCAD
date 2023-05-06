#pragma once
#include "Joint.h"

namespace MbD {
    class CylindricalJoint : public Joint
    {
        //frmI frmJ constraints friction 
    public:
        CylindricalJoint();
        CylindricalJoint(const char* str);
        void initialize();
    };
}

