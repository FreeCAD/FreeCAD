#pragma once

#include "EulerArray.h"
#include "EulerParametersDot.h"

namespace MbD {

    template<typename T>
    class EulerParametersDDot : public EulerArray<T>
    {
        //qEdot aAddot aBddot aCddot 
    public:
        EulerParametersDDot(int count) : EulerArray<T>(count) {}
        EulerParametersDDot(int count, const T& value) : EulerArray<T>(count, value) {}
        EulerParametersDDot(std::initializer_list<T> list) : EulerArray<T>{ list } {}

        //std::shared_ptr<EulerParametersDot<T>> qEdot;
        FMatDsptr aAddot, aBddot, aCddot;
    };
}

