#include <assert.h>
#include <memory>

#include "EndFramec.h"
#include "MarkerFrame.h"

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

void EndFramec::initEndFrameqct()
{
	assert(false);
}

void MbD::EndFramec::calcPostDynCorrectorIteration()
{
	rOeO = markerFrame->rOmO;
	aAOe = markerFrame->aAOm;
}

FColDsptr MbD::EndFramec::aAjOe(int j)
{
	return aAOe->column(j);
}

double MbD::EndFramec::riOeO(int i)
{
	return rOeO->at(i);
}
