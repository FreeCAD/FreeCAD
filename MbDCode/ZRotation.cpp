#include "ZRotation.h"

using namespace MbD;

ZRotation::ZRotation() {
	initialize();
}

ZRotation::ZRotation(const char* str) : PrescribedMotion(str) {
	initialize();
}

void ZRotation::initialize()
{
}
