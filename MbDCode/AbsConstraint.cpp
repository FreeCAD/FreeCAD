#include "AbsConstraint.h"
#include "PartFrame.h"

using namespace MbD;
//
//AbsConstraint::AbsConstraint() {}
//
//AbsConstraint::AbsConstraint(const char* str) : Constraint(str) {}

AbsConstraint::AbsConstraint(int i)
{
    axis = i;
}

void AbsConstraint::initialize()
{
    Constraint::initialize();
}

void MbD::AbsConstraint::calcPostDynCorrectorIteration()
{
    if (axis < 3) {
        aG = static_cast<PartFrame*>(owner)->qX->at(axis);
    }
    else {
        aG = static_cast<PartFrame*>(owner)->qE->at(axis - 3);
    }
}

void MbD::AbsConstraint::useEquationNumbers()
{
    iqXminusOnePlusAxis = static_cast<PartFrame*>(owner)->iqX + axis;
}

void MbD::AbsConstraint::fillPosICJacob(SpMatDsptr mat)
{
    mat->atijplusNumber(iG, iqXminusOnePlusAxis, 1.0);
    mat->atijplusNumber(iqXminusOnePlusAxis, iG, 1.0);
}

void MbD::AbsConstraint::fillPosICError(FColDsptr col)
{
    Constraint::fillPosICError(col);
    col->at(iqXminusOnePlusAxis) += lam;
}

void MbD::AbsConstraint::fillPosKineJacob(SpMatDsptr mat)
{
    mat->atijplusNumber(iG, iqXminusOnePlusAxis, 1.0);
}

void MbD::AbsConstraint::fillVelICJacob(SpMatDsptr mat)
{
    this->fillPosICJacob(mat);
}

void MbD::AbsConstraint::fillAccICIterError(FColDsptr col)
{
    col->atiplusNumber(iqXminusOnePlusAxis, lam);
    auto partFrame = static_cast<PartFrame*>(owner);
        double sum;
        if (axis < 3) {
            sum = partFrame->qXddot->at(axis);
        }
        else {
            sum = partFrame->qEddot->at(axis - 3);
        }
        col->atiplusNumber(iG, sum);
}
