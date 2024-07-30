/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "AbsConstraint.h"
#include "PartFrame.h"

using namespace MbD;
//
//AbsConstraint::AbsConstraint() {}
//
//AbsConstraint::AbsConstraint(const std::string& str) : Constraint(str) {}

AbsConstraint::AbsConstraint(size_t i)
{
    axis = i;
}

void AbsConstraint::calcPostDynCorrectorIteration()
{
    if (axis < 3) {
        aG = static_cast<PartFrame*>(owner)->qX->at(axis);
    }
    else {
        aG = static_cast<PartFrame*>(owner)->qE->at(axis - 3);
    }
}

void AbsConstraint::useEquationNumbers()
{
    iqXminusOnePlusAxis = static_cast<PartFrame*>(owner)->iqX + axis;
}

void AbsConstraint::fillPosICJacob(SpMatDsptr mat)
{
    mat->atijplusNumber(iG, iqXminusOnePlusAxis, 1.0);
    mat->atijplusNumber(iqXminusOnePlusAxis, iG, 1.0);
}

void AbsConstraint::fillPosICError(FColDsptr col)
{
    Constraint::fillPosICError(col);
    col->at(iqXminusOnePlusAxis) += lam;
}

void AbsConstraint::fillPosKineJacob(SpMatDsptr mat)
{
    mat->atijplusNumber(iG, iqXminusOnePlusAxis, 1.0);
}

void AbsConstraint::fillVelICJacob(SpMatDsptr mat)
{
    this->fillPosICJacob(mat);
}

void AbsConstraint::fillAccICIterError(FColDsptr col)
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
