#pragma once

#include "ASMTJoint.h"
#include "CylindricalJoint.h"

namespace MbD {
    class ASMTCylindricalJoint : public ASMTJoint
    {
        //
    public:
        std::shared_ptr<Joint> mbdClassNew() override;


    };
}

