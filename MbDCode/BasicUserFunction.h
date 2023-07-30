#pragma once

#include <string>

#include "UserFunction.h"

namespace MbD {
    class Units;

    class BasicUserFunction : public UserFunction
    {
        //funcText myUnit units 
    public:
        BasicUserFunction(const std::string& expression, double myUnt);
        void initialize();
            
        std::string funcText;
        double myUnit;
        std::shared_ptr<Units> units;
    };
}

