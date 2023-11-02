#pragma once

#include "FullColumn.ref.h"

namespace MbD {
    template<typename T>
    class FullRow;

    template<typename T>
    using FRowsptr = std::shared_ptr<FullRow<T>>;
    using FRowDsptr = std::shared_ptr<FullRow<double>>;

    using ListFRD = std::initializer_list<FRowDsptr>;
}