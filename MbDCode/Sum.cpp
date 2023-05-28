#include "Sum.h"

using namespace MbD;

double Sum::getValue()
{
	double answer = 0.0;
	for (int i = 0; i < terms->size(); i++) answer += terms->at(i)->getValue();
	return answer;
}
