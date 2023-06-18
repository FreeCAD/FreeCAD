#pragma once

#include "EulerArray.h"
#include "EulerAngleszxzDot.h"

namespace MbD {

    template<typename T>
    class EulerAngleszxzDDot : public EulerArray<T>
    {
        //phiThePsiDot phiAddot theAddot psiAddot aAddot 
    public:

        std::shared_ptr<EulerAngleszxzDot<double>> phiThePsiDot;
        FMatDsptr phiAddot, theAddot, psiAddot, aAddot;
    };
}

