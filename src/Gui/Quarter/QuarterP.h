#ifndef QUARTER_QUARTERP_H
#define QUARTER_QUARTERP_H
#include <Inventor/SbName.h>
#include <QtGui/QCursor>
#ifndef _MSC_VER
#include <config.h>
#endif

template <class Key, class T> class QMap;

namespace SIM { namespace Coin3D { namespace Quarter {

class QuarterP {
 public:
  QuarterP();
  ~QuarterP();

  class SensorManager * sensormanager;
  class ImageReader * imagereader;

  typedef QMap<SbName, QCursor> StateCursorMap;
  static StateCursorMap * statecursormap;

  bool initCoin;
};

}}};

#define QUARTER_MAJOR_VERSION 1
#define QUARTER_MINOR_VERSION 0
#define QUARTER_MICRO_VERSION 0

#define COIN_CT_ASSERT(expr)                                            \
  do { switch ( 0 ) { case 0: case (expr): break; } } while ( 0 )

#define COMPILE_ONLY_BEFORE(MAJOR,MINOR,MICRO,REASON) \
  COIN_CT_ASSERT( (QUARTER_MAJOR_VERSION < MAJOR) || (QUARTER_MAJOR_VERSION == MAJOR && ((QUARTER_MINOR_VERSION < MINOR) || ( QUARTER_MINOR_VERSION == MINOR && (QUARTER_MICRO_VERSION < MICRO )))))

#endif //QUARTER_QUARTERP_H
