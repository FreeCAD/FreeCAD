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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLAudioClip SoVRMLAudioClip.h Inventor/VRMLnodes/SoVRMLAudioClip.h
  \brief The SoVRMLAudioClip class is used to load and store audio data.

  \ingroup coin_VRMLnodes
  \ingroup coin_sound

  Audio data is loaded using the simage library, so make sure you have
  built the simage library with support for the audio file formats you
  intend to use (libogg, libvorbis and libvorbisfile for OggVorbis,
  libsndfile for WAV and several other formats).

  \WEB3DCOPYRIGHT

  \verbatim
  AudioClip {
    exposedField   SFString description      ""
    exposedField   SFBool   loop             FALSE
    exposedField   SFFloat  pitch            1.0        # (0, inf)
    exposedField   SFTime   startTime        0          # (-inf, inf)
    exposedField   SFTime   stopTime         0          # (-inf, inf)
    exposedField   MFString url              []
    eventOut       SFTime   duration_changed
    eventOut       SFBool   isActive
  }
  \endverbatim

  An AudioClip node specifies audio data that can be referenced by
  Sound nodes.  The description field specifies a textual description
  of the audio source. A browser is not required to display the
  description field but may choose to do so in addition to playing the
  sound.  The url field specifies the URL from which the sound is
  loaded.  Browsers shall support at least the wavefile format in
  uncompressed PCM format (see
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/bibliography.html#[WAV]).
  It is recommended that browsers also support the MIDI file type 1
  sound format
  (see http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[MIDI]);
  MIDI files are presumed to use the
  General MIDI patch set. Subclause 4.5, VRML and the World Wide Web
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.5>),
  contains details on the url field. The results are undefined when no
  URLs refer to supported data types.

  The loop, startTime, and stopTime exposedFields and the isActive
  eventOut, and their effects on the AudioClip node, are discussed in
  detail in 4.6.9, Time-dependent nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.9>).
  The "cycle" of an AudioClip is the length of time in seconds for one
  playing of the audio at the specified pitch.  The pitch field
  specifies a multiplier for the rate at which sampled sound is
  played. Values for the pitch field shall be greater than
  zero. Changing the pitch field affects both the pitch and playback
  speed of a sound. A set_pitch event to an active AudioClip is
  ignored and no pitch_changed eventOut is generated. If pitch is set
  to 2.0, the sound shall be played one octave higher than normal and
  played twice as fast. For a sampled sound, the pitch field alters
  the sampling rate at which the sound is played. The proper
  implementation of pitch control for MIDI (or other note sequence
  sound clips) is to multiply the tempo of the playback by the pitch
  value and adjust the MIDI Coarse Tune and Fine Tune controls to
  achieve the proper pitch change.  A duration_changed event is sent
  whenever there is a new value for the "normal" duration of the
  clip. Typically, this will only occur when the current url in use
  changes and the sound data have been loaded, indicating that the clip
  is playing a different sound source.  The duration is the length of
  time in seconds for one cycle of the audio for a pitch set to
  1.0. Changing the pitch field will not trigger a duration_changed
  event. A duration value of "-1" implies that the sound data have not
  yet loaded or the value is unavailable for some reason. A
  duration_changed event shall be generated if the AudioClip node is
  loaded when the VRML file is read or the AudioClip node is added to
  the scene graph.  The isActive eventOut may be used by other nodes
  to determine if the clip is currently active. If an AudioClip is
  active, it shall be playing the sound corresponding to the sound
  time (i.e., in the sound's local time system with sample 0 at time
  0):
  \verbatim
  t = (now - startTime) modulo (duration / pitch)
  \endverbatim */

/*!
  \var SoSFString SoVRMLAudioClip::description
  Description of the audio clip. Default value is an empty string.
*/

/*!
  \var SoSFBool SoVRMLAudioClip::loop
  Specifies whether sound should be looped. Is FALSE by default.
*/

/*!
  \var SoSFFloat SoVRMLAudioClip::pitch
  Specifies the pitch. The default value is 1.0.

  Alters the sampling rate at which the sound is played. A pitch of
  2.0 means that the sound should be played twice as fast and one
  octave higher than normal.
*/

/*!
  \var SoSFTime SoVRMLAudioClip::startTime
  Specifies the start time. Default value is 0.
*/
/*!
  \var SoSFTime SoVRMLAudioClip::stopTime
  Specifies the stop time. Default value is 0.
*/

/*!
  \var SoMFString SoVRMLAudioClip::url
  The audio data URL.
*/

/*!
  \var SoVRMLAudioClip::duration_changed
  An eventOut sent when a new sound starts playing.
*/

/*!
  \var SoVRMLAudioClip::isActive
  This eventOut is sent when the sound starts/stops playing.
*/

#include <Inventor/VRMLnodes/SoVRMLAudioClip.h>
#include "coindefs.h"

#include <cstring>
#include <cstdio> // for EOF

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoAudioDevice.h>
#include <Inventor/SoInput.h>
#include <Inventor/C/tidbits.h>

#ifdef HAVE_THREADS
#include <Inventor/threads/SbMutex.h>
#include <Inventor/threads/SbThreadAutoLock.h>
#endif

#include "nodes/SoSubNodeP.h"
#include "glue/simage_wrapper.h"
#include "tidbitsp.h"

#define DEBUG_AUDIO 0

class SoVRMLAudioClipP
{
public:
  SoVRMLAudioClipP(SoVRMLAudioClip * master) : master(master) {};
  SoVRMLAudioClip *master;

  static void urlSensorCBWrapper(void *, SoSensor *);
  void urlSensorCB(SoSensor *);

  static void loopSensorCBWrapper(void *, SoSensor *);
  void loopSensorCB(SoSensor *);

  static void startTimeSensorCBWrapper(void *, SoSensor *);
  void startTimeSensorCB(SoSensor *);

  static void stopTimeSensorCBWrapper(void *, SoSensor *);
  void stopTimeSensorCB(SoSensor *);

  static void timerCBWrapper(void *, SoSensor *);
  void timerCB(SoSensor *);

  void * internal_open(const SbStringList &url, SoVRMLAudioClip *clip);
  size_t internal_read(void *datasource, void *buffer, 
                       int numframes, int &channels, SoVRMLAudioClip *clip);
  int    internal_seek(void *datasource, long offset, int whence,
                       SoVRMLAudioClip *clip);
  long   internal_tell(void *datasource, SoVRMLAudioClip *clip);
  int    internal_close(void *datasource, SoVRMLAudioClip *clip);

  static void * internal_open_wrapper(const SbStringList &url,
                                      SoVRMLAudioClip *clip, 
                                      void *userdataptr);
  static size_t internal_read_wrapper(void *datasource, void *buffer, 
                                      int numframes, int &channels,
                                      SoVRMLAudioClip *clip, 
                                      void *userdataptr);
  static int    internal_seek_wrapper(void *datasource, long offset, 
                                      int whence, SoVRMLAudioClip *clip, 
                                      void *userdataptr);
  static long   internal_tell_wrapper(void *datasource,
                                      SoVRMLAudioClip *clip, 
                                      void *userdataptr);
  static int    internal_close_wrapper(void *datasource,
                                       SoVRMLAudioClip *clip, 
                                       void *userdataptr);
  
  SoVRMLAudioClip::open_func *open;
  SoVRMLAudioClip::read_func *read;
  SoVRMLAudioClip::seek_func *seek;
  SoVRMLAudioClip::tell_func *tell;
  SoVRMLAudioClip::close_func *close;

  void * callbackuserdataptr;

  void loadUrl(void);
  void unloadUrl(void);

  void startPlaying();
  void stopPlaying();

  static SbBool simageVersionOK(const char *functionName);

  SoFieldSensor * urlsensor;
  SoFieldSensor * loopsensor;
  SoFieldSensor * startTimeSensor;
  SoFieldSensor * stopTimeSensor;
  SoTimerSensor * timerSensor;

  class StaticData {
  public:
    StaticData(void)
      : pauseBetweenTracks(2.0)
      , introPause(0.0)
      , defaultTimerInterval(0.1f)
      , defaultSampleRate(44100)
      , warnAboutMissingSimage(TRUE)
    {
    }
    SbStringList subdirectories;
    SbTime pauseBetweenTracks;
    SbTime introPause;
    SbTime defaultTimerInterval;
    int defaultSampleRate;
    SbBool warnAboutMissingSimage;
  };

  static StaticData * staticdata;
  
  int sampleRate;

  SbTime currentPause;

  SbBool openFile(int playlistIndex);
  SbBool openFile(const char *filename);
  void closeFile();

  s_stream * stream;

  int channels;
  int bitspersample;

  SbList<SbString> playlist;
  volatile SbBool playlistDirty;
  volatile int currentPlaylistIndex;

  SbBool loop;
  volatile SbBool soundHasFinishedPlaying;

#ifdef HAVE_THREADS
  SbMutex syncmutex;
#endif

  SbTime actualStartTime;
  int totalNumberOfFramesToPlay;
};

SoVRMLAudioClipP::StaticData * SoVRMLAudioClipP::staticdata = NULL;

static void
cleanup_audioclip(void)
{
  delete SoVRMLAudioClipP::staticdata;
}

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

SO_NODE_SOURCE(SoVRMLAudioClip);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLAudioClip::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLAudioClip, SO_VRML97_NODE_TYPE);
  SoVRMLAudioClipP::staticdata = new SoVRMLAudioClipP::StaticData;
  coin_atexit((coin_atexit_f*) cleanup_audioclip, CC_ATEXIT_NORMAL);

  const char * env = coin_getenv("COIN_SOUND_INTRO_PAUSE");
  float intropause = env ? (float) atof(env) : 0.0f;
  SoVRMLAudioClip::setDefaultIntroPause(intropause);
}

/*!
  Constructor.
*/
SoVRMLAudioClip::SoVRMLAudioClip(void)
{
  // This is done to trigger the operation which sets up
  // coin_sound_should_traverse() (which, when TRUE informs
  // SoSceneManager that it should start applying an
  // SoAudioRenderAction on its scene graphs).
  //
  // Note: even though SoAudioDevice::instance() is called further
  // below in this constructor, keep this here right at the top, so we
  // don't end up by chance without any call, in case the one(s) below
  // are later removed.
  (void)SoAudioDevice::instance();

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLAudioClip);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(description, (""));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(loop, (FALSE));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(pitch, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(startTime, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(stopTime, (0.0f));
  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(url);

  SO_VRMLNODE_ADD_EVENT_OUT(duration_changed);
  SO_VRMLNODE_ADD_EVENT_OUT(isActive);

  this->isActive.setValue(FALSE);

  PRIVATE(this) = new SoVRMLAudioClipP(this);

  PRIVATE(this)->urlsensor = new SoFieldSensor(PRIVATE(this)->urlSensorCBWrapper, PRIVATE(this));
  PRIVATE(this)->urlsensor->setPriority(0);
  PRIVATE(this)->urlsensor->attach(&this->url);

  PRIVATE(this)->loopsensor = new SoFieldSensor(PRIVATE(this)->loopSensorCBWrapper, PRIVATE(this));
  PRIVATE(this)->loopsensor->setPriority(0);
  PRIVATE(this)->loopsensor->attach(&this->loop);

  PRIVATE(this)->startTimeSensor = new SoFieldSensor(PRIVATE(this)->startTimeSensorCBWrapper,
                                            PRIVATE(this));
  PRIVATE(this)->startTimeSensor->setPriority(0);
  PRIVATE(this)->startTimeSensor->attach(&this->startTime);

  PRIVATE(this)->stopTimeSensor = new SoFieldSensor(PRIVATE(this)->stopTimeSensorCBWrapper,
                                           PRIVATE(this));
  PRIVATE(this)->stopTimeSensor->setPriority(0);
  PRIVATE(this)->stopTimeSensor->attach(&this->stopTime);

  PRIVATE(this)->timerSensor = new SoTimerSensor;
  PRIVATE(this)->timerSensor->setFunction(PRIVATE(this)->timerCBWrapper);
  PRIVATE(this)->timerSensor->setData(PRIVATE(this));
  PRIVATE(this)->timerSensor->setInterval(SoVRMLAudioClipP::staticdata->defaultTimerInterval);
  PRIVATE(this)->timerSensor->schedule();

  PRIVATE(this)->loop = FALSE;
  PRIVATE(this)->soundHasFinishedPlaying = FALSE;

  PRIVATE(this)->stream = NULL;

  PRIVATE(this)->channels = 0;
  PRIVATE(this)->bitspersample = 0;

  PRIVATE(this)->currentPlaylistIndex = 0;
  PRIVATE(this)->playlistDirty = FALSE;

  PRIVATE(this)->sampleRate = SoVRMLAudioClipP::staticdata->defaultSampleRate;

  this->setCallbacks(PRIVATE(this)->internal_open_wrapper,
                     PRIVATE(this)->internal_read_wrapper,
                     PRIVATE(this)->internal_seek_wrapper,
                     PRIVATE(this)->internal_tell_wrapper,
                     PRIVATE(this)->internal_close_wrapper,
                     PRIVATE(this));

  PRIVATE(this)->actualStartTime = 0.0f;
  PRIVATE(this)->totalNumberOfFramesToPlay = 0;
}

/*!
  Destructor.
*/
SoVRMLAudioClip::~SoVRMLAudioClip()
{
  PRIVATE(this)->timerSensor->unschedule();
  delete PRIVATE(this)->timerSensor;

  PRIVATE(this)->unloadUrl();

  delete PRIVATE(this)->urlsensor;
  delete PRIVATE(this)->loopsensor;
  delete PRIVATE(this)->startTimeSensor;
  delete PRIVATE(this)->stopTimeSensor;
  delete PRIVATE(this);
}

void
SoVRMLAudioClip::setDefaultSampleRate(int samplerate)
{
  SoVRMLAudioClipP::staticdata->defaultSampleRate = samplerate;
}

int 
SoVRMLAudioClip::getDefaultSampleRate(void)
{
  return SoVRMLAudioClipP::staticdata->defaultSampleRate;
}

void
SoVRMLAudioClip::setDefaultPauseBetweenTracks(SbTime pause)
{
  // FIXME: use both default and node-specific pause. 20021007 thammer.
  SoVRMLAudioClipP::staticdata->pauseBetweenTracks = pause;
}

SbTime 
SoVRMLAudioClip::getDefaultPauseBetweenTracks(void)
{
  return SoVRMLAudioClipP::staticdata->pauseBetweenTracks;
}

void
SoVRMLAudioClip::setDefaultIntroPause(SbTime pause)
{
  SoVRMLAudioClipP::staticdata->introPause = pause;
}

SbTime 
SoVRMLAudioClip::getDefaultIntroPause(void)
{
  return SoVRMLAudioClipP::staticdata->introPause;
}

void
SoVRMLAudioClip::setDefaultTimerInterval(SbTime interval)
{
  SoVRMLAudioClipP::staticdata->defaultTimerInterval = interval;
}

SbTime 
SoVRMLAudioClip::getDefaultTimerInterval(void)
{
  return SoVRMLAudioClipP::staticdata->defaultTimerInterval;
}

int
SoVRMLAudioClip::getSampleRate()
{
  return PRIVATE(this)->sampleRate;
}

/*! Sets callbacks for opening, reading, seeking, telling and closing
 an audio source. Specifying NULL for a function is OK, except for the
 read function. If a function set to NULL is later called, a default
 implementation doing nothing is called in its place. */

void
SoVRMLAudioClip::setCallbacks(open_func *opencb, read_func *readcb, 
                              seek_func *seekcb, tell_func *tellcb, 
                              close_func *closecb, void *userdataptr)
{
  PRIVATE(this)->open = opencb;
  PRIVATE(this)->read = readcb;
  PRIVATE(this)->seek = seekcb;
  PRIVATE(this)->tell = tellcb;
  PRIVATE(this)->close = closecb;
  PRIVATE(this)->callbackuserdataptr = userdataptr;
}

/*!  Opens an audio source with the given \a url. Returns a handle to
  the datasource.  */
void * 
SoVRMLAudioClip::open(const SbStringList &urlref)
{
  if (PRIVATE(this)->open == NULL)
    return NULL;

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  return PRIVATE(this)->open(urlref, this, PRIVATE(this)->callbackuserdataptr);
}

/*! Moves the "filepointer" in the \a datasource, returns -1L on error.  
*/

int
SoVRMLAudioClip::seek(void *datasource, long offset, int whence)
{
  if (PRIVATE(this)->seek == NULL)
    return -1;

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  return PRIVATE(this)->seek(datasource, offset, whence,
                             this, PRIVATE(this)->callbackuserdataptr);
}

/*! Returns the current position of the "filepointer" in the \a datasource, 
    or -1L on error.  
*/

long
SoVRMLAudioClip::tell(void *datasource)
{
  if (PRIVATE(this)->tell == NULL)
    return -1L;

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  return PRIVATE(this)->tell(datasource, 
                             this, PRIVATE(this)->callbackuserdataptr);
}

/*! Closes \a datasource.
 */

int
SoVRMLAudioClip::close(void *datasource)
{
  if (PRIVATE(this)->close == NULL)
    return EOF;

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  return PRIVATE(this)->close(datasource, 
                              this, PRIVATE(this)->callbackuserdataptr);
}

/*!  Reads \a numframes frames of audio with \a channels channels from
     \a datasource into \a buffer. Buffer must be allocated by the
     caller, and must be able to hold all the audio data (size = \a
     numframes * \a channels * sizeof(int16_t)). The function must
     always fill the buffer completely unless \a buffer == NULL. 

     When an error occurs, or when end-of-file has been reached, 
     this function returns 0. Otherwise, the function should return
     \a numframes.

     When the caller receives a return value of 0, it will queue the
     returned buffer for playing. When this buffer is finished playing,
     the caller will call read() one final time, with \a buffer == NULL. 
     The read() function can then set the isActive field to FALSE, 
     free any resources allocated, etc.
*/

size_t
SoVRMLAudioClip::read(void *datasource, void *buffer,
                      int numframes, int &channels)
{
  assert (PRIVATE(this)->read != NULL);

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  if (PRIVATE(this)->actualStartTime == 0.0f)
    PRIVATE(this)->actualStartTime = SbTime::getTimeOfDay();
  size_t ret;
  ret = PRIVATE(this)->read(datasource, buffer,
                            numframes, channels,
                            this, PRIVATE(this)->callbackuserdataptr);
  if (ret != 0) {
    PRIVATE(this)->totalNumberOfFramesToPlay += numframes;
  }

  return ret;
}

void
SoVRMLAudioClip::setSubdirectories(const SbList<SbString> &subdirectories)
{
  int i;
  for (i = 0; i < SoVRMLAudioClipP::staticdata->subdirectories.getLength(); i++) {
    delete SoVRMLAudioClipP::staticdata->subdirectories[i];
  }
  for (i = 0; i < subdirectories.getLength(); i++) {
    SoVRMLAudioClipP::staticdata->subdirectories.append(new SbString(subdirectories[i]));
  }
}

const SbStringList &
SoVRMLAudioClip::getSubdirectories()
{
  return SoVRMLAudioClipP::staticdata->subdirectories;
}

SbBool
SoVRMLAudioClipP::simageVersionOK(const char *functionName)
{
  if (simage_wrapper()->available &&
      simage_wrapper()->versionMatchesAtLeast(1,4,0) &&
      simage_wrapper()->s_stream_open &&
      simage_wrapper()->s_stream_get_buffer &&
      simage_wrapper()->s_stream_params &&
      simage_wrapper()->s_params_get &&
      simage_wrapper()->s_stream_close &&
      simage_wrapper()->s_stream_destroy) {
      return TRUE;
  } else {
    if (SoVRMLAudioClipP::staticdata->warnAboutMissingSimage) {
      SoDebugError::postWarning(functionName,
                                "This function needs a version of simage that supports"
                                "the stream interface and parameter access to be able "
                                "to read audio files. Please visit https://github.com/coin3d/ "
                                "and download the latest version of simage.");
      SoVRMLAudioClipP::staticdata->warnAboutMissingSimage = FALSE;
    }
    return FALSE;
  }
}

void
SoVRMLAudioClipP::startPlaying()
{
#if COIN_DEBUG && DEBUG_AUDIO
  SoDebugError::postInfo("SoVRMLAudioClipP::startPlaying", "start");
#endif // debug
#ifdef HAVE_THREADS
  this->syncmutex.lock();
#endif
  this->currentPause = SoVRMLAudioClipP::staticdata->introPause;
  this->currentPlaylistIndex = 0;
  this->soundHasFinishedPlaying = FALSE;
  this->actualStartTime = 0.0f; // will be set in read()
  this->totalNumberOfFramesToPlay = 0; // will be increased in read()
#ifdef HAVE_THREADS
  this->syncmutex.unlock();
#endif
  PUBLIC(this)->isActive.setValue(TRUE);
}

void
SoVRMLAudioClipP::stopPlaying()
{
#if COIN_DEBUG && DEBUG_AUDIO
  SoDebugError::postInfo("SoVRMLAudioClipP::stopPlaying", "stop");
#endif // debug
  PUBLIC(this)->isActive.setValue(FALSE);
#ifdef HAVE_THREADS
  this->syncmutex.lock();
#endif
  /*
    FIXME: If the stream is closed here, and read() is called
    before the sound figures out it should also stop playing,
    read() might try to open the next file (or reopen the
    existing file if loop==TRUE) and we might get an "echo-effect".
    We should perhaps keep an internal isActive that is "always"
    equal to the external isActive, although this should
    be synchronized, so read() can check it safely, and
    decide to not open a file if it is FALSE.
    Investigate this further.
    2002-11-15 thammer.
   */

  this->closeFile();
#ifdef HAVE_THREADS
  this->syncmutex.unlock();
#endif
}

void *
SoVRMLAudioClipP::internal_open_wrapper(const SbStringList &url,
                               SoVRMLAudioClip *clip, void *userdataptr)
{
  SoVRMLAudioClipP *pthis = (SoVRMLAudioClipP *)userdataptr;
  return pthis->internal_open(url, clip);
}

size_t
SoVRMLAudioClipP::internal_read_wrapper(void *datasource, void *buffer, 
                               int numframes, int &channels,
                               SoVRMLAudioClip *clip, void *userdataptr)
{
  SoVRMLAudioClipP *pthis = (SoVRMLAudioClipP *)userdataptr;
  return pthis->internal_read(datasource, buffer, numframes, channels, clip);
}

int   
SoVRMLAudioClipP::internal_seek_wrapper(void *datasource, long offset, 
                                        int whence, SoVRMLAudioClip *clip, 
                                        void *userdataptr)
{
  SoVRMLAudioClipP *pthis = (SoVRMLAudioClipP *)userdataptr;
  return pthis->internal_seek(datasource, offset, whence, clip);
}

long  
SoVRMLAudioClipP::internal_tell_wrapper(void *datasource,
                               SoVRMLAudioClip *clip, void *userdataptr)
{
  SoVRMLAudioClipP *pthis = (SoVRMLAudioClipP *)userdataptr;
  return pthis->internal_tell(datasource, clip);
}

int   
SoVRMLAudioClipP::internal_close_wrapper(void *datasource,
                                SoVRMLAudioClip *clip, void *userdataptr)
{
  SoVRMLAudioClipP *pthis = (SoVRMLAudioClipP *)userdataptr;
  return pthis->internal_close(datasource, clip);
}

void * 
SoVRMLAudioClipP::internal_open(const SbStringList & COIN_UNUSED_ARG(url), 
                                SoVRMLAudioClip * COIN_UNUSED_ARG(clip))
{
  return NULL;
}

size_t
SoVRMLAudioClipP::internal_read(void * COIN_UNUSED_ARG(datasource), void *buffer, int numframes,
                                int &channelsref, SoVRMLAudioClip * COIN_UNUSED_ARG(clip))
{
  // 20021007 thammer note: this method might be called from a thread
  // different from the thread which created the "main" Coin thread.

  /* FIXME: We should really support different sampling rates and
     bitspersample.  I think it should be the AudioClip's
     responsibility to resample if necessary. 20021007 thammer.  */

  /* FIXME: Opening a file might take some CPU time, so we should
     perhaps try doing this in non-critical places. Such as when url
     changes. Perhaps we should even open multiple files when url
     changes. This _might_ improve the current problem we have with
     possible stuttering at the beginning of playing a buffer...
     20021007 thammer.  */

  if (!this->simageVersionOK("SoVRMLAudioClipP::internal_read")) {
    int outputsize = numframes * 1 * sizeof(int16_t);
    memset(buffer, 0, outputsize);
    channelsref=1;
    return 0;
  }

  if (buffer == NULL) {
    /* Note: The SoVRMLSound node has signalled that it has received
       an eof previously sent by this SoVRMLAudioClip, and it has
       played all buffers including the last one it received. This is
       a pretty good indicator that this SoVRMLAudioClip can stop
       playing. 2002-11-06 thammer.  */
    this->soundHasFinishedPlaying = TRUE;
    return 0;
  }

  assert (!this->soundHasFinishedPlaying);

  SbBool bufferFilled = FALSE;
  int framepos = 0;
  int channelsdelivered = 1;
  while (!bufferFilled) {
    if (this->currentPause>0.0) {
      // deliver a zero'ed,  buffer
      size_t outputsize = size_t(numframes - framepos) * size_t(channelsdelivered) *
        sizeof(int16_t);
      memset(((int16_t *)buffer) + framepos*channelsdelivered, 0, outputsize);
      this->currentPause -= (double)(numframes - framepos) / 
        (double)SoVRMLAudioClipP::staticdata->defaultSampleRate;
      channelsref = channelsdelivered;

      return numframes;
    }

    if (this->playlistDirty) {
      this->playlistDirty = FALSE;
      this->closeFile();
      this->currentPlaylistIndex = 0;
    }

    if (this->playlist.getLength() == 0) {
      this->closeFile();
      size_t outputsize = size_t(numframes - framepos) * size_t(channelsdelivered) *
        sizeof(int16_t);
      memset(((int16_t *)buffer) + framepos*channelsdelivered, 0, 
              outputsize);
      channelsref = channelsdelivered;
      return 0; // signal that we're done playing
    }

    /* FIXME: Read the VRML spec on the url field in AudioClip more
       carefully. I think it's only supposed to play one file, and
       only try the next if the current file fails.
       2003-01-16 thammer. */

    if (this->stream==NULL) {
      if (this->currentPlaylistIndex >= this->playlist.getLength()) {
        if (this->loop)
          this->currentPlaylistIndex = 0;
        else {
          // We have played all files in the list, and we're not looping.
          // => We can stop playing.
          size_t outputsize = size_t(numframes - framepos) * size_t(channelsdelivered) *
            sizeof(int16_t);
          memset(((int16_t *)buffer) + framepos*channelsdelivered, 0, 
                 outputsize);
          channelsref = channelsdelivered;
          return 0; // signal that eof has been reached
        }
      }

      int startindex = this->currentPlaylistIndex;
      SbBool success = FALSE;
      SbBool allfailed = FALSE;
      while ( ! ( success || allfailed ) ) {
        success = openFile(this->currentPlaylistIndex);
        if (!success) {
          this->currentPlaylistIndex++;
          if ( this->loop && 
               (this->currentPlaylistIndex >= this->playlist.getLength()))
            this->currentPlaylistIndex = 0;
          if ( (this->currentPlaylistIndex == startindex) ||
               (this->currentPlaylistIndex >= this->playlist.getLength()) )
            allfailed = TRUE;
        }
      }

      if (!success) {
        size_t outputsize = size_t(numframes - framepos) * size_t(channelsdelivered) * 
          sizeof(int16_t);
        memset(((int16_t *)buffer) + framepos*channelsdelivered, 0, 
               outputsize);
        channelsref = channelsdelivered;
        return 0;
      }
    }

    assert(this->stream!=NULL);
    assert(bitspersample == sizeof(int16_t) * 8);

    channelsdelivered = this->channels;
    int inputsize = (numframes - framepos) * this->channels * 
      this->bitspersample / 8;
    int numread = inputsize;

    simage_wrapper()->s_stream_get_buffer(this->stream,
                                          ((int16_t *)buffer) + 
                                          framepos*channelsdelivered,
                                          &numread, NULL);

    channelsref = this->channels;

    /* FIXME: If numread==0 and we've just opened the file, we should
       return with an error instead of doing an infinite loop (duh!).
       2003-02-25 thammer.
     */

    if (numread != inputsize) {
      closeFile();
      framepos += (numread / (this->channels * this->bitspersample / 8));

      this->currentPlaylistIndex++;
      // FIXME: Used to use the following check in the conditional
      // below. Investigate if this should still be used.
      // if ( (this->currentPlaylistIndex<this->playlist.getLength()) &&
      //      this->loop)
      // 2005-05-25 thammer.
      if (this->playlist.getLength() > 1)
        this->currentPause = SoVRMLAudioClipP::staticdata->pauseBetweenTracks;
    } else {
      bufferFilled = TRUE;
    }
  }
  return numframes;
}

int    
SoVRMLAudioClipP::internal_seek(void * COIN_UNUSED_ARG(datasource), long COIN_UNUSED_ARG(offset), int whence,
                       SoVRMLAudioClip *clip)
{
  return -1;
}
long   
SoVRMLAudioClipP::internal_tell(void * COIN_UNUSED_ARG(datasource), SoVRMLAudioClip * COIN_UNUSED_ARG(clip))
{
  return -1;
}

int    
SoVRMLAudioClipP::internal_close(void * COIN_UNUSED_ARG(datasource), SoVRMLAudioClip * COIN_UNUSED_ARG(clip))
{
  return 0;
}

void
SoVRMLAudioClipP::loadUrl()
{
#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&this->syncmutex);
#endif
  this->unloadUrl();

  for (int i=0; i<PUBLIC(this)->url.getNum(); i++) {
    const char * str = PUBLIC(this)->url[i].getString();
    if (!str || str[0] == '\0')
      continue; // ignore empty url

    SbString filename =
      SoInput::searchForFile(SbString(str), SoInput::getDirectories(),
                             SoVRMLAudioClip::getSubdirectories());

    if (filename.getLength() <= 0) {
      SoDebugError::postWarning("SoVRMLAudioClipP::loadUrl(index)",
                                "File not found: '%s'",
                                str);
      continue; // ignore invalid file
    }

    this->playlist.append(filename);
  }
}

void
SoVRMLAudioClipP::unloadUrl()
{
  this->playlistDirty = TRUE;
  this->playlist.truncate(0);
  this->closeFile();
}

//
// called when filename changes
//
void
SoVRMLAudioClipP::urlSensorCBWrapper(void * data, SoSensor *)
{
  SoVRMLAudioClipP * thisp = (SoVRMLAudioClipP*) data;
  thisp->urlSensorCB(NULL);
}

//
// called when filename changes
//
void
SoVRMLAudioClipP::urlSensorCB(SoSensor *)
{
  this->loadUrl();
}

//
// called when loop changes
//
void
SoVRMLAudioClipP::loopSensorCBWrapper(void * data, SoSensor *)
{
  SoVRMLAudioClipP * thisp = (SoVRMLAudioClipP*) data;
  thisp->loopSensorCB(NULL);
}

//
// called when loop changes
//
void
SoVRMLAudioClipP::loopSensorCB(SoSensor *)
{
#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&this->syncmutex);
#endif
  this->loop = PUBLIC(this)->loop.getValue();
}

//
// called when startTime changes
//
void
SoVRMLAudioClipP::startTimeSensorCBWrapper(void * data, SoSensor *)
{
  SoVRMLAudioClipP * thisp = (SoVRMLAudioClipP*) data;
  thisp->startTimeSensorCB(NULL);
}

//
// called when startTime changes
//
void
SoVRMLAudioClipP::startTimeSensorCB(SoSensor *)
{
  SbTime now = SbTime::getTimeOfDay();
  SbTime start = PUBLIC(this)->startTime.getValue();

  if (now>=start) {
    if (!PUBLIC(this)->isActive.getValue())
      this->startPlaying();
  }
}

//
// called when stopTime changes
//
void
SoVRMLAudioClipP::stopTimeSensorCBWrapper(void * data, SoSensor *)
{
  SoVRMLAudioClipP * thisp = (SoVRMLAudioClipP*) data;
  thisp->stopTimeSensorCB(NULL);
}

//
// called when stopTime changes
//
void
SoVRMLAudioClipP::stopTimeSensorCB(SoSensor *)
{
  SbTime now = SbTime::getTimeOfDay();
  SbTime start = PUBLIC(this)->startTime.getValue();
  SbTime stop = PUBLIC(this)->stopTime.getValue();

  if ( (now>=stop) && (stop>start) )
  {
    // we shouldn't be playing now
    if  (PUBLIC(this)->isActive.getValue())
      this->stopPlaying();
    return;
  }
}
//
// checks current time to see if we should start or stop playing
//
void
SoVRMLAudioClipP::timerCBWrapper(void * data, SoSensor *)
{
  SoVRMLAudioClipP * thisp = (SoVRMLAudioClipP*) data;
  thisp->timerCB(NULL);
}

//
// checks current time to see if we should start or stop playing
//
void
SoVRMLAudioClipP::timerCB(SoSensor *)
{
  SbTime now = SbTime::getTimeOfDay();
  SbTime start = PUBLIC(this)->startTime.getValue();
  SbTime stop = PUBLIC(this)->stopTime.getValue();

#if COIN_DEBUG && DEBUG_AUDIO
  SbString start_str = start.format("%D %h %m %s");
  SbString stop_str = stop.format("%D %h %m %s");
  SbString now_str = now.format("%D %h %m %s");
#endif // debug

#if COIN_DEBUG && DEBUG_AUDIO
  SoDebugError::postInfo("SoVRMLAudioClipP::timerCB", "(timerCB)");
#endif // debug

  if (((now>=stop) && (stop>start)) ||
      (! SoAudioDevice::instance()->haveSound()) ||
      (! SoAudioDevice::instance()->isEnabled()))
  {
    // we shouldn't be playing now
    if  (PUBLIC(this)->isActive.getValue())
      this->stopPlaying();
    return;
  }

  // if we got this far, ( (now<stop) || (stop<=start) )
  if (this->soundHasFinishedPlaying) {
    if  (PUBLIC(this)->isActive.getValue()) {
      // FIXME: perhaps add some additional slack, the size of one buffer? 20021008 thammer.
#if COIN_DEBUG && DEBUG_AUDIO
      SoDebugError::postInfo("SoVRMLAudioClipP::timerCB", "soundHasFinishedPlaying");
#endif // debug
      this->stopPlaying();
    }
    return;
  }

  if (now>=start) {
    if (!PUBLIC(this)->isActive.getValue())
      this->startPlaying();
  }
}

SbBool
SoVRMLAudioClipP::openFile(int playlistIndex)
{
  assert ( (playlistIndex<this->playlist.getLength()) && (playlistIndex>=0) );

  return this->openFile(this->playlist[playlistIndex].getString());
}

SbBool
SoVRMLAudioClipP::openFile(const char *filename)
{
  this->closeFile();

  if (!this->simageVersionOK("SoVRMLAudioClipP::openFile")) {
    return FALSE;
  }

  // FIXME: only looks in current directory -- should use the full set
  // of SoInput::getDirectories() (make a copy of the returned
  // SbStringList from when the SoVRMLAudioClip was read).  20050113 mortene.
  //
  // FIXME: this is attempted again and again when the file cannot be
  // opened. Once should be sufficient, and subsequent attempts should
  // be short-cutted somewhere before this in the call-chain. 20050627 mortene.
  this->stream = simage_wrapper()->s_stream_open(filename, NULL);
  if (this->stream == NULL) {
    // FIXME: sound should stop playing.  20021101 thammer
    SoDebugError::postWarning("SoVRMLAudioClipP::openFile",
                              "Couldn't open file '%s'.\n"
                              "Here's some advice for debugging:\n\n"
                              "Audio data is loaded using the \"simage\" library. "
                              "Make sure you have\n"
                              "built the simage library with support for the audio file formats you\n"
                              "intend to use. Simage needs the libraries libogg, libvorbis and \n"
                              "libvorbisfile for the OggVorbis audio file format, and the\n"
                              "libsndfile library for WAV and several other formats (visit\n"
                              "http://www.mega-nerd.com/libsndfile/ for a complete list of supported\n"
                              "file formats).\n\n"
                              "Verify that you have the necessary access rights for reading the\n"
                              "audio file.\n\n"
                              , filename);
    return FALSE;
  }

  s_params * params;
  params = simage_wrapper()->s_stream_params(stream);

  this->channels = 0;
  this->bitspersample = 16;
  int samplerate = 0;
  if (params != NULL) {
    simage_wrapper()->s_params_get(params,
                 "channels", S_INTEGER_PARAM_TYPE, &this->channels, NULL);
    simage_wrapper()->s_params_get(params,
                 "samplerate", S_INTEGER_PARAM_TYPE, &samplerate, NULL);
  }

#if COIN_DEBUG && DEBUG_AUDIO
  SoDebugError::postInfo("SoVRMLAudioClipP::openFile",
                         "Wave file '%s' opened successfully\n", filename);
#endif // debug

  return TRUE; // OK
}

void
SoVRMLAudioClipP::closeFile()
{
  if (this->stream != NULL) {
    if (!this->simageVersionOK("SoVRMLAudioClipP::closeFile")) {
      return;
    } else {
      simage_wrapper()->s_stream_close(this->stream);
      simage_wrapper()->s_stream_destroy(this->stream);
    }
    this->stream = NULL;
  }
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
