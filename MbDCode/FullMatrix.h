#pragma once
#include "RowTypeMatrix.h"
#include "FullRow.h"

namespace MbD {
    template <typename T>
    class FullMatrix : public RowTypeMatrix<FullRow<T>*>
    {
    public:
        FullMatrix() {
            // constructor code here
        }
        FullMatrix(int m, int n);
        //FullMatrix(std::initializer_list<T> list) : RowTypeMatrix<FullRow<T>>{ list } {}
    };
}

