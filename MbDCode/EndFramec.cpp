#include <assert.h>
#include <memory>

#include "EndFramec.h"
#include "MarkerFrame.h"

using namespace MbD;

EndFramec::EndFramec() {
}

EndFramec::EndFramec(const char* str) : CartesianFrame(str) {
}

System* EndFramec::root()
{
	return markerFrame->root();
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

void EndFramec::calcPostDynCorrectorIteration()
{
	rOeO = markerFrame->rOmO;
	aAOe = markerFrame->aAOm;
}

FColDsptr EndFramec::aAjOe(int j)
{
	return aAOe->column(j);
}

double EndFramec::riOeO(int i)
{
	return rOeO->at(i);
}

FColDsptr EndFramec::rmeO()
{
	return rOeO->minusFullColumn(markerFrame->rOmO);
}

FColDsptr EndFramec::rpep()
{
	return FColDsptr();
}

FColFMatDsptr EndFramec::pAOppE()
{
	return FColFMatDsptr();
}

FMatDsptr EndFramec::aBOp()
{
	return FMatDsptr();
}
