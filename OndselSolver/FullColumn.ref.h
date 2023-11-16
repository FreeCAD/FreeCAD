#pragma once

#include <algorithm>
#include <memory>

namespace MbD {
    template<typename T>
    class FullColumn;

    using FColDsptr = std::shared_ptr<FullColumn<double>>;

    template<typename T>
    using FColsptr = std::shared_ptr<FullColumn<T>>;
}
