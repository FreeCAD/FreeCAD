#pragma once

#include "ASMTJoint.h"
#include "SphericalJoint.h"

namespace MbD {
    class ASMTSphericalJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;

    };
}

