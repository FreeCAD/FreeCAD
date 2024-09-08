/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <functional>

#include "GeneralSpline.h"
#include "CREATE.h"
#include "GESpMatParPvMarkoFast.h"
#include "DifferentiatedGeneralSpline.h"

using namespace MbD;

MbD::GeneralSpline::GeneralSpline(Symsptr arg) : AnyGeneralSpline(arg)
{
}

double MbD::GeneralSpline::getValue()
{
	return y(xx->getValue());
}

Symsptr MbD::GeneralSpline::differentiateWRTx()
{
	auto self = clonesptr();
	auto& arg = std::static_pointer_cast<GeneralSpline>(self)->xx;
	auto deriv = std::make_shared<DifferentiatedGeneralSpline>(arg, self, 1);
	return deriv;
}

void MbD::GeneralSpline::arguments(Symsptr args)
{
	auto array = args->getTerms();
	auto& arg = array->at(0);
	size_t order = (size_t)array->at(1)->getValue();
	size_t n = (array->size() - 2) / 2;
	auto xarray = std::make_shared<std::vector<double>>(n);
	auto yarray = std::make_shared<std::vector<double>>(n);
	for (size_t i = 0; i < n; i++)
	{
		size_t ii = 2 * (i + 1);
		xarray->at(i) = array->at(ii)->getValue();
		yarray->at(i) = array->at(ii + 1)->getValue();
	}
	initxdegreexsys(arg, order, xarray, yarray);
}

void MbD::GeneralSpline::initxdegreexsys(Symsptr arg, size_t order, std::shared_ptr<std::vector<double>> xarr, std::shared_ptr<std::vector<double>> yarr)
{
	xx = arg;
	degree = order;
	xs = xarr;
	ys = yarr;
	if (!Numeric::isIncreasingVector(xs.get())) throw std::runtime_error("x must be increasing.");
	computeDerivatives();
}

void MbD::GeneralSpline::computeDerivatives()
{
	//"See derivation in MbDTheory 9spline.fodt."
	if (degree == 0) throw std::runtime_error("ToDo: Use ZeroDegreeSpline");
	auto n = xs->size();
	auto p = degree;
	auto np = n * p;
	auto matrix = std::make_shared<SparseMatrix<double>>(np, np);
	auto bvector = std::make_shared<FullColumn<double>>(np, 0.0);
	auto hs = std::make_shared<FullColumn<double>>(n - 1);
	double hmax = 0.0;
	for (size_t i = 0; i < n - 1; i++)
	{
		double h = xs->at(i + 1) - xs->at(i);
		hmax = std::max(hmax, std::abs(h));
		hs->atiput(i, h);
	}
	for (size_t i = 0; i < n - 1; i++)
	{
		auto offset = i * p;
		double hbar = hs->at(i) / hmax;
		for (size_t j = 1; j < p; j++)
		{
			matrix->atijput(offset + j, offset + j - 1, 1.0);
			matrix->atijput(offset + j, offset + j - 1 + p, -1.0);
		}
		double dum = 1.0;
		for (size_t j = 0; j < p; j++)
		{
			dum = dum * hbar / (j + 1);
			for (size_t k = j; k < p; k++)
			{
				matrix->atijput(offset + k - j, offset + k, dum);
			}
		}
		bvector->atiput(offset, ys->at(i + 1) - ys->at(i));
	}
	if (isCyclic()) {
		for (size_t j = 1; j < p + 1; j++)
		{
			matrix->atijput(np - j, np - j, 1.0);
			matrix->atijput(np - j, p - j, -1.0);
		}
	}
	else {
		//"Zero out higher derivatives at node n and node 1 to get the p end equations."
		unsigned int count = 0;
		unsigned int npass = 0;
		while (count < p) {
			matrix->atijput(np - count, np - npass, 1.0);
			count++;
			if (count < p) {
				matrix->atijput(np - count, p - npass, 1.0);
				count++;
			}
		}
		npass = npass + 1;
	}
	auto solver = CREATE<GESpMatParPvMarkoFast>::With();
	auto derivsVector = solver->solvewithsaveOriginal(matrix, bvector, false);
	derivs = std::make_shared<FullMatrix<double>>(n, p);
	auto hmaxpowers = std::make_shared<FullColumn<double>>(p);
	for (size_t j = 0; j < p; j++)
	{
		hmaxpowers->atiput(j, std::pow(hmax, j + 1));
	}
	for (size_t i = 0; i < n; i++)
	{
		auto& derivsi = derivs->at(i);
		derivsi->equalArrayAt(derivsVector, (i - 1) * p + 1);
		for (size_t j = 0; j < p; j++)
		{
			derivsi->atiput(j, derivsi->at(j) / hmaxpowers->at(j));
		}
	}
}

bool MbD::GeneralSpline::isCyclic() const
{
	return (ys->size() > 3) && (ys->front() == ys->back());
}

double MbD::GeneralSpline::derivativeAt(size_t n, double xxx)
{
	//"dydx(x) := dydxi + d2ydx2i*hi + d3ydx3i*hi^2/2! +"
	//"d2ydx2(x) := d2ydx2i + d3ydx3i*hi + d4ydx4i*hi^2/2! +"
	if (n > degree) return 0.0;
	calcIndexAndDeltaFor(xxx);
	auto& derivsi = derivs->at(index);
	double sum = 0.0;
	for (ssize_t j = (ssize_t)degree; j >= (ssize_t) n + 1; j--)	//Use ssize_t because of decrement
	{
		sum = (sum + derivsi->at((size_t)j - 1)) * delta / (j - n);
	}
	return derivsi->at(n - 1) + sum;
}

void MbD::GeneralSpline::calcIndexAndDeltaFor(double xxx)
{
	xvalue = xxx;
	if (isCyclic()) {
		calcCyclicIndexAndDelta();
	}
	else {
		calcNonCyclicIndexAndDelta();
	}
}

void MbD::GeneralSpline::calcCyclicIndexAndDelta()
{
	double xFirst = xs->front();
	double xLast = xs->back();
	xvalue = std::fmod(xvalue - xFirst, xLast - xFirst) + xFirst;
	calcIndexAndDelta();
}

void MbD::GeneralSpline::calcNonCyclicIndexAndDelta()
{
	double xFirst = xs->front();
	double xLast = xs->back();
	if (xvalue <= xFirst) {
		index = 0;
		delta = xvalue - xFirst;
	}
	else {
		if (xvalue >= xLast) {
			index = xs->size() - 1;
			delta = xvalue - xLast;
		}
		else {
			calcIndexAndDelta();
		}
	}
}

void MbD::GeneralSpline::calcIndexAndDelta()
{
	if (!(index < xs->size() - 1) || !(xs->at(index) <= xvalue) || !(xvalue < xs->at(index + 1))) {
		searchIndexFromto(0, xs->size());	//Using range.
	}
	delta = xvalue - xs->at(index);
}

void MbD::GeneralSpline::searchIndexFromto(size_t first, size_t last)
{
	//"Assume xs(first) <= xvalue < xs(last)."
	if (first + 1 == last) {
		index = first;
	}
	else {
		auto middle = (size_t)std::floor((first + last) / 2);
		if (xvalue < xs->at(middle)) {
			searchIndexFromto(first, middle);
		}
		else {
			searchIndexFromto(middle, last);
		}
	}
}

Symsptr MbD::GeneralSpline::clonesptr()
{
	return std::make_shared<GeneralSpline>(*this);
}

double MbD::GeneralSpline::y(double xxx)
{
	//"y(x) := yi + dydxi*hi + d2ydx2i*hi^2/2! + d3ydx3i*hi^3/3! +"

	calcIndexAndDeltaFor(xxx);
	auto& derivsi = derivs->at(index);
	double sum = 0.0;
	for (ssize_t j = (ssize_t)degree; j >= 1; j--)	//Use ssize_t because of decrement
	{
		sum = (sum + derivsi->at((size_t)j - 1)) * delta / j;
	}
	return ys->at(index) + sum;
}

std::ostream& MbD::GeneralSpline::printOn(std::ostream& s) const
{
	s << "Spline(";
	s << *xx << ", ";
	s << degree << ", " << std::endl;
	s << "xs{";
	s << xs->at(0);
	for (size_t i = 1; i < xs->size(); i++)
	{
		s << ", " << xs->at(i);
	}
	s << "}, " << std::endl;
	s << "ys{";
	s << ys->at(0);
	for (size_t i = 1; i < ys->size(); i++)
	{
		s << ", " << ys->at(i);
	}
	s << "})" << std::endl;
	return s;
}
