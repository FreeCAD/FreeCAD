#ifndef COIN_SOCONFIGSETTINGS_H
#define COIN_SOCONFIGSETTINGS_H

class SbString;

class SoConfigSettings
{
 public:
  static SoConfigSettings * getInstance();

  void reinitialize();
  const SbString & getSetting(const SbString &) const;
  int settingAsInt(const SbString &) const;

 protected:
  SoConfigSettings();
  ~SoConfigSettings();
  static void destroy();

 private:
  class SoConfigSettingsP * pimpl;
};

#endif //COIN_SOCONFIGSETTINGS_H
