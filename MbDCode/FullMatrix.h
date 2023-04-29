#pragma once
#include "RowTypeMatrix.h"
#include "FullRow.h"

namespace MbD {
    template <typename T>
    class FullMatrix : public RowTypeMatrix<std::shared_ptr<FullRow<T>>>
    {
    public:
        FullMatrix() {
            // constructor code here
        }
        FullMatrix(int m, int n);
        //FullMatrix(std::initializer_list<T> list) : RowTypeMatrix<FullRow<T>>{ list } {}
    };
}

typedef std::shared_ptr<MbD::FullMatrix<double>> FullMatDptr;
