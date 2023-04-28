#include "FullMatrix.h"

using namespace MbD;

FullMatrix<double>::FullMatrix(int m, int n) {
    for (int i = 0; i < m; i++) {
        auto* row = new FullRow<double>(n);
        this->push_back(row);
    }
}

