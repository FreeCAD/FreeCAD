#pragma once
#include "Array.h"

namespace MbD {
    template <typename T>
    class Vector : public Array<T>
    {
    public:
        Vector() {}
        Vector(size_t count) : Array<T>(count) {}
        Vector(size_t count, const T& value) : Array<T>(count, value) {}
        Vector(std::initializer_list<T> list) : Array<T>{ list } {}
    };
}

typedef std::initializer_list<double> ListD;
