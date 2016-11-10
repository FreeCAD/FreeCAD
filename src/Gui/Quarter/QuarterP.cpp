#include "QuarterP.h"
#include "SensorManager.h"
#include "ImageReader.h"
#include "KeyboardP.h"

using namespace SIM::Coin3D::Quarter;
QuarterP::StateCursorMap * QuarterP::statecursormap = NULL;

QuarterP::QuarterP(void)
{
  this->sensormanager = new SensorManager;
  this->imagereader = new ImageReader;
  assert(QuarterP::statecursormap == NULL);
  QuarterP::statecursormap = new StateCursorMap;

}

QuarterP::~QuarterP()
{
  delete this->imagereader;
  delete this->sensormanager;

  assert(QuarterP::statecursormap != NULL);
  delete QuarterP::statecursormap;

  // FIXME: Why not use an atexit mechanism for this?
  if (KeyboardP::keyboardmap != NULL) {
    KeyboardP::keyboardmap->clear();
    KeyboardP::keypadmap->clear();
    delete KeyboardP::keyboardmap;
    delete KeyboardP::keypadmap;
    KeyboardP::keyboardmap = NULL;
    KeyboardP::keypadmap = NULL;
  }


}
