#include "EndFramec.h"

using namespace MbD;

EndFramec::EndFramec() {
	initialize();
}

EndFramec::EndFramec(const char* str) : CartesianFrame(str) {
	initialize();
}

void EndFramec::initialize()
{
}

void EndFramec::setMarkerFrame(MarkerFrame* markerFrm)
{
	markerFrame = markerFrm;
}
