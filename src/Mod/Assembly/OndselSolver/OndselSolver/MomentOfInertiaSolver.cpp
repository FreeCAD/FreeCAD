#include "MomentOfInertiaSolver.h"
#include "EulerParameters.h"
#include <iostream>
#include <algorithm>

using namespace MbD;

void MbD::MomentOfInertiaSolver::example1()
{
	auto aJpp = std::make_shared<FullMatrix<double>>(ListListD{
		{ 1, 0, 0 },
		{ 0, 2, 0 },
		{ 0, 0, 3 }
		});

	auto rpPp = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 1 });
	auto rotAxis = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 1 });
	auto aApP = std::make_shared<EulerParameters<double>>(rotAxis, M_PI*10/180)->aA;
	auto solver = std::make_shared<MomentOfInertiaSolver>();
	solver->setm(4.0);
	solver->setJPP(aJpp);
	solver->setrPoP(rpPp);
	solver->setAPo(aApP);
	auto rPcmP = std::make_shared<FullColumn<double>>(ListD{ 0, 0, 0 });
	solver->setrPcmP(rPcmP);
	solver->calc();
	auto aJPP = solver->getJoo();
	std::cout << *solver->getJoo() << std::endl;
	std::cout << *solver->getJpp() << std::endl;
	std::cout << *solver->getAPp() << std::endl;
	solver->setm(4.0);
	solver->setJPP(aJPP);
	auto rPoP = aApP->transposeTimesFullColumn(rpPp->negated());
	solver->setrPoP(rPoP);
	auto aAPo = aApP->transpose();
	solver->setAPo(aAPo);
	solver->setrPcmP(rPoP);
	solver->calc();
	std::cout << *solver->getJoo() << std::endl;
	std::cout << *solver->getJpp() << std::endl;
	std::cout << *solver->getAPp() << std::endl;
}

void MbD::MomentOfInertiaSolver::doFullPivoting(size_t p)
{
	double max = 0.0;
	auto pivotRow = p;
	auto pivotCol = p;
	for (size_t i = p; i < 3; i++)
	{
		auto rowi = aJcmPcopy->at(i);
		for (size_t j = p; j < 3; j++)
		{
			auto aij = rowi->at(j);
			if (aij != 0.0) {
				auto mag = std::abs(aij);
				if (mag > max) {
					max = mag;
					pivotRow = i;
					pivotCol = j;
				}
			}
		}

	}
	if (p != pivotRow) aJcmPcopy->swapElems(p, pivotRow);
	if (p != pivotCol) {
		for (auto& row : *aJcmPcopy) {
			row->swapElems(p, pivotCol);
		}
		colOrder->swapElems(p, pivotCol);
	}
}

void MbD::MomentOfInertiaSolver::forwardEliminateWithPivot(size_t p)
{
	auto rowp = aJcmPcopy->at(p);
	auto app = rowp->at(p);
	for (size_t i = p + 1; i < 3; i++)
	{
		auto rowi = aJcmPcopy->at(i);
		auto aip = rowi->at(p);
		if (aip != 0) {
			rowi->atiput(p, 0.0);
			auto factor = aip / app;
			for (size_t j = p + 1; j < 3; j++)
			{
				rowi->atiminusNumber(j, factor * rowp->at(j));
			}
		}
	}
}

void MbD::MomentOfInertiaSolver::backSubstituteIntoDU()
{
}

void MbD::MomentOfInertiaSolver::postSolve()
{
}

FColDsptr MbD::MomentOfInertiaSolver::basicSolvewithsaveOriginal(FMatDsptr, FColDsptr, bool)
{
	return FColDsptr();
}

FColDsptr MbD::MomentOfInertiaSolver::basicSolvewithsaveOriginal(SpMatDsptr, FColDsptr, bool)
{
	return FColDsptr();
}

void MbD::MomentOfInertiaSolver::preSolvewithsaveOriginal(FMatDsptr, FColDsptr, bool)
{
}

void MbD::MomentOfInertiaSolver::preSolvewithsaveOriginal(SpMatDsptr, FColDsptr, bool)
{
}

double MbD::MomentOfInertiaSolver::getmatrixArowimaxMagnitude(size_t)
{
	return 0.0;
}

void MbD::MomentOfInertiaSolver::doPivoting(size_t)
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

DiagMatDsptr MbD::MomentOfInertiaSolver::getJpp()
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
	//"aJcmP = aJPP + mass*(rPcmPTilde*rPcmPTilde)"

	auto rPcmPtilde = FullMatrix<double>::tildeMatrix(rPcmP);
	aJcmP = aJPP->plusFullMatrix(rPcmPtilde->timesFullMatrix(rPcmPtilde)->times(m));
	aJcmP->symLowerWithUpper();
	aJcmP->conditionSelfWithTol(aJcmP->maxMagnitude() * 1.0e-6);
	if (aJcmP->isDiagonal()) {
		calcJppFromDiagJcmP();
	}
	else {
		calcJppFromFullJcmP();
	}
}

void MbD::MomentOfInertiaSolver::calcAPp()
{
	FColDsptr eigenvector1, eigenvector2, eigenvector0;
	auto lam0 = aJpp->at(0);
	auto lam1 = aJpp->at(1);
	auto lam2 = aJpp->at(2);
	if (lam0 == lam1) {
		if (lam1 == lam2) {
			aAPp = FullMatrix<double>::identitysptr(3);
		}
		else {
			eigenvector1 = eigenvectorFor(lam1);
			eigenvector2 = eigenvectorFor(lam2);
			eigenvector1->normalizeSelf();
			eigenvector2->normalizeSelf();
			if (eigenvector1->at(1) < 0.0) eigenvector1->negateSelf();
			if (eigenvector2->at(2) < 0.0) eigenvector2->negateSelf();
			eigenvector0 = eigenvector1->cross(eigenvector2);
		}
	}
	else {
		eigenvector0 = eigenvectorFor(lam0);
		eigenvector1 = eigenvectorFor(lam1);
		eigenvector0->normalizeSelf();
		eigenvector1->normalizeSelf();
		if (eigenvector0->at(0) < 0.0) eigenvector0->negateSelf();
		if (eigenvector1->at(1) < 0.0) eigenvector1->negateSelf();
		eigenvector2 = eigenvector0->cross(eigenvector1);
	}
	aAPp = std::make_shared<FullMatrix<double>>(3, 3);
	aAPp->atijputFullColumn(0, 0, eigenvector0);
	aAPp->atijputFullColumn(0, 1, eigenvector1);
	aAPp->atijputFullColumn(0, 2, eigenvector2);
}

FColDsptr MbD::MomentOfInertiaSolver::eigenvectorFor(double lam)
{
	//"[aJcmP] - lam[I]."

	double e0, e1, e2;
	aJcmPcopy = aJcmP->copy();
	colOrder = std::make_shared<FullRow<size_t>>(3);
	auto eigenvector = std::make_shared<FullColumn<double>>(3);
	for (size_t i = 0; i < 3; i++)
	{
		colOrder->atiput(i, i);
		aJcmPcopy->atijminusNumber(i, i, lam);
	}
	for (size_t p = 0; p < 3; p++)
	{
		doFullPivoting(p);
		forwardEliminateWithPivot(p);
	}

	auto row0 = aJcmPcopy->at(0);
	auto row1 = aJcmPcopy->at(1);
	auto row2 = aJcmPcopy->at(2);
	auto norm0 = row0->length();
	//auto aaaa = row2->length();
	if ((row2->length() / norm0) > 1.0e-5) throw std::runtime_error("3rd row should be very small.");
	if ((row1->length() / norm0) > 1.0e-5) {
		e2 = 1.0;
		e1 = e2 * -row1->at(2) / row1->at(1);
		e0 = -(e1 * row0->at(1) + e2 * row0->at(2)) / row0->at(0);
	}
	else {
		//"Repeated roots."
		e2 = 1.0;
		e1 = 0.0;
		e0 = -(e1 * row0->at(1) + e2 * row0->at(2)) / row0->at(0);
	}


	auto dum = std::make_shared<FullColumn<double>>(ListD{ e0, e1, e2 });
	for (size_t i = 0; i < 3; i++)
	{
		eigenvector->atiput(colOrder->at(i), dum->at(i));
	}
	return eigenvector;
}

void MbD::MomentOfInertiaSolver::calcJppFromDiagJcmP()
{
	//"Eigenvalues are orders from smallest to largest."

	double average;
	auto sortedJ = std::make_shared<DiagonalMatrix<double>>();
	sortedJ->push_back(aJcmP->at(0)->at(0));
	sortedJ->push_back(aJcmP->at(1)->at(1));
	sortedJ->push_back(aJcmP->at(2)->at(2));
	std::sort(sortedJ->begin(), sortedJ->end(), [](double a, double b) { return a < b; });
	auto lam0 = sortedJ->at(0);
	auto lam1 = sortedJ->at(1);
	auto lam2 = sortedJ->at(2);
	if (lam1 == 0 || std::abs((lam0 / lam1) - 1.0) < 1.0e-6) {
		if (lam2 == 0 || std::abs((lam1 / lam2) - 1.0) < 1.0e-6) {
			//"All are equal."
			average = (lam0 + lam1 + lam2) / 3.0;
			lam0 = average;
			lam1 = average;
			lam2 = average;
		}
		else {
			//"lam0 = lam1"
			average = (lam0 + lam1) / 2.0;
			lam0 = average;
			lam1 = average;
		}
	}
	else {
		if (std::abs(lam1 / lam2 - 1.0) < 1.0e-6) {
			//"lam1 = lam2"
			average = (lam1 + lam2) / 2.0;
			lam1 = average;
			lam2 = average;
		}
	}
	aJpp = std::make_shared<DiagonalMatrix<double>>(ListD{ lam0, lam1, lam2 });
}

void MbD::MomentOfInertiaSolver::calcJppFromFullJcmP()
{
	//"Eigenvalues are orders from smallest to largest."
	//"Rounding error can be significant."
	//"e.g. (diag 100.0d 100.0d 1.0d) gives only 8 digit accuracy."

	double average;
	auto a00 = aJcmP->at(0)->at(0);
	auto a01 = aJcmP->at(0)->at(1);
	auto a02 = aJcmP->at(0)->at(2);
	auto a11 = aJcmP->at(1)->at(1);
	auto a12 = aJcmP->at(1)->at(2);
	auto a22 = aJcmP->at(2)->at(2);
	auto a = -(a00 + a11 + a22);
	auto b = a00 * a11 + (a00 * a22) + (a11 * a22) - (a01 * a01) - (a02 * a02) - (a12 * a12);
	auto c = a00 * a12 * a12 + (a11 * a02 * a02) + (a22 * a01 * a01) - (a00 * a11 * a22) - (2.0 * a01 * a02 * a12);
	auto aDiv3 = a / 3.0;
	auto p = (b / 3.0) - (aDiv3 * aDiv3);
	auto q = (c + (2.0 * aDiv3 * aDiv3 * aDiv3) - (aDiv3 * b)) / 2.0;
	auto phiDiv3 = modifiedArcCos(-q / std::sqrt(-p * p * p)) / 3.0;
	auto twoSqrtMinusp = 2.0 * std::sqrt(-p);
	auto piDiv3 = M_PI / 3.0;
	auto sortedJ = std::make_shared<DiagonalMatrix<double>>();
	sortedJ->push_back(twoSqrtMinusp * std::cos(phiDiv3));
	sortedJ->push_back(twoSqrtMinusp * -std::cos(phiDiv3 + piDiv3));
	sortedJ->push_back(twoSqrtMinusp * -std::cos(phiDiv3 - piDiv3));
	std::sort(sortedJ->begin(), sortedJ->end(), [](double a, double b) { return a < b; });
	auto lam0 = sortedJ->at(0) - aDiv3;
	auto lam1 = sortedJ->at(1) - aDiv3;
	auto lam2 = sortedJ->at(2) - aDiv3;
	if (lam1 == 0 || std::abs((lam0 / lam1) - 1.0) < 1.0e-6) {
		if (lam2 == 0 || std::abs((lam1 / lam2) - 1.0) < 1.0e-6) {
			//"All are equal."
			average = (lam0 + lam1 + lam2) / 3.0;
			lam0 = average;
			lam1 = average;
			lam2 = average;
		}
		else {
			//"lam0 = lam1"
			average = (lam0 + lam1) / 2.0;
			lam0 = average;
			lam1 = average;
		}
	}
	else {
		if (std::abs((lam1 / lam2) - 1.0) < 1.0e-6) {
			//"lam1 = lam2"
			average = (lam1 + lam2) / 2.0;
			lam1 = average;
			lam2 = average;
		}
	}
	aJpp = std::make_shared<DiagonalMatrix<double>>(ListD{ lam0, lam1, lam2 });
}

double MbD::MomentOfInertiaSolver::modifiedArcCos(double val)
{
	if (val > 1.0) {
		return 0.0;
	}
	else {
		if (val < -1.0) {
			return M_PI;
		}
		else {
			return std::acos(val);
		}
	}
}
