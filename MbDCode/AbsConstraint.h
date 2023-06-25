#pragma once
#include "Constraint.h"
namespace MbD {
    class AbsConstraint : public Constraint
    {
        //axis iqXminusOnePlusAxis 
    public:
        //AbsConstraint();
        //AbsConstraint(const char* str);
        AbsConstraint(int axis);
        void initialize() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void fillAccICIterError(FColDsptr col) override;

        int axis = -1;
        int iqXminusOnePlusAxis = -1;
    };
}

