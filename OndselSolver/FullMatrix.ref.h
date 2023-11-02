#pragma once

#include "FullColumn.ref.h"

namespace MbD {
    template<typename T>
    class FullMatrix;

    using FMatDsptr = std::shared_ptr<MbD::FullMatrix<double>>;

    template<typename T>
    using FMatsptr = std::shared_ptr<FullMatrix<T>>;

    using FMatFColDsptr = std::shared_ptr<FullMatrix<FColDsptr>>;
    using FMatFMatDsptr = std::shared_ptr<FullMatrix<FMatDsptr>>;

    using FColFMatDsptr = std::shared_ptr<FullColumn<FMatDsptr>>;
}


