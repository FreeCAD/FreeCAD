#include "MomentOfInertiaSolver.h"
#include "EulerParameters.h"
#include <iostream>

using namespace MbD;

void MbD::MomentOfInertiaSolver::example1()
{
	auto aJpp = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 2, 0 },
		{ 0, 0, 3 }
		});

	auto rpPp = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 1 });
	auto axis = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 1 });
	auto aApP = std::make_shared<EulerParameters<double>>(axis, 0.1)->aA;
	auto solver = std::make_shared<MomentOfInertiaSolver>();
	solver->setm(4.0);
	solver->setJPP(aJpp);
	solver->setrPoP(rpPp);
	solver->setAPo(aApP);
	auto rPcmP = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	solver->setrPcmP(rPcmP);
	solver->calc();
	auto aJPP = solver->getJoo();
	std::cout << solver->getJoo();
	std::cout << solver->getJpp();
	std::cout << solver->getAPp();
	std::cout << std::endl;
	solver->setm(4.0);
	solver->setJPP(aJPP);
	auto rPoP = aApP->transposeTimesFullColumn(rpPp->negated());
	solver->setrPoP(rPoP);
	auto aAPo = aApP->transpose();
	solver->setAPo(aAPo);
	solver->setrPcmP(rPoP);
	solver->calc();
	std::cout << solver->getJoo();
	std::cout << solver->getJpp();
	std::cout << solver->getAPp();
	std::cout << std::endl;
}

void MbD::MomentOfInertiaSolver::forwardEliminateWithPivot(int p)
{
}

void MbD::MomentOfInertiaSolver::backSubstituteIntoDU()
{
}

void MbD::MomentOfInertiaSolver::postSolve()
{
}

FColDsptr MbD::MomentOfInertiaSolver::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	return FColDsptr();
}

FColDsptr MbD::MomentOfInertiaSolver::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	return FColDsptr();
}

void MbD::MomentOfInertiaSolver::preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
}

void MbD::MomentOfInertiaSolver::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
}

double MbD::MomentOfInertiaSolver::getmatrixArowimaxMagnitude(int i)
{
	return 0.0;
}

void MbD::MomentOfInertiaSolver::doPivoting(int p)
{
}

void MbD::MomentOfInertiaSolver::setm(double mass)
{
	m = mass;
}

void MbD::MomentOfInertiaSolver::setJPP(FMatDsptr mat)
{
	aJPP = mat;
}

void MbD::MomentOfInertiaSolver::setrPoP(FColDsptr col)
{
	rPoP = col;
}

void MbD::MomentOfInertiaSolver::setAPo(FMatDsptr mat)
{
	aAPo = mat;
}

void MbD::MomentOfInertiaSolver::setrPcmP(FColDsptr col)
{
	rPcmP = col;
}

FMatDsptr MbD::MomentOfInertiaSolver::getJoo()
{
	return aJoo;
}

FMatDsptr MbD::MomentOfInertiaSolver::getJpp()
{
	return aJpp;
}

FMatDsptr MbD::MomentOfInertiaSolver::getAPp()
{
	return aAPp;
}

void MbD::MomentOfInertiaSolver::calc()
{
	calcJoo();
	calcJpp();
	calcAPp();
}

void MbD::MomentOfInertiaSolver::calcJoo()
{
	//"aJoo = aAPoT*[aJPP + mass*(rPoPTilde*rPoPTilde + rPoPTilde*rocmPTilde + rocmPTilde*rPoPTilde)]*aAPo"

	if (!rPoP) {
		rPoP = rPcmP;
		aAPo = FullMatrix<double>::identitysptr(3);
	}
	auto rocmPtilde = FullMatrix<double>::tildeMatrix(rPcmP->minusFullColumn(rPoP));
	auto rPoPtilde = FullMatrix<double>::tildeMatrix(rPoP);
	auto term1 = aJPP;
	auto term21 = rPoPtilde->timesFullMatrix(rPoPtilde);
	auto term22 = rPoPtilde->timesFullMatrix(rocmPtilde);
	auto term23 = term22->transpose();
	auto term2 = term21->plusFullMatrix(term22)->plusFullMatrix(term23)->times(m);
	aJoo = aAPo->transposeTimesFullMatrix(term1->plusFullMatrix(term2))->timesFullMatrix(aAPo);
	aJoo->symLowerWithUpper();
	aJoo->conditionSelfWithTol(aJoo->maxMagnitude() * 1.0e-6);
}

void MbD::MomentOfInertiaSolver::calcJpp()
{
}

void MbD::MomentOfInertiaSolver::calcAPp()
{
}
