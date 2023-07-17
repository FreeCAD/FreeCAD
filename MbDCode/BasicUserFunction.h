#pragma once

#include <string>

#include "UserFunction.h"

namespace MbD {
    class BasicUserFunction : public UserFunction
    {
        //funcText myUnit units 
    public:
        std::string funcText;
        double myUnit;
    };
}

