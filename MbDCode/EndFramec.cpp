#include <assert.h>
#include <memory>

#include "EndFramec.h"

using namespace MbD;

EndFramec::EndFramec() {
}

EndFramec::EndFramec(const char* str) : CartesianFrame(str) {
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

void EndFramec::initEndFrameqct()
{
	assert(false);
}

void MbD::EndFramec::calcPostDynCorrectorIteration()
{
}
