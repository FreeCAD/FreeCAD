#include <sstream>

#include "FullColumn.h"

using namespace MbD;

std::string FullColumn<double>::toString() {
    std::stringstream ss;

    ss << "FullColumn { ";
    for (int i = 0; i < this->size() - 1; i++) {
        ss << this->at(i) << ", ";
    }
    ss << this->back() << " }";
    return ss.str();
}
