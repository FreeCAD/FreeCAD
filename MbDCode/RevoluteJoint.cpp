#include "RevoluteJoint.h"

using namespace MbD;

RevoluteJoint::RevoluteJoint() {
	initialize();
}

RevoluteJoint::RevoluteJoint(const char* str) : Joint(str) {
	initialize();
}

void RevoluteJoint::initialize()
{
}
