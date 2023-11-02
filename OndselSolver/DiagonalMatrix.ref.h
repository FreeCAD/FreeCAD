#pragma once

#include <memory>

namespace MbD {
    template<typename T>
    class DiagonalMatrix;
    template<typename T>
    using DiagMatsptr = std::shared_ptr<DiagonalMatrix<T>>;
    using DiagMatDsptr = std::shared_ptr<DiagonalMatrix<double>>;
}