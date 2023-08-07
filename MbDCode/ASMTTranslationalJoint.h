#pragma once

#include "ASMTJoint.h"
#include "TranslationalJoint.h"

namespace MbD {
    class ASMTTranslationalJoint : public ASMTJoint
    {
        //
    public:
        std::shared_ptr<Joint> mbdClassNew() override;


    };
}

