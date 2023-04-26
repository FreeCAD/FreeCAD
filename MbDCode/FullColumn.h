#pragma once
#include "Vector.h"
namespace MbD {
    template <typename T>
    class FullColumn : public Vector<T>
    {
    public:
        FullColumn(std::initializer_list<T> list) : Vector<T>{ list } {}
    };
}

