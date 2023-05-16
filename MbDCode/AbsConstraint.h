#pragma once
#include "Constraint.h"
namespace MbD {
    class AbsConstraint : public Constraint
    {
        //axis iqXminusOnePlusAxis 
    public:
        AbsConstraint();
        AbsConstraint(const char* str);
        AbsConstraint(int axis);
        void initialize();
        int axis;
        int iqXminusOnePlusAxis;
    };
}

