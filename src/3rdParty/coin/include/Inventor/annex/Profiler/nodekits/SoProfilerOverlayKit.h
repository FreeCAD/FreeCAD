#ifndef COIN_SOPROFILEROVERLAYKIT_H
#define COIN_SOPROFILEROVERLAYKIT_H

#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/fields/SoSFVec3f.h>

class COIN_DLL_API SoProfilerOverlayKit : public SoBaseKit {
  typedef SoBaseKit inherited;
  SO_KIT_HEADER(SoProfilerOverlayKit);
  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(profilingStats);
  SO_KIT_CATALOG_ENTRY_HEADER(viewportInfo);
  SO_KIT_CATALOG_ENTRY_HEADER(overlayCamera);
  SO_KIT_CATALOG_ENTRY_HEADER(depthTestOff);
  SO_KIT_CATALOG_ENTRY_HEADER(overlaySep);
  SO_KIT_CATALOG_ENTRY_HEADER(depthTestOn);

public:
  static void initClass(void);
  SoProfilerOverlayKit(void);

  SoSFVec3f viewportSize; // output in pixels for internal use

  void addOverlayGeometry(SoNode * node);

protected:
  virtual ~SoProfilerOverlayKit(void);

private:
  struct SoProfilerOverlayKitP * pimpl;
};

#endif // !COIN_SOPROFILEROVERLAYKIT_H
