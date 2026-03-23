/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SoAudioDevice SoAudioDevice.h Inventor/misc/SoAudioDevice.h
  \brief The SoAudioDevice class is used to control an audio device.

  \ingroup coin_general
  \ingroup coin_sound

  The SoAudioDevice class is responsible for initialization of an 
  audio device, as well as enabling and disabling sound. It is a singleton
  class.

  The application programmer does not need to use this class directly, as
  audio support is enabled by default, and the default settings are 
  reasonable.

  Coin uses OpenAL (http://www.openal.org/,
  http://developer.soundblaster.com [Games section]) to render audio.
  OpenAL should work with any soundcard, and on most modern operating
  systems (including Unix, Linux, IRIX, *BSD, Mac OS X and Microsoft
  Windows). 2 speaker output is always available, and on some OS and
  soundcard combinations, more advanced speaker configurations are
  supported. On Microsoft Windows, OpenAL can use DirectSound3D to
  render audio, thus supporting any speaker configuration the current
  DirectSound3D driver supports. Configuring speakers are done through
  the soundcard driver, and is transparent to both Coin and OpenAL.  
*/

// *************************************************************************

#include <Inventor/misc/SoAudioDevice.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"
#include "misc/AudioTools.h"
#include "glue/openal_wrapper.h"

// *************************************************************************

class SoAudioDeviceP {
public:
  SoAudioDeviceP(SoAudioDevice * master);
  ~SoAudioDeviceP();

  static SoAudioDevice *singleton;

  static void clean();

  void *context;
  void *device;

  SbBool enabled;
  SbBool initOK;
  float lastGain;

private:
  SoAudioDevice *master;
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

SoAudioDevice *SoAudioDeviceP::singleton = NULL;

// *************************************************************************

/*!
  Returns a pointer to the SoAudioDevice class, which is a singleton.
 */

SoAudioDevice *
SoAudioDevice::instance()
{
  if (SoAudioDeviceP::singleton == NULL) {
    SoAudioDeviceP::singleton = new SoAudioDevice();

    // Note: there is a known problem with the OpenAL driver on
    // certain platforms. If cleanup is not done before application
    // exit, that is, alcDestroyContext() and alcCloseDevice() is
    // *not* invoked, the application hangs on exit.
    //
    // For Coin, that means one /has/ to invoke SoDB::finish() for
    // application which uses sound, or the application will hang on
    // exit for some users.
    coin_atexit((coin_atexit_f *)SoAudioDeviceP::clean, CC_ATEXIT_NORMAL);
  }
  return SoAudioDeviceP::singleton;
}

void
SoAudioDeviceP::clean()
{
  delete SoAudioDeviceP::singleton;
  SoAudioDeviceP::singleton = NULL;
}

// *************************************************************************

SoAudioDeviceP::SoAudioDeviceP(SoAudioDevice * master)
  : master(master)
{
  this->context = NULL;
  this->device = NULL;
  this->enabled = FALSE;
  this->initOK = FALSE;
  this->lastGain = 1.0f;
}

SoAudioDeviceP::~SoAudioDeviceP()
{
  if (this->context) { openal_wrapper()->alcDestroyContext(this->context); }
  if (this->device) { openal_wrapper()->alcCloseDevice(this->device); }
}

// *************************************************************************

/*!
  Constructor
 */

SoAudioDevice::SoAudioDevice()
{
  PRIVATE(this) = NULL;

  const char * env = coin_getenv("COIN_SOUND_DRIVER_NAME");
  (void)this->init("OpenAL", env ? env : "DirectSound3D");
}

/*!
  Destructor
 */

SoAudioDevice::~SoAudioDevice()
{
  if (coin_debug_audio()) {
    SoDebugError::postInfo("SoAudioDevice::~SoAudioDevice", "closing");
  }

  if (this->haveSound()) { this->disable(); }
  delete PRIVATE(this);
}

/*!
  Initializes the audio device. Currently, the only supported \a devicetype
  is "OpenAL". The supported \a devicename depends on the OS and on installed
  sound cards and drivers. On Microsoft Windows, supported device names are
  "DirectSound3D", "DirectSound", and "MMSYSTEM". See OpenAL documentation
  (available from http://www.openal.org/) for further information.

  The application programmer may override the default setting by
  calling this method with the wanted device type and name.

  The user can also control which \a devicename OpenAL uses by setting
  the COIN_SOUND_DRIVER_NAME environment variable. On Microsoft
  Windows, the default driver name is "DirectSound3D", which should
  normally be what the user wants.
*/
SbBool
SoAudioDevice::init(const SbString & devicetype, const SbString & devicename)
{
  if (PRIVATE(this)) {
    if (this->haveSound()) { this->disable(); }
    delete PRIVATE(this);
  }

  PRIVATE(this) = new SoAudioDeviceP(this);


  // Default disabled, as sound support through OpenAL caused crashes
  // under Linux.
  SbBool initaudio = FALSE;
  // FIXME: nobody bothered to document what was crashing, and how and
  // why and how to solve it, though. *grumpf*.
  //
  // And it works for me, on my Debian Linux system with the following
  // OpenAL specifics:
  //
  // AL_VENDOR=='J. Valenzuela'
  // AL_VERSION=='0.0.6'
  // AL_RENDERER=='Software'
  // AL_EXTENSIONS==''
  //
  // 20030507 mortene.

#ifdef HAVE_WIN32_API
  initaudio = TRUE;
#endif // HAVE_WIN32_API

  const char * env = coin_getenv("COIN_SOUND_ENABLE");
  if (env) {
    if (!initaudio && atoi(env) > 0) {
      SoDebugError::postInfo("SoAudioDevice::init", 
                             "Sound has been enabled because the environment variable "
                             "COIN_SOUND_ENABLE=1. Sound support on this platform is considered "
                             "experimental, and is therefore not enabled by default. "
                             SOUND_NOT_ENABLED_BY_DEFAULT_STRING );
      initaudio = TRUE;
    }
    else if (initaudio && atoi(env) == 0) {
      SoDebugError::postInfo("SoAudioDevice::init", 
                             "Sound has been disabled because the environment variable "
                             "COIN_SOUND_ENABLE=0.");
      initaudio = FALSE;
    }
  }

  // Yes, there's both COIN_SOUND_ENABLE and COIN_SOUND_DISABLE -- one
  // should be sufficient, but for compatibility reasons with
  // existing runtime systems, we keep them both.
  env = coin_getenv("COIN_SOUND_DISABLE");
  if (env && (atoi(env) > 0)) {
    if (coin_debug_audio()) {
      SoDebugError::postInfo("SoAudioDevice::init", 
                             "Sound has been disabled because the "
                             "environment variable COIN_SOUND_DISABLE was set.");
    }
    initaudio = FALSE;
  }


  if (!initaudio) { return FALSE; }

  if (devicetype != "OpenAL") {
    SoDebugError::postWarning("SoAudioDevice::init",
                              "devicetype != OpenAL - currently OpenAL is "
                              "the only supported device type for audio "
                              "rendering");
    return FALSE;
  }

#ifndef HAVE_SOUND
  SoDebugError::postWarning("SoAudioDevice::init",
                            "Sound support was forced off when building "
                            "this Coin binary.");
  return FALSE;
#endif // !HAVE_SOUND

  if (!openal_wrapper()->available) {
    PRIVATE(this)->enabled = FALSE;
    PRIVATE(this)->initOK = FALSE;
    return FALSE;
  }

  PRIVATE(this)->device = 
    openal_wrapper()->alcOpenDevice((unsigned char*)devicename.getString());

  if (PRIVATE(this)->device == NULL) {
    SoDebugError::postWarning("SoAudioDevice::init",
                              "Failed to initialize OpenAL. "
                              "Sound will not be available.");
    return FALSE;
  }

  // FIXME: the version string should be checked against the minimum
  // version we demand. A standard "Debian testing" distribution as of
  // now comes with version 0.0.6, for instance, and that one has
  // problems with thammer's code. It is unknown if the problems are
  // caused by our code, or by old bugs in OpenAL, though.
  //
  // const ALubyte * str = alGetString(AL_VERSION);
  //
  // 20021029 mortene.

  PRIVATE(this)->context = 
    openal_wrapper()->alcCreateContext(PRIVATE(this)->device, NULL);
  openal_wrapper()->alcMakeContextCurrent(PRIVATE(this)->context);

  // Clear Error Code
  //
  // FIXME: huh? This seems bogus -- shouldn't we rather check and
  // report if there is one? 20050627 mortene.
  (void)openal_wrapper()->alGetError();

  // Set listener parameters (position, orientation, velocity, gain)
  // These will never change, since we're simulating listener movement by
  // moving sounds instead of moving the listener. 2002-11-13 thammer.
  int error;
  float alfloat3[3] = { 0.0f, 0.0f, 0.0f };
  float alfloat6[6] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
  float gain = 1.0f;

  // FIXME: why disable sound all together if any one of the below
  // calls fail? 20050627 mortene.

  // FIXME: it seems like it would be better to integrate error
  // checking into the OpenAL wrapper..? At least for catching and
  // reporting non-fatal errors. 20050627 mortene.

  openal_wrapper()->alListenerfv(AL_POSITION, alfloat3);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    if (coin_debug_audio())
      SoDebugError::postWarning("SoAudioDevice::init",
                                "alListenerfv(AL_POSITION,) failed. %s."
                                "Sound will not be available.",
                                coin_get_openal_error(error));
    return FALSE;
  }

  openal_wrapper()->alListenerfv(AL_VELOCITY, alfloat3);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    if (coin_debug_audio())
      SoDebugError::postWarning("SoAudioDevice::init",
                                "alListenerfv(AL_VELOCITY,) failed. %s."
                                "Sound will not be available.",
                                coin_get_openal_error(error));
    return FALSE;
  }
  
  openal_wrapper()->alListenerfv(AL_ORIENTATION, alfloat6);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    if (coin_debug_audio())
      SoDebugError::postWarning("SoAudioDevice::init",
                                "alListenerfv(AL_ORIENTATION,) failed. %s."
                                "Sound will not be available.",
                                coin_get_openal_error(error));
    return FALSE;
  }

  openal_wrapper()->alListenerf(AL_GAIN, gain);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    if (coin_debug_audio())
      SoDebugError::postWarning("SoAudioDevice::init",
                                "alListenerf(AL_GAIN,) failed. %s."
                                "Sound will not be available.",
                                coin_get_openal_error(error));
    return FALSE;
  }

  // Disable OpenAL's distance attenuation since it doesn't
  // fit very well with the model used by the VRML Sound node.
  // The line below disables distance attenuation for all sources.
  // Distance attenuation is also disabled on a per-source basis
  // in SoVRMLSound, by setting the rolloff factor to 0.0 in
  // generateAlSource() and by normalizing the position of the 
  // source relative to the listener in audioRender(). The reason
  // we do the same thing all these places is to be more robust
  // for drivers not supporting all these methods (see note regarding
  // CreativeLabs Extigy in SoVRMLSoundP::generateAlSource()).
  // Note that if we later want to implement sound nodes that use
  // OpenAL's distance attenuation, we can enable
  // AL_INVERSE_DISTANCE_CLAMPED distance model here and 
  // nullify the distance attenuation in SoVRMLSound instead. 
  // 
  // 2005-04-10 thammer
  openal_wrapper()->alDistanceModel(AL_NONE);

  PRIVATE(this)->enabled = TRUE;
  PRIVATE(this)->initOK = TRUE;

  if (coin_debug_audio() && PRIVATE(this)->initOK) {
    SoDebugError::postInfo("SoAudioDevice::init",
                           "Initialization succeeded");
  }
  
  // Notify SoSceneManager that sound is (maybe) being used, so it
  // should start applying an SoAudioRenderAction on its scene graphs.
  //
  // For further information, see function's code comments in
  // AudioTools.cpp.
  coin_sound_enable_traverse();

  return TRUE;
}

/*!
  Returns true if the audio device has been initialized successfully.
 */

SbBool SoAudioDevice::haveSound()
{
  return PRIVATE(this)->initOK;
}

/*!
  Enables sound
 */

SbBool SoAudioDevice::enable()
{
  if (!this->haveSound())
    return FALSE;

  if (PRIVATE(this)->enabled)
    return TRUE; // already enabled

  PRIVATE(this)->enabled = TRUE;

  openal_wrapper()->alcProcessContext(PRIVATE(this)->context);

  return TRUE;
}

/*!
  Disables sound. Effectively silencing all audio output.
 */

void SoAudioDevice::disable()
{
  if (!this->haveSound())
    return;

  if (!PRIVATE(this)->enabled)
    return; // already disabled
  
  PRIVATE(this)->enabled = FALSE;

  openal_wrapper()->alcSuspendContext(PRIVATE(this)->context);
}

/*!
  Returns TRUE if audio is enabled.
 */

SbBool 
SoAudioDevice::isEnabled()
{
  return PRIVATE(this)->enabled;
}

void 
SoAudioDevice::setGain(float gain)
{
  if (!this->haveSound())
    return;

  gain = (gain < 0.0f) ? 0.0f : gain;

  int error;
  openal_wrapper()->alListenerf(AL_GAIN, gain);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoAudioDevice::setGain",
                              "alListenerf(AL_GAIN,) failed. %s",
                              coin_get_openal_error(error));
    return;
  }

  PRIVATE(this)->lastGain = gain;
}

void 
SoAudioDevice::mute(SbBool mute)
{
  if (mute) {
    float lastgain = PRIVATE(this)->lastGain;
    this->setGain(0.0f);
    PRIVATE(this)->lastGain = lastgain;
  } else {
    this->setGain(PRIVATE(this)->lastGain);
  }
}

#undef PRIVATE
#undef PUBLIC
