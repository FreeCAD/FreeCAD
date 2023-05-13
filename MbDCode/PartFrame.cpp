#include<algorithm>

#include "Part.h"
#include "PartFrame.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"

using namespace MbD;

PartFrame::PartFrame()
{
	initialize();
}
PartFrame::PartFrame(const char* str) : CartesianFrame(str)
{
	initialize();
}
void PartFrame::initialize()
{
	aGeu = std::make_shared<EulerConstraint>("EulerCon");
	aGeu->setOwner(this);
	aGabs = std::make_shared<std::vector<std::shared_ptr<AbsConstraint>>>();
	markerFrames = std::make_shared<std::vector<std::shared_ptr<MarkerFrame>>>();
}
void PartFrame::setqX(FColDsptr x) {
	qX->copy(x);
}
FColDsptr PartFrame::getqX() {
	return qX;
}
void PartFrame::setqE(FColDsptr x) {
	qE->copy(x);
}
FColDsptr PartFrame::getqE() {
	return qE;
}
void PartFrame::setPart(Part* x) {
	part = x;
}
Part* PartFrame::getPart() {
	return part;
}

void PartFrame::addMarkerFrame(std::shared_ptr<MarkerFrame> markerFrame)
{
	markerFrame->setPartFrame(this);
	markerFrames->push_back(markerFrame);
}

EndFrmcptr PartFrame::endFrame(std::string name)
{
	auto match = std::find_if(markerFrames->begin(), markerFrames->end(), [&](auto mkr) {return mkr->getName() == name; });
	return (*match)->endFrames->at(0);
}

void MbD::PartFrame::asFixed()
{
	for (int i = 0; i < 6; i++) {
		auto con = std::make_shared<AbsConstraint>(i);
		con->setOwner(this);
		aGabs->push_back(con);
	}
}

void PartFrame::initializeLocally()
{
	std::for_each(markerFrames->begin(), markerFrames->end(), [](const auto& markerFrame) { markerFrame->initializeLocally(); });
	aGeu->initializeLocally();
	std::for_each(aGabs->begin(), aGabs->end(), [](const auto& aGab) { aGab->initializeLocally(); });
}

void PartFrame::initializeGlobally()
{
	std::for_each(markerFrames->begin(), markerFrames->end(), [](const auto& markerFrame) { markerFrame->initializeGlobally(); });
	aGeu->initializeGlobally();
	std::for_each(aGabs->begin(), aGabs->end(), [](const auto& aGab) { aGab->initializeGlobally(); });
}
