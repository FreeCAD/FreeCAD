#include "FunctionWithManyArgs.h"
#include "Symbolic.h"

using namespace MbD;

FunctionWithManyArgs::FunctionWithManyArgs(std::shared_ptr<Symbolic> term)
{
	terms = std::make_shared<std::vector<std::shared_ptr<Symbolic>>>();
	terms->push_back(term);
}

FunctionWithManyArgs::FunctionWithManyArgs(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1) : FunctionWithManyArgs(term)
{
	terms->push_back(term1);
}

FunctionWithManyArgs::FunctionWithManyArgs(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1, std::shared_ptr<Symbolic> term2) : FunctionWithManyArgs(term, term1)
{
	terms->push_back(term2);
}

FunctionWithManyArgs::FunctionWithManyArgs(std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> _terms) {
	terms = std::make_shared<std::vector<std::shared_ptr<Symbolic>>>();
	for (int i = 0; i < _terms->size(); i++)
		terms->push_back(_terms->at(i));
}

