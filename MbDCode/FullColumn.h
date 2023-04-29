#pragma once
#include "Vector.h"

namespace MbD {
    template <typename T>
    class FullColumn : public Vector<T>
    {
    public:
        FullColumn(int i) : Vector<T>(i) {}
        FullColumn(std::initializer_list<T> list) : Vector<T>{ list } {}
        std::string toString();
    };
}

typedef std::shared_ptr<MbD::FullColumn<double>> FullColDptr;
