#pragma once

#include "EulerArray.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EulerParameters.h"

namespace MbD {

    template <typename T>
    class EulerParametersDot : public EulerArray<T>
    {
        //qE aAdot aBdot aCdot pAdotpE
    public:
        EulerParametersDot(int count) : EulerArray<T>(count) {}
        EulerParametersDot(int count, const T& value) : EulerArray<T>(count, value) {}
        EulerParametersDot(std::initializer_list<T> list) : EulerArray<T>{ list } {}
        static std::shared_ptr<EulerParametersDot<T>> FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qe, FColDsptr omeOpO);
        std::shared_ptr<EulerParameters<T>> qE;
        FMatDsptr aAdot, aBdot, aCdot;
        FColFMatDsptr pAdotpE;
    };

    template<typename T>
    inline std::shared_ptr<EulerParametersDot<T>> EulerParametersDot<T>::FromqEOpAndOmegaOpO(std::shared_ptr<EulerParameters<T>> qe, FColDsptr omeOpO)
    {
        return std::shared_ptr<EulerParametersDot<T>>();
    }

}

