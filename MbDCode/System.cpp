#include "System.h"

void MbD::System::addPart(std::shared_ptr<Part> part)
{
	part->setSystem(*this);
	parts.push_back(part);
}

MbD::System::System() {
	parts = std::vector<std::shared_ptr<Part>>();
	jointsMotions = std::vector<std::shared_ptr<Joint>>();
	systemSolver = std::make_shared<SystemSolver>(*this);
}
