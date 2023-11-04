#pragma once

#include "FullColumn.ref.h"

namespace MbD {
    class FullMatrixDouble;
    class FullMatrixFullMatrixDouble;
    class FullMatrixFullColumnDouble;

    using FMatDsptr = std::shared_ptr<FullMatrixDouble>;

    using FMatFMatDsptr = std::shared_ptr<FullMatrixFullMatrixDouble>;

    using FColFMatDsptr = std::shared_ptr<FullColumn<FMatDsptr>>;
    using FMatFColDsptr = std::shared_ptr<FullMatrixFullColumnDouble>;
}


