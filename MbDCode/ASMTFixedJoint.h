#pragma once

#include "ASMTJoint.h"
#include "FixedJoint.h"

namespace MbD {
    class ASMTFixedJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;

    };
}
