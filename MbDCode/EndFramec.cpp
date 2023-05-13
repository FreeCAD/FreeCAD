#include <memory>

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

MarkerFrame* EndFramec::getMarkerFrame()
{
	return markerFrame;
}

void EndFramec::initializeLocally()
{
}

void EndFramec::initializeGlobally()
{
}

void MbD::EndFramec::EndFrameqctFrom(std::shared_ptr<EndFramec>& newFrmI)
{
}
