#include "FunctionWithManyArgs.h"
#include "Symbolic.h"

using namespace MbD;

FunctionWithManyArgs::FunctionWithManyArgs()
{
	terms = std::make_shared<std::vector<Symsptr>>();
}

FunctionWithManyArgs::FunctionWithManyArgs(Symsptr term) : FunctionWithManyArgs()
{
	terms->push_back(term);
}

FunctionWithManyArgs::FunctionWithManyArgs(Symsptr term, Symsptr term1) : FunctionWithManyArgs(term)
{
	terms->push_back(term1);
}

FunctionWithManyArgs::FunctionWithManyArgs(Symsptr term, Symsptr term1, Symsptr term2) : FunctionWithManyArgs(term, term1)
{
	terms->push_back(term2);
}

FunctionWithManyArgs::FunctionWithManyArgs(std::shared_ptr<std::vector<Symsptr>> _terms) {
	terms = std::make_shared<std::vector<Symsptr>>();
	for (int i = 0; i < _terms->size(); i++)
		terms->push_back(_terms->at(i));
}

std::shared_ptr<std::vector<Symsptr>> FunctionWithManyArgs::getTerms()
{
	return terms;
}

