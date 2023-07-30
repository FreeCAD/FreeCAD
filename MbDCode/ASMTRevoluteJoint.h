#pragma once

#include "ASMTJoint.h"
#include "RevoluteJoint.h"

namespace MbD {
    class ASMTRevoluteJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;

    };
}

