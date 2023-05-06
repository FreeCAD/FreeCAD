#include "Joint.h"

using namespace MbD;

Joint::Joint() {
	initialize();
}

Joint::Joint(const char* str) : Item(str) {
	initialize();
}

void Joint::initialize()
{
	constraints = std::make_unique<std::vector<std::shared_ptr<Constraint>>>();
}

void Joint::connectsItoJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj)
{
	frmI = frmi;
	frmJ = frmj;
}

void Joint::initializeLocally()
{
}

void Joint::initializeGlobally()
{
}
