#pragma once
#include "Array.h"
namespace MbD {
    template <typename T>
    class DiagonalMatrix : public Array<T>
    {
        //
    public:
        DiagonalMatrix(size_t count) : Array<T>(count) {}
        DiagonalMatrix(std::initializer_list<T> list) : Array<T>{ list } {}
    };
}

