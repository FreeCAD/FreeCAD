#pragma once
#include "FullColumn.h"

namespace MbD {

    template <typename T>
    class EulerArray : public FullColumn<T>
    {
        //
    public:
        EulerArray(int count) : FullColumn<T>(count) {}
        EulerArray(int count, const T& value) : FullColumn<T>(count, value) {}
        EulerArray(std::initializer_list<T> list) : FullColumn<T>{ list } {}
        virtual void initialize();
    };
    template<typename T>
    inline void EulerArray<T>::initialize()
    {
    }
}

