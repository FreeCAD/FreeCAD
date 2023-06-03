#pragma once
#include "Constraint.h"
namespace MbD {
    class AbsConstraint : public Constraint
    {
        //axis iqXminusOnePlusAxis 
    public:
        AbsConstraint();
        AbsConstraint(const char* str);
        AbsConstraint(size_t axis);
        void initialize();
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;
        void fillPosICJacob(SpMatDsptr mat) override;

        size_t axis = -1;
        size_t iqXminusOnePlusAxis = -1;
    };
}

