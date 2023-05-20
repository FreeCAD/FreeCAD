#include "Product.h"

using namespace MbD;

double Product::getValue()
{
	double answer = 1.0;
	for (int i = 0; i < terms->size(); i++) answer *= terms->at(i)->getValue();
	return answer;
}
