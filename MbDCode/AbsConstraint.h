#pragma once
#include "Constraint.h"
namespace MbD {
    class AbsConstraint : public Constraint
    {
        //axis iqXminusOnePlusAxis 
    public:
        AbsConstraint();
        AbsConstraint(const char* str);
        void initialize();
        int axis;
        int iqXminusOnePlusAxis;
    };
}

