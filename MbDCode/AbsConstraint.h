#pragma once
#include "Constraint.h"
namespace MbD {
    class AbsConstraint : public Constraint
    {
        //axis iqXminusOnePlusAxis 
    public:
        static std::shared_ptr<AbsConstraint> Create(const char* name);
        AbsConstraint();
        AbsConstraint(const char* str);
        AbsConstraint(int axis);
        void initialize();
        void calcPostDynCorrectorIteration() override;

        int axis;
        int iqXminusOnePlusAxis;
    };
}

