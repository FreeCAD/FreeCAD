#include "Part.h"
#include "PartFrame.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"

using namespace MbD;

PartFrame::PartFrame()
{
	aGeu = std::make_shared<EulerConstraint>();
	aGabs = std::vector<std::shared_ptr<AbsConstraint>>();
	markerFrames = std::vector<std::shared_ptr<MarkerFrame>>();
}
void PartFrame::setqX(FullColumn<double>* x) {
	qX->copy(x);
}
FullColumn<double>* PartFrame::getqX() {
	return qX;
}
void PartFrame::setqE(FullColumn<double>* x) {
	qE->copy(x);
}
FullColumn<double>* PartFrame::getqE() {
	return qE;
}
void PartFrame::setPart(std::shared_ptr<Part> x) {
	part = x;
}
std::shared_ptr<Part> PartFrame::getPart() {
	return part.lock();
}

void MbD::PartFrame::addMarkerFrame(std::shared_ptr<MarkerFrame> markerFrame)
{
	markerFrame->setPartFrame(this);
	markerFrames.push_back(markerFrame);
}
