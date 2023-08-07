#pragma once

#include "ASMTJoint.h"

namespace MbD {
    class ASMTUniversalJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;

    };
}

