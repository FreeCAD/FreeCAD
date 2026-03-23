#include <Inventor/C/basic.h>
#include <Inventor/SbString.h>
#include <Inventor/errors/SoDebugError.h>
#include "SoConfigSettings.h"
#include "misc/SbHash.h"

#include <cstdio>


namespace {
  SoConfigSettings * singleton = NULL;
  const char * VALID_OPTIONS[] = {
    "COIN_VBO",
    "COIN_WARNING_LEVEL"
  };

  inline size_t options_size()
  {
    return sizeof(VALID_OPTIONS)/sizeof(VALID_OPTIONS[0]);
  }
  const char COIN [] = "COIN";

  SbBool isValidOption(const SbString & option)
  {
    size_t i;
    for (i = 0; i<options_size() && option != VALID_OPTIONS[i]; ++i) {}
    return (i!=options_size());
  }

  const SbString INVALID_SETTING("");
};

class SoConfigSettingsP {
public:
  SbHash<SbString,SbString> settings;
};

#define PRIVATE(X) ((X)->pimpl)

SoConfigSettings *
SoConfigSettings::getInstance()
{
  if (!singleton)
    singleton = new SoConfigSettings();
  return singleton;
}

void
SoConfigSettings::destroy()
{
  delete singleton;
  singleton = NULL;
}

SoConfigSettings::SoConfigSettings()
{
  this->pimpl = new SoConfigSettingsP();
  this->reinitialize();

  //FIXME: Get this working correctly, needs correct priority BFG 20091009
  //cc_coin_atexit(static_cast<coin_atexit_f*>(destroy));
}

SoConfigSettings::~SoConfigSettings()
{
  delete this->pimpl;
}

void
SoConfigSettings::reinitialize()
{
  for (size_t i = 0; i<options_size(); ++i) {
    const char * envVariable = coin_getenv(VALID_OPTIONS[i]);
    if (envVariable) {
      PRIVATE(this)->settings[ VALID_OPTIONS[i] ] = envVariable;
    }
  }
  //FIXME: environ is not available on non-unix platforms, so
  //disabling this for now. Write a configure test for this in the
  //future. BFG 20091013
#if COIN_DEBUG && 0
  for (char ** test = environ; *test != NULL; ++test) {
    char * first = strchr(*test,'=');
    if (first) {
      size_t n;
      if ((n=first - *test) < sizeof(COIN)-1) {
        continue;
      }
      if (strncmp(*test,COIN,sizeof(COIN)-1)==0) {
        size_t i;
        SbString option(*test,0,n-1);
        if (!isValidOption(option))
          SoDebugError::postInfo(HAVE_CPP_COMPILER_FUNCTION_NAME_VAR, "%s seems like a coin setting, but is not recognized\n",*test);
      }
    }
  }
#endif //COIN_DEBUG
}


const SbString &
SoConfigSettings::getSetting(const SbString & setting) const
{
  SbHash<SbString,SbString>::const_iterator iter = PRIVATE(this)->settings.find(setting);
  return (PRIVATE(this)->settings.const_end()!=iter)?
    iter->obj:INVALID_SETTING;
}

int
SoConfigSettings::settingAsInt(const SbString & setting) const
{
  return atoi(this->getSetting(setting).getString());
}


#undef PRIVATE
