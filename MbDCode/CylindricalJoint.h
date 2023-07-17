#pragma once

#include "InLineJoint.h"

namespace MbD {
    class CylindricalJoint : public InLineJoint
    {
        //frmI frmJ constraints friction 
    public:
        CylindricalJoint();
        CylindricalJoint(const char* str);
        void initializeGlobally() override;
    };
}

