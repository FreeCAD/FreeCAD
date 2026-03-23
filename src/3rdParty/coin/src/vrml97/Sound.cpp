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
  \class SoVRMLSound SoVRMLSound.h Inventor/VRMLnodes/SoVRMLSound.h
  \brief The SoVRMLSound class is used to represent a sound source.

  \ingroup coin_VRMLnodes
  \ingroup coin_sound

  \WEB3DCOPYRIGHT

  \verbatim
  Sound {
    exposedField SFVec3f  direction     0 0 1   # (-inf, inf)
    exposedField SFFloat  intensity     1       # [0,1]
    exposedField SFVec3f  location      0 0 0   # (-inf, inf)
    exposedField SFFloat  maxBack       10      # [0,inf)
    exposedField SFFloat  maxFront      10      # [0,inf)
    exposedField SFFloat  minBack       1       # [0,inf)
    exposedField SFFloat  minFront      1       # [0,inf)
    exposedField SFFloat  priority      0       # [0,1]
    exposedField SFNode   source        NULL
    field        SFBool   spatialize    TRUE
  }
  \endverbatim

  The Sound node specifies the spatial presentation of a sound in a
  VRML scene. The sound is located at a point in the local coordinate
  system and emits sound in an elliptical pattern (defined by two
  ellipsoids). The ellipsoids are oriented in a direction specified by
  the direction field. The shape of the ellipsoids may be modified to
  provide more or less directional focus from the location of the
  sound.

  The source field specifies the sound source for the Sound node. If
  the source field is not specified, the Sound node will not emit
  audio. The source field shall specify either an AudioClip node or a
  MovieTexture node. If a MovieTexture node is specified as the sound
  source, the MovieTexture shall refer to a movie format that supports
  sound (e.g., MPEG1-Systems).

  The intensity field adjusts the loudness (decibels) of the sound
  emitted by the Sound node (note: this is different from the
  traditional definition of intensity with respect to sound).

  The intensity field has a value that ranges from 0.0 to 1.0 and
  specifies a factor which shall be used to scale the normalized
  sample data of the sound source during playback. A Sound node with
  an intensity of 1.0 shall emit audio at its maximum loudness (before
  attenuation), and a Sound node with an intensity of 0.0 shall emit
  no audio. Between these values, the loudness should increase
  linearly from a -20 dB change approaching an intensity of 0.0 to a 0
  dB change at an intensity of 1.0.

  The priority field provides a hint for the browser to choose which
  sounds to play when there are more active Sound nodes than can be
  played at once due to either limited system resources or system
  load. 7.3.4, Sound priority, attenuation, and spatialization
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#7.3.4>),
  describes a recommended algorithm for determining which sounds to
  play under such circumstances. The priority field ranges from 0.0 to
  1.0, with 1.0 being the highest priority and 0.0 the lowest
  priority.

  The location field determines the location of the sound emitter in
  the local coordinate system. A Sound node's output is audible only
  if it is part of the traversed scene. Sound nodes that are descended
  from LOD, Switch, or any grouping or prototype node that disables
  traversal (i.e., drawing) of its children are not audible unless
  they are traversed. If a Sound node is disabled by a Switch or LOD
  node, and later it becomes part of the traversal again, the sound
  shall resume where it would have been had it been playing
  continuously.

  The Sound node has an inner ellipsoid that defines a volume of space
  in which the maximum level of the sound is audible. Within this
  ellipsoid, the normalized sample data is scaled by the intensity
  field and there is no attenuation. The inner ellipsoid is defined by
  extending the direction vector through the location.

  The minBack and minFront fields specify distances behind and in
  front of the location along the direction vector respectively. The
  inner ellipsoid has one of its foci at location (the second focus is
  implicit) and intersects the direction vector at minBack and
  minFront.  The Sound node has an outer ellipsoid that defines a
  volume of space that bounds the audibility of the sound. No sound
  can be heard outside of this outer ellipsoid.

  The outer ellipsoid is defined by extending the direction vector
  through the location. The maxBack and maxFront fields specify
  distances behind and in front of the location along the direction
  vector respectively. The outer ellipsoid has one of its foci at
  location (the second focus is implicit) and intersects the direction
  vector at maxBack and maxFront.  The minFront, maxFront, minBack,
  and maxBack fields are defined in local coordinates, and shall be
  greater than or equal to zero. The minBack field shall be less than
  or equal to maxBack, and minFront shall be less than or equal to
  maxFront.  The ellipsoid parameters are specified in the local
  coordinate system but the ellipsoids' geometry is affected by
  ancestors' transformations.

  Between the two ellipsoids, there shall be a linear attenuation ramp
  in loudness, from 0 dB at the minimum ellipsoid to -20 dB at the
  maximum ellipsoid: attenuation = -20 × (d' / d") where d' is the
  distance along the location-to-viewer vector, measured from the
  transformed minimum ellipsoid boundary to the viewer, and d" is the
  distance along the location-to-viewer vector from the transformed
  minimum ellipsoid boundary to the transformed maximum ellipsoid
  boundary (see Figure 6.14).

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/Sound.gif">
  Figure 6.14 -- Sound node geometry
  </center>

  The spatialize field specifies if the sound is perceived as being
  directionally located relative to the viewer. If the spatialize
  field is TRUE and the viewer is located between the transformed
  inner and outer ellipsoids, the viewer's direction and the relative
  location of the Sound node should be taken into account during
  playback. Details outlining the minimum required spatialization
  functionality can be found in 7.3.4, Sound priority, attenuation,
  and spatialization
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#7.3.4>),
  If the spatialize field is FALSE, then directional effects are
  ignored, but the ellipsoid dimensions and intensity will still
  affect the loudness of the sound.  If the sound source is
  multi-channel (e.g., stereo), then the source should retain its
  channel separation during playback.

*/

/*!
  \var SoSFNode SoVRMLSound::source

  The audio stream. Either an SoVRMLAudioClip or an
  SoVRMLMovieTexture. Coin only supports playback of audio from
  SoVRMLAudioClip, not from SoVRMLMovieTexture.
*/

/*!
  \var SoSFFloat SoVRMLSound::intensity
  Sound intensity. A value from 0 to 1. Default value is 1.
*/

/*!
  \var SoSFFloat SoVRMLSound::priority
  Browser hint for how important the sound is. A value from 0 to
  1. Default value is 0. Coin does not yet support this field.  
*/

/*!
  \var SoSFVec3f SoVRMLSound::location
  The sound position. Default value is (0, 0, 0).
*/

/*!
  \var SoSFVec3f SoVRMLSound::direction
  Sound direction. Default value is (0, 0, 1). 
*/

/*!
  \var SoSFFloat SoVRMLSound::minFront
  Inner ellipse front value. Default value is 1.
*/

/*!
  \var SoSFFloat SoVRMLSound::maxFront
  Outer ellipse front value. Default value is 10. 
*/

/*!
  \var SoSFFloat SoVRMLSound::minBack
  Inner ellipse back value. Default value is 1. 
*/

/*!
  \var SoSFFloat SoVRMLSound::maxBack
  Outer ellips back value. Default value is 10. 
*/

/*!
  \var SoSFBool SoVRMLSound::spatialize
  Set to TRUE if sound should be spatialized (directional effects 
  are applied) with respect to the viewer. Distance attenuation is 
  always applied. Default value is TRUE.  
*/

#include <Inventor/VRMLnodes/SoVRMLSound.h>
#include "coindefs.h"

#include <cstddef>

#include <Inventor/VRMLnodes/SoVRMLAudioClip.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoListenerPositionElement.h>
#include <Inventor/elements/SoListenerOrientationElement.h>
#include <Inventor/elements/SoListenerDopplerElement.h>
#include <Inventor/elements/SoListenerGainElement.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/misc/SoAudioDevice.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbTime.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbCondVar.h>
#include <Inventor/threads/SbMutex.h>
#include <Inventor/threads/SbThreadAutoLock.h>
#include <Inventor/C/threads/thread.h>
#endif // HAVE_THREADS

#include "misc/AudioTools.h"
#include "glue/openal_wrapper.h"
#include "nodes/SoSubNodeP.h"

// *************************************************************************

#define DEBUG_AUDIO 0

// *************************************************************************

class SoVRMLSoundP
{
public:
  SoVRMLSoundP(SoVRMLSound * master) : master(master) {};
  SoVRMLSound *master;

  static void sourceSensorCBWrapper(void *, SoSensor *);
  void sourceSensorCB(SoSensor *);

  SbBool stopPlaying();
  SbBool startPlaying();

  static void timercb(void * data, SoSensor *);

  static void * threadCallbackWrapper(void *userdata);
  void * threadCallback();
  void fillBuffers();

  void deleteAlBuffers();

  void generateAlSource();
  void deleteAlSource();
  SbBool hasValidAlSource();

  SoFieldSensor * sourcesensor;

  unsigned int sourceId;
  SbList<unsigned int> alBuffers;

  SoVRMLAudioClip *currentAudioClip;
  SbBool playing;
  SbBool useTimerCallback;
  SbBool endoffile;
  SbBool waitingForAudioClipToFinish;

  SoTimerSensor * timersensor;
#ifdef HAVE_THREADS
  cc_thread *workerThread;
  SbMutex syncmutex;
  SbMutex exitthreadmutex;
  SbCondVar exitthreadcondvar;
#endif
  volatile SbBool exitthread;
  volatile SbBool errorInThread;

  int16_t *audioBuffer;
  int channels;
  SbTime sleepTime;
  SbTime workerThreadSleepTime;

  static int defaultBufferLength;
  static int defaultNumBuffers;
  static double defaultSleepTime;

  int bufferLength; // bytesize = bufferLength*bitspersample/8*channels
  int numBuffers;

  void *cliphandle;
};

#define PRIVATE(p) ((p)->pimpl)
#define PUBLIC(p) ((p)->master)

int SoVRMLSoundP::defaultBufferLength = 44100/10;
int SoVRMLSoundP::defaultNumBuffers = 5;
double SoVRMLSoundP::defaultSleepTime = 0.100; // 100ms

SO_NODE_SOURCE(SoVRMLSound);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLSound::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLSound, SO_VRML97_NODE_TYPE);
  SoAudioRenderAction::addMethod(SoVRMLSound::getClassTypeId(),
                                 SoNode::audioRenderS);

  /* Note: The default buffersize is currently set to 4096*10. This is
     because the Linux version of OpenAL currently in CVS at
     www.openal.org is slightly buggy when it comes to buffer
     handling, and for mysterious reasons, if the buffer size is a
     multiple of 4096, everything works almost as it should.  The
     problem (and this quick-fix) has been acknowledged by the guy in
     charge of the Linux version of OpenAL, and it is being worked
     at. 2003-03-10 thammer */
  const char * env = coin_getenv("COIN_SOUND_BUFFER_LENGTH");
  int bufferlength = env ? atoi(env) : 40960;

  env = coin_getenv("COIN_SOUND_NUM_BUFFERS");
  int numbuffers = env ? atoi(env) : 5;

  env = coin_getenv("COIN_SOUND_THREAD_SLEEP_TIME");
  float threadsleeptime = env ? (float) atof(env) : 0.250f;

  SoVRMLSound::setDefaultBufferingProperties(bufferlength, numbuffers, threadsleeptime);
}

/*!
  Constructor.
*/
SoVRMLSound::SoVRMLSound(void)
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

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLSound);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(source, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(intensity, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(priority, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(location, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(direction, (0.0f, 0.0f, 1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(minFront, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(maxFront, (10.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(minBack, (1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(maxBack, (10.0f));

  SO_VRMLNODE_ADD_FIELD(spatialize, (TRUE));

  PRIVATE(this) = new SoVRMLSoundP(this);

  PRIVATE(this)->channels = 1;
  // because spatialize defaults to TRUE
  // and OpenAL only spatializes mono buffers

  PRIVATE(this)->currentAudioClip = NULL;
  PRIVATE(this)->playing = FALSE;
  PRIVATE(this)->endoffile = FALSE;
  PRIVATE(this)->waitingForAudioClipToFinish = FALSE;

  PRIVATE(this)->timersensor = NULL;
#ifdef HAVE_THREADS
  /* FIXME: Let the user override use of timer callback with an
     environment variable. 2003-01-16 thammer.  */
  PRIVATE(this)->useTimerCallback = FALSE;
#else
  PRIVATE(this)->useTimerCallback = TRUE;
#endif // HAVE_THREADS

  /* FIXME: if (coin_debug_audio()), post info about which playback
     mode is used, threaded or timer callback. 2003-01-14 thammer */

  PRIVATE(this)->sourcesensor = new SoFieldSensor(
    PRIVATE(this)->sourceSensorCBWrapper, PRIVATE(this));
  PRIVATE(this)->sourcesensor->setPriority(0);
  PRIVATE(this)->sourcesensor->attach(&this->source);

#ifdef HAVE_THREADS
  PRIVATE(this)->workerThread = NULL;
#endif
  PRIVATE(this)->exitthread = FALSE;
  PRIVATE(this)->errorInThread = FALSE;
  PRIVATE(this)->audioBuffer = NULL;
  PRIVATE(this)->bufferLength = 0;

  this->setBufferingProperties(SoVRMLSoundP::defaultBufferLength,
                               SoVRMLSoundP::defaultNumBuffers,
                               SbTime(SoVRMLSoundP::defaultSleepTime));

  PRIVATE(this)->sourceId = 0;

  PRIVATE(this)->cliphandle = NULL;

  static SbBool warningprintedonce = FALSE;

  // FIXME: I believe all this checking with HAVE_SOUND should be
  // unnecessary -- using SoAudioDevice::instance()->haveSound()
  // should be sufficient. This goes for a lot of code in this file
  // (and nowhere else -- after I did some cleaning up). 20050628 mortene.

  // FIXME: the remaining code in this constructor seems to just be
  // more or less a duplicate of the envvar checking, warning
  // messages, and other functionality from
  // SoAudioDevice::init(). Clean up. 20050628 mortene.

#ifdef HAVE_SOUND
  if (!warningprintedonce) {
    if (!SoAudioDevice::instance()->haveSound()) {
      warningprintedonce = TRUE;

      // FIXME: checking support platform and the COIN_SOUND_ENABLE
      // envvar is already done in SoAudioDevice.cpp -- I don't see
      // why it needs to be done again. Clean up. 20050627 mortene.
      SbBool unsupportedplatform = TRUE;
#ifdef _WIN32
      unsupportedplatform = FALSE;
#endif // _WIN32
      SbBool forceenable = FALSE;
      if (unsupportedplatform) {
        const char * env;
        env = coin_getenv("COIN_SOUND_ENABLE");
        if (env && atoi(env))
          forceenable = TRUE;
      }

      if (unsupportedplatform && (!forceenable)) {
        SoDebugError::postWarning("SoVRMLSound::SoVRMLSound",
          "You are using a SoVRMLSound node, but sound support on this "
          "platform is considered experimental and is not enabled by "
          "default. If you'd like to enable sound, set the environment "
          "variable COIN_SOUND_ENABLE=1. "
          SOUND_NOT_ENABLED_BY_DEFAULT_STRING );
      }
      else {
        if (!openal_wrapper()->available) {
          SoDebugError::postWarning("SoVRMLSound::SoVRMLSound",
            "You are using a SoVRMLSound node, but Coin was "
            "unable to link with the OpenAL library. Attempted to use %s "
            "linking. Sound will not be available. "
            "The probable reason for this is that the OpenAL library, "
            "needed for rendering 3D audio, is not installed correctly on "
            "your system. If you'd like to use sounds in Coin, "
            "download the latest "
            "version of OpenAL from www.openal.org [all platforms], "
            "ftp://opensource.creative.com/pub/sdk/ (OpenALWEAX.exe or "
            "OpenALWEAX2.exe) [Windows platform only], or ask the "
            "manufacturer of "
            "your soundcard for a native OpenAL driver (several soundcard"
            "manufacturers offer this).",
            openal_wrapper()->runtime ? "runtime" : "link-time");
          if (openal_wrapper()->runtime) {
            SoDebugError::postInfo("SoVRMLSound::SoVRMLSound",
                                   "To get more debug information, "
                                   "set the environment variable "
                                   "COIN_DEBUG_DL=1 and run the "
                                   "application again");
          }
        }
        else {
          SoDebugError::postWarning("SoVRMLSound::SoVRMLSound",
            "Initialization of the audio device failed. To get more debug "
            "information, set the environment variable COIN_DEBUG_AUDIO=1 "
            "and run the application again.");
        }
      }
    }
  }
#else // !HAVE_SOUND
  if (!warningprintedonce) {
    SoDebugError::postWarning("SoVRMLSound::SoVRMLSound",
      "You are using a SoVRMLSound node, but this version of Coin was built "
      "without sound support. If you'd like to have sound support in Coin, "
      "please reconfigure and rebuild the Coin library without specifying "
      "--disable-sound or --disable-vrml on the configure command line.");
    warningprintedonce = TRUE;
  }
#endif // !HAVE_SOUND
}

/*!
  Destructor.
*/

SoVRMLSound::~SoVRMLSound(void)
{
  delete PRIVATE(this)->sourcesensor;

  PRIVATE(this)->stopPlaying();

  if (PRIVATE(this)->currentAudioClip != NULL)
    PRIVATE(this)->currentAudioClip->unref();
  PRIVATE(this)->currentAudioClip = NULL;

  delete[] PRIVATE(this)->audioBuffer;

#ifdef HAVE_SOUND
  assert(!PRIVATE(this)->hasValidAlSource());
  PRIVATE(this)->deleteAlBuffers();
#endif
  delete PRIVATE(this);
}

/*!
  Sets the doppler velocity relative to the global coordinate
  system. Not implemented yet.
*/

void
SoVRMLSound::setDopplerVelocity(float COIN_UNUSED_ARG(velocity))
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::setDopplerVelocity",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
}

/*!
  Returns the doppler velocity relative to the global coordinate
  system. Not implemented yet.
*/

float
SoVRMLSound::getDopplerVelocity() const
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::getDopplerVelocity",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
  return 0.0f;
}

/*!
  Sets the doppler factor. Not implemented yet.
*/

void
SoVRMLSound::setDopplerFactor(float COIN_UNUSED_ARG(factor))
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::setDopplerFactor",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
}

/*!
  Returns the doppler factor. Not implemented yet.
*/

float
SoVRMLSound::getDopplerFactor() const
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::getDopplerFactor",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
  return 0.0f;
}

/*!
  Starts playing the sound. Not implemented yet.
  Please use the fields of SoVRMLAudioClip to start and stop sounds.
*/

void
SoVRMLSound::startPlaying(SoPath * COIN_UNUSED_ARG(path), void * COIN_UNUSED_ARG(userdataptr))
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::startPlaying",
                            "Not yet implemented for Coin.");
}

/*!
  Stops playing the sound. Not implemented yet.
  Please use the fields of SoVRMLAudioClip to start and stop sounds.
*/

void
SoVRMLSound::stopPlaying(SoPath * COIN_UNUSED_ARG(path), void * COIN_UNUSED_ARG(userdataptr))
{
  // FIXME: as of yet unimplemented. 2003-02-26 thammer.
  SoDebugError::postWarning("SoVRMLSound::stopPlaying",
                            "Not yet implemented for Coin.");
}

/*
  FIXME: Calling setDefaultBufferingProperties or
  setBufferingProperties while a sound was playing might mess up quite
  a bit. This should be made more robust, or at the very least
  documented properly.  2002-11-15 thammer.  */

void SoVRMLSound::setDefaultBufferingProperties(int bufferLength, 
                                                int numBuffers, 
                                                SbTime sleepTime)
{
  SoVRMLSoundP::defaultBufferLength = bufferLength;
  SoVRMLSoundP::defaultNumBuffers = numBuffers;
  SoVRMLSoundP::defaultSleepTime = sleepTime.getValue();
}

void SoVRMLSound::setBufferingProperties(int bufferLength, 
                                         int numBuffers, 
                                         SbTime sleepTime)
{
  /* FIXME: if (coin_debug_audio()), post the function
     parameters. 2003-01-14 thammer */

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  PRIVATE(this)->numBuffers = numBuffers;
  PRIVATE(this)->sleepTime = sleepTime;

  if (PRIVATE(this)->bufferLength == bufferLength)
    return;

  PRIVATE(this)->bufferLength = bufferLength;
  delete[] PRIVATE(this)->audioBuffer;
  PRIVATE(this)->audioBuffer = new int16_t[PRIVATE(this)->bufferLength * 2];
}

void SoVRMLSound::getBufferingProperties(int &bufferLength, 
                                         int &numBuffers, 
                                         SbTime &sleepTime)
{
#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  bufferLength = PRIVATE(this)->bufferLength;
  numBuffers = PRIVATE(this)->numBuffers;
  sleepTime = PRIVATE(this)->sleepTime;
}

#ifdef HAVE_SOUND
static inline
void SbVec3f2ALfloat3(float *dest, const SbVec3f &source)
{
  source.getValue(dest[0], dest[1], dest[2]);
}
#endif // HAVE_SOUND

void SoVRMLSound::audioRender(SoAudioRenderAction *action)
{
#ifdef HAVE_SOUND
#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&PRIVATE(this)->syncmutex);
#endif
  SoState * state = action->getState();
  SoSoundElement::setSceneGraphHasSoundNode(state, this, TRUE);
  SoSoundElement::setSoundNodeIsPlaying(state, this, FALSE); 
  // ^-- might be changed below

  if (!SoAudioDevice::instance()->haveSound())
    return;

  if (PRIVATE(this)->currentAudioClip == NULL)
    return;

  SoSFBool * isActiveField = (SoSFBool *)PRIVATE(this)->currentAudioClip->getField("isActive");
  SbBool isactive = isActiveField->getValue();

  if ( (!PRIVATE(this)->playing) &&
       ( (!isactive) || (!SoSoundElement::isPartOfActiveSceneGraph(state)) ) )
    return;

  if (PRIVATE(this)->errorInThread) {
#ifdef HAVE_THREADS
    PRIVATE(this)->syncmutex.unlock();
#endif
    PRIVATE(this)->stopPlaying();
#ifdef HAVE_THREADS
    PRIVATE(this)->syncmutex.lock();
#endif
    return;
  }

  if ( PRIVATE(this)->playing &&
       ( (!isactive) || (!SoSoundElement::isPartOfActiveSceneGraph(state)) ) ) {
#ifdef HAVE_THREADS
      PRIVATE(this)->syncmutex.unlock();
#endif
      PRIVATE(this)->stopPlaying();
#ifdef HAVE_THREADS
      PRIVATE(this)->syncmutex.lock();
#endif
    return;
  }

  // if we got here then we're either already playing, or we should be

  if (!PRIVATE(this)->hasValidAlSource())
    PRIVATE(this)->generateAlSource();
  
  // Clamp field values
  float intensity = this->intensity.getValue();
  float minFront = this->minFront.getValue();
  float maxFront = this->maxFront.getValue();
  float minBack = this->minBack.getValue();
  float maxBack = this->maxBack.getValue();

  intensity = (intensity < 0.0f) ? 0.0f : (intensity > 1.0f) ? 1.0f : intensity;
  maxFront = (maxFront < 0.0f) ? 0.0f : maxFront;
  minFront = (minFront > maxFront) ? maxFront : minFront;
  maxBack = (maxBack < 0.0f) ? 0.0f : maxBack;
  minBack = (minBack > maxBack) ? maxBack : minBack;

  // get listener stuff
  const SbVec3f &listenerpos = SoListenerPositionElement::get(state);
  const SbRotation &listenerorientation = SoListenerOrientationElement::get(state);
  float listenergain = SoListenerGainElement::get(state);

#if COIN_DEBUG && 0 // debug
  float x, y, z;
  listenerpos.getValue(x, y, z);
  SoDebugError::postInfo("SoVRMLSound::audioRender", 
                         "listenerpos = (%0.2f, %0.2f, %0.2f)", x, y, z);
#endif // debug

  int error;
  float alfloat3[3];
  SbVec3f pos, worldpos, relativepos;

  pos = this->location.getValue();
  SoModelMatrixElement::get(action->getState()).multVecMatrix(pos, worldpos);
  worldpos -= listenerpos;
  relativepos = worldpos;
  listenerorientation.inverse().multVec(worldpos, worldpos);
#if COIN_DEBUG && 0 // debug
  worldpos.getValue(x, y, z);
  SoDebugError::postInfo("SoVRMLSound::audioRender", 
                         "rotated (inversed) : (%0.2f, %0.2f, %0.2f)", 
                         x, y, z);
#endif // debug

  // Since we're not using OpenAL to calculate the distance
  // attenuation, we normalize the source position (relative to the
  // listener) before we send it to OpenAL.  So it's only the
  // direction from the listener to the source that matters
  SbVec3f normworldpos = worldpos;
  if (normworldpos.length() > 0.0f)
    normworldpos.normalize();

  SbVec3f2ALfloat3(alfloat3, normworldpos);
  if (!this->spatialize.getValue()) {
    // don't spatialize - i.e. do distance attenuation but not
    // directional effects
    alfloat3[0] = 0.0f; alfloat3[1] = 0.0f; alfloat3[2] = 0.0f;
  }

  // Set position
  openal_wrapper()->alSourcefv(PRIVATE(this)->sourceId, AL_POSITION, alfloat3);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSound::audioRender",
                              "alSourcefv(,AL_POSITION,) failed. %s",
                              coin_get_openal_error(error));
    PRIVATE(this)->deleteAlSource();
    return;
  }

#if 0
  // Note: if we ever want to implement velocity (supported by OpenAL)
  // then this is how it should be done.  get alfloat3 from
  // PRIVATE(this)->velocity.  2002-10-07 thammer.
  SbVec3f2ALfloat3(alfloat3, velocity.getValue());

  alSourcefv(this->sourceId, AL_VELOCITY, alfloat3);
  if ((error = alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSound::GLRender",
                              "alSourcefv(,AL_VELOCITY,) failed. %s",
                              coin_get_openal_error(error));
    PRIVATE(this)->deleteAlSource();
    return;
  }
#endif

  float gain = intensity;
  gain *= listenergain;

  // Distance attenuation

  // There are some nice formulas for the ellipse at 
  // http://mathworld.wolfram.com/Ellipse.html
  // the letters used below are defined there
  // (this is the common lettering, as used in e.g. 
  // Edwards & Penny's "Calculus and Analytic Geometry")

  // r and theta measured from one of the focal points
  // a = (back + front) / 2
  // c = (front - back) / 2
  // e = c / a = (front - back) / (front + back)
  // r = (a * (1 - e^2)) / (1 + e * cos theta))

  float theta = 0.0f;
  float distance = 0.0f;

  SbVec3f world_direction;
  SbVec3f source_translation;
  SbRotation source_rotation;
  SbVec3f source_scale;
  SbRotation source_scaleorientation;

  SoModelMatrixElement::get(action->getState()).getTransform(
    source_translation, source_rotation, source_scale,
    source_scaleorientation);
  
  source_rotation.multVec(this->direction.getValue(),
    world_direction);

  SbRotation rot(world_direction, -relativepos);
  SbVec3f dummy;
  rot.getValue(dummy, theta);
  theta = (float)(M_PI - theta);

  distance = relativepos.length();

  float min_r =0;
  float max_r = 0;
  float a, c, e;

  a = (minFront + minBack) / 2.0f;
  c = (minFront - minBack) / 2.0f;
  e = c / a;
  if ( (e < 1.0f) && (e > -1.0f))
    min_r = (float) ((a * (1 - e * e)) / (1 + e * cos(theta)));
  // if e == +/- 1, the ellipse is (approaches) a straight horizontal line
  // and we'll define the sound level to be zero in this case

  a = (maxFront + maxBack) / 2.0f;
  c = (maxFront - maxBack) / 2.0f;
  e = c / a;
  if ( (e < 1.0f) && (e > -1.0f))
    max_r = (float) ((a * (1 - e * e)) / (1 + e * cos(theta)));

  if (max_r < min_r) {
    max_r = min_r;
  }

  if (distance >= max_r)
    gain = 0.0f;
  else if (distance >= min_r) {
    float diff_r = max_r - min_r;
    if (diff_r > 0.0f) {
      /* Note: According to the VRML97 spec, the attenuation should vary 
         from 0dB at the minimum (inner) ellipsoid to 20dB at the 
         maximum (outer) ellipsoid. Since OpenAL's gain uses a linear
         scale, not a decibel one, we must do some conversion.
         
         attenuation_dB = -20 * (distance - min_r) / diff_r; // VRML97 spec
         attenuation_linear = 10 ^ (attenuation_dB / 20)
                            = 10 ^ (-1 * (distance - min_r) / diff_r)
                            = 10 ^ ( (min_r - distance) / diff_r)

         Personally, I think a maximum attenuation of 20dB is too
         little. This means that the transition between no sound just
         outside the max ellipsoid and some sound just inside the max
         ellipsoid will be quite abrupt. 40dB would have been a much
         better choice - or the falloff could be steeper (nonlinear)
         when approaching the max ellipsoid from inside. But we'll
         follow the VRML97 spec.

         Old (linear, not dB) formula, kept for future reference: 
           gain *= (max_r - distance) / diff_r;

         2005-04-15 thammer.  */
      float attenuation_linear =
        (float) pow(10.0f, ((min_r - distance) / diff_r));
      gain *= attenuation_linear;
    }
  }

  // clamp gain to [0.0, 1.0]
  gain = (gain > 0.0f) ? ((gain < 1.0f) ? gain : 1.0f) : 0.0f;
  openal_wrapper()->alSourcef(PRIVATE(this)->sourceId,AL_GAIN, gain);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSound::audioRender",
                              "alSourcef(,AL_GAIN,) failed. %s",
                              coin_get_openal_error(error));
    PRIVATE(this)->deleteAlSource();
    return;
  }


  /* Note: According to the OpenAL 1.0 spec, the legal range for pitch
     is [0, 1].  However, both the Win32 implementation and the linux
     implementation supports the range [0, 2]. The Mac implementation
     supports the range [0, infinite>.  Testing shows that Creative
     Labs' binary-only OpenAL implementations also supports the range
     [0, 2]. Since it is very useful to be able to increase the pitch
     above unity, and since the VRML97 spec specifies the range to be
     [0, infinite>, we will allow the range to be within [0, 2], and
     clamp outside this range. 2002-11-07 thammer.

     Update: It turns out that CreativeLabs' Win32 binary release of
     OpenAL will crash if pitch == 0.0. For that reason, we will clamp
     at 0.01. The range supported is thus [0.01..2.0]. 2002-11-07
     thammer.  */

  float pitch = PRIVATE(this)->currentAudioClip->pitch.getValue();
  pitch = (pitch >= 0.01f) ? ( (pitch<=2.0f) ? pitch : 2.0f ) : 0.01f;
  openal_wrapper()->alSourcef(PRIVATE(this)->sourceId, AL_PITCH, pitch);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR)
  {
    SoDebugError::postWarning("SoVRMLSoundP::sourceSensorCB",
                              "alSourcef(,AL_PITCH,) failed. %s",
                              coin_get_openal_error(error));
    PRIVATE(this)->deleteAlSource();
    return;
  }

  // Spatialization

  int newchannels = this->spatialize.getValue() ? 1 : 2;
  if (PRIVATE(this)->channels != newchannels) {
    if (PRIVATE(this)->playing) {
#ifdef HAVE_THREADS
      PRIVATE(this)->syncmutex.unlock();
#endif
      PRIVATE(this)->stopPlaying();
#ifdef HAVE_THREADS
      PRIVATE(this)->syncmutex.lock();
#endif
    }

    PRIVATE(this)->channels = newchannels;
  }

  if ( (!PRIVATE(this)->playing) && isactive )  {
    PRIVATE(this)->startPlaying();
  }

  SoSoundElement::setSoundNodeIsPlaying(state, this, TRUE);

#endif // HAVE_SOUND
}

static void
mono2stereo(short int *buffer, int length)
{
  // assumes that buffersize = length * sizeof(short int) * 2
  for (int i=length-1; i>=0; i--) {
    buffer[i*2] = buffer[i*2+1] = buffer[i];
  }
}

static void
stereo2mono(short int *buffer, int length)
{
  // assumes that buffersize = length * sizeof(short int) * 2

  for (int i=0; i<length; i++) {
    buffer[i] = buffer[i*2] / 2 + buffer[i*2+1] / 2;
  }
}

#ifdef HAVE_SOUND
static int
getALSampleFormat(int channels, int bitspersample)
{
  int  alformat = 0;;

  if ( (channels==1) && (bitspersample==8) )
    alformat = AL_FORMAT_MONO8;
  else if ( (channels==1) && (bitspersample==16) )
    alformat = AL_FORMAT_MONO16;
  else if ( (channels==2) && (bitspersample==8) )
    alformat = AL_FORMAT_STEREO8;
  else if ( (channels==2) && (bitspersample==16) )
    alformat = AL_FORMAT_STEREO16;

  return alformat;
}
#endif // HAVE_SOUND

void
SoVRMLSoundP::deleteAlBuffers()
{
#ifdef HAVE_SOUND
  int error;
  if (SoAudioDevice::instance()->haveSound()) {
    while (this->alBuffers.getLength() > 0) {
        unsigned int bufferid = this->alBuffers.pop();
        openal_wrapper()->alDeleteBuffers(1, &bufferid);
        if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
          SoDebugError::postWarning("SoVRMLSound::deleteAlBuffers",
                                    "alDeleteBuffers failed. %s",
                                    coin_get_openal_error(error));
        }
    }
  }
#endif
}

void
SoVRMLSoundP::generateAlSource()
{
#ifdef HAVE_SOUND
  if (SoAudioDevice::instance()->haveSound()) {
    assert (this->sourceId == 0);
    int error;
    openal_wrapper()->alGenSources(1, &(this->sourceId));
    if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
      SoDebugError::post("SoVRMLSound::generateAlSource",
                         "alGenSources failed. %s",
                         coin_get_openal_error(error));
      return;
    }
    // Turn off OpenAL's distance attenuation for this source
    // (We're doing distance attenuation ourselves)
    // Distance attenuation is also eliminated for this source in 
    // SoVRMLSound::audioRender() by normalizing the position
    // of the source relative to the listener, and in 
    // SoAudioDevice::init() by setting the distance model to 
    // AL_NONE.

    /* Note: On some systems, it might not be possible to disable
      distance attenuation by setting the AL_ROLLOFF_FACTOR to 0.0.
      This has been experienced by thammer on WindowsXP using Creaitve
      Labs Extigy, driver version 5.12.01.0038.  On the same system,
      using another soundcard (DellInspiron 8200's built-in
      soundcard), distance attenuation was disabled, as it should be.
      This difference is probably due to poor DirectSound3D drivers
      for the Extigy.  2002-11-07 thammer.  */
    openal_wrapper()->alSourcef(this->sourceId,
                                AL_ROLLOFF_FACTOR, 0.0f);
    if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
      SoDebugError::postWarning("SoVRMLSound::generateAlSource",
                                "alSourcef(,AL_ROLLOFF_FACTOR,) failed. %s",
                                coin_get_openal_error(error));
      this->deleteAlSource();
      return;
    }

  }
#endif
}

void
SoVRMLSoundP::deleteAlSource()
{
#ifdef HAVE_SOUND
  if (SoAudioDevice::instance()->haveSound()) {
    assert (this->sourceId != 0);
    int error;
    openal_wrapper()->alDeleteSources(1, &(this->sourceId));
    this->sourceId = 0;
    if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
      SoDebugError::postWarning("SoVRMLSound::~SoVRMLSound",
                                "alDeleteSources() failed. %s",
                                coin_get_openal_error(error));
    }
  }
#endif
}

SbBool
SoVRMLSoundP::hasValidAlSource()
{
  return this->sourceId != 0;
}


void *
SoVRMLSoundP::threadCallbackWrapper(void *userdata)
{
  SoVRMLSoundP *thisp = (SoVRMLSoundP *)userdata;
  return thisp->threadCallback();
}

void *
SoVRMLSoundP::threadCallback()
{
  /* FIXME: An application using Coin might crash when shutdown
     because a SoAudioClip node might have been deleted (even though
     it is ref'ed). Using this->currentAudioClip in the fillThread is
     then undefined...  Investigate this further. 20021104 thammer */
  while (!this->exitthread) {
    this->fillBuffers();
    /* FIXME: If we're not playing (and the thread should be
       idle-ing), the sleep time could be a lot longer, or we could
       have infinite sleeping time, as long as the main thread calls
       wake() on the condvar when thread should exit or when playback
       should start. This would make the thread consume less CPU time
       when idle. 2003-01-20 thammer. */
#ifdef HAVE_THREADS
    // cc_sleep(this->workerThreadSleepTime.getValue());
    /* Note: See http://www.llnl.gov/computing/tutorials/workshops/workshop/pthreads/MAIN.html#ConditionVariables
       for a sample of condvar usage. 2002-01-20 thammer. */
    this->exitthreadmutex.lock();
    if (!this->exitthread)
      this->exitthreadcondvar.timedWait(this->exitthreadmutex, this->workerThreadSleepTime);
    this->exitthreadmutex.unlock();
#endif
  }
  return NULL;
}

void
SoVRMLSoundP::timercb(void * data, SoSensor * COIN_UNUSED_ARG(s))
{
  SoVRMLSoundP * thisp = (SoVRMLSoundP*) data;
  thisp->fillBuffers();
}

SbBool SoVRMLSoundP::stopPlaying()
{
#ifdef HAVE_SOUND
  #if COIN_DEBUG && DEBUG_AUDIO // debug
  SoDebugError::postInfo("SoVRMLSound::stopPlaying", "stop");
  #endif // debug

  if (!SoAudioDevice::instance()->haveSound())
    return FALSE;

  if (!this->playing)
    return TRUE;

  int error;

  // stop timersensor
  if (this->timersensor) {
    if (this->timersensor->isScheduled())
      this->timersensor->unschedule();
    delete this->timersensor;
    this->timersensor = NULL;
  }

  /* FIXME: joining with workerThread will normally cause a lag of
     sleepTime. This should be fixed in some way. 20021107 thammer. */

  // stop thread
#ifdef HAVE_THREADS
  if (this->workerThread!=NULL) {
    this->exitthreadmutex.lock();
    this->exitthread = TRUE;
    this->exitthreadcondvar.wakeAll();
    this->exitthreadmutex.unlock();
    void *retval = NULL;
    cc_thread_join(this->workerThread, &retval);
    cc_thread_destruct(this->workerThread);
    this->workerThread = NULL;
  }
#endif // HAVE_THREADS

  this->errorInThread = FALSE;

  SbBool retval = TRUE;


  openal_wrapper()->alSourceStop(this->sourceId);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSound::stopPlaying",
                              "alSourceStop failed. %s",
                              coin_get_openal_error(error));
    retval= FALSE;
  }

  /* Note: Rewinding will make sure state is AL_INITIAL, not just
     AL_STOPPED.  This lets us give the user a warning if the source
     stopped playing because a buffer underrun occurred. See
     fillBuffers().  2002-11-07 thammer.  */

  openal_wrapper()->alSourceRewind(this->sourceId);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSound::stopPlaying",
                              "alSourceRewind failed. %s",
                              coin_get_openal_error(error));
    retval= FALSE;
  }

  int      processed;
  int      queued;

  openal_wrapper()->alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);
  openal_wrapper()->alGetSourcei(this->sourceId, AL_BUFFERS_PROCESSED, &processed);

  /* Note: if the sound was stopped after the thread reported to the
     audioclip that all buffer were played, queued and processed
     should both be 0. If the sound was stopped for any other reason
     (for instance because the url or startTime/stopTime of the
     audioclip changed, processed and/or queued could be != 0.
     20021106 thammer

  */
#if COIN_DEBUG && DEBUG_AUDIO // debug
  SoDebugError::postInfo("SoVRMLSound::stopPlaying",
                         "Queued: %d, Processed: %d.",
                         queued, processed);
#endif // debug

  if (processed >0) {
    unsigned int *removedBuffers = new unsigned int[processed];
    openal_wrapper()->alSourceUnqueueBuffers(this->sourceId,
                                             processed, removedBuffers);
    delete[] removedBuffers;
    if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
      SoDebugError::postWarning("SoVRMLSoundP::stopPlaying",
                                "alSourceUnqueueBuffers failed. %s",
                                coin_get_openal_error(error));
      retval = FALSE;
    }
  }

  /* Note: The OpenAL manual states that when a source is stopped,
     "the entire queue is considered processed" (see documentation for
     "Unqueueing command".  However, sometimes (encountered on Win32
     using OpenAL from CVS) after stopping, processed==0 and queued !=
     0, which means that the above unqueueing would fail (because it
     unqueues only processed buffers).

     So, we try explisitly setting the AL_BUFFER source attribute to
     NULL, which is legal according to the OpenAL documentation (and
     also redundant, wrt alSourceUnqueueBuffers according to the same
     documentation).

     To be absolutely sure the queue is empty, we verify this with an
     assert.

     2003-01-17 thammer. */

  openal_wrapper()->alSourcei(this->sourceId, AL_BUFFER, AL_NONE);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSoundP::stopPlaying",
                              "alSourcei(,AL_BUFFER, AL_NONE) failed. %s",
                              coin_get_openal_error(error));
    retval = FALSE;
  }

  openal_wrapper()->alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);

  assert(queued == 0);

  this->deleteAlSource();
  this->deleteAlBuffers();

  this->playing = FALSE;

  return retval;
#else
  return FALSE;
#endif // HAVE_SOUND
}

extern "C" {
typedef void * thread_f(void *);
}

SbBool SoVRMLSoundP::startPlaying()
{
#ifdef HAVE_SOUND
  #if COIN_DEBUG && DEBUG_AUDIO
    SoDebugError::postInfo("SoVRMLSound::startPlaying", "start");
  #endif // debug

  if (!SoAudioDevice::instance()->haveSound())
    return FALSE;

  if (this->playing)
    return TRUE;

  int error;

  assert(this->alBuffers.getLength() == 0);

  if (!this->hasValidAlSource())
    this->generateAlSource();

  openal_wrapper()->alSourcei(this->sourceId, AL_LOOPING, FALSE);
  if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
    SoDebugError::postWarning("SoVRMLSoundP::startPlaying",
                              "alSourcei(,AL_LOOPING,) failed. %s",
                              coin_get_openal_error(error));
    if (this->hasValidAlSource())
      this->deleteAlSource();

    return FALSE;
  }

  this->playing = TRUE;
  this->endoffile = FALSE;
  this->waitingForAudioClipToFinish = FALSE;
  this->cliphandle = NULL;

  // Start timer or thread
  if (this->useTimerCallback) {
    // stop previous timer
    if (this->timersensor) {
      if (this->timersensor->isScheduled())
        this->timersensor->unschedule();
      delete this->timersensor;
      this->timersensor = NULL;
    }
    this->errorInThread = FALSE;
    // start new timer
    this->timersensor = new SoTimerSensor(timercb, this);
    this->timersensor->setInterval(this->sleepTime);
    this->timersensor->schedule();
  }
  else {
    // stop existing thread, start new thread
#ifdef HAVE_THREADS
    if (this->workerThread!=NULL) {
      /* FIXME: Verify that this will actually happen sometimes. Also
         verify that it is indeed necessary to stop and start the
         thread. 2003-01-20 thammer. */
      this->exitthreadmutex.lock();
      this->exitthread = TRUE;
      this->exitthreadcondvar.wakeAll();
      this->exitthreadmutex.unlock();
      void *retval = NULL;
      cc_thread_join(this->workerThread, &retval);
      cc_thread_destruct(this->workerThread);
      this->workerThread = NULL;
    }

    this->workerThreadSleepTime = this->sleepTime;
    this->errorInThread = FALSE;
    this->exitthread = FALSE;
    this->workerThread = cc_thread_construct((thread_f *) this->threadCallbackWrapper, this);
#endif // HAVE_THREADS
  }

  return TRUE;
#else
  return FALSE;
#endif // HAVE_SOUND
}

void SoVRMLSoundP::fillBuffers()
{
#ifdef HAVE_SOUND
#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&this->syncmutex);
#endif

  assert(this->currentAudioClip != NULL);
  assert(this->hasValidAlSource());

  if (this->waitingForAudioClipToFinish) {
#if COIN_DEBUG && DEBUG_AUDIO // debug
    SoDebugError::postInfo("SoVRMLSound::fillBuffers",
                           "this->waitingForAudioClipToFinish == TRUE, "
                           "returning.");
#endif // debug
    return;
  }

  int      processed;

  // Get status
  openal_wrapper()->alGetSourcei(this->sourceId, AL_BUFFERS_PROCESSED, 
                                 &processed);

  int      queued;
  openal_wrapper()->alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);

#if COIN_DEBUG && DEBUG_AUDIO
  SoDebugError::postInfo("SoVRMLSound::fillBuffers",
                         "Processed: %d, Queued: %d", processed, queued);
#endif // debug

  unsigned int bufferid = 0;
  int error;
  size_t ret;

#if 0
  // 20021021 thammer, kept for debugging purposes
  if (queued<=0) {
    // no buffers were queued, so there's nothing to do. This should
    // only happen after audioclip::fillBuffer() returns NULL to
    // indicate an EOF, and sound::fillBuffers() does not queue new
    // buffers after that
    #if COIN_DEBUG && DEBUG_AUDIO // debug
            SoDebugError::postInfo("SoVRMLSound::fillBuffers",
                                   "No more buffers queued (we're "
                                   "probably stopping soon)");
    #endif // debug
    return;
  }
#endif

  if (this->endoffile) {
    if (processed > 0) {
      assert(queued > 0);
      unsigned int *removedBuffers = new unsigned int[processed];
      openal_wrapper()->alSourceUnqueueBuffers(this->sourceId, processed,
                                               removedBuffers);
      delete[] removedBuffers;
    } else if (queued == 0) {
      this->waitingForAudioClipToFinish = TRUE;
      // inform currentAudioClip() that the last buffer has been played,
      // so it can decide if it would like to stop playing
      int numchannels;
      ret = this->currentAudioClip->read(this->cliphandle, NULL, 0, 
                                         numchannels);
      assert (ret == 0); // or else the AudioClip isn't performing as it should
    }
  } else {
    while (((processed > 0) || (queued<this->numBuffers)) 
           && !this->endoffile)  {
      // FIXME: perhaps we should reread processed in the while loop
      // too. This might make buffer underruns less frequent.
      // 2002-10-07 thammer.

      if (queued < this->numBuffers) {
        openal_wrapper()->alGenBuffers(1, &bufferid);
        if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
          SoDebugError::post("SoVRMLSoundP::fillBuffers",
                             "alGenBuffers failed. %s",
                             coin_get_openal_error(error));
          this->errorInThread = TRUE;
          return;
        }
        this->alBuffers.push(bufferid);
      } else {
      // unqueue one buffer
        openal_wrapper()->alSourceUnqueueBuffers(this->sourceId, 1, &bufferid);
        if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
          SoDebugError::post("SoVRMLSound::fillBuffers",
                             "alSourceUnqueueBuffers failed. "
                             "Queued: %d, Processed: %d."
                             "OpenAL error: %s.",
                             queued, processed,
                             coin_get_openal_error(error));
          this->errorInThread = TRUE;
          return;
        }
      }
      // fill buffer

      int numchannels;
      /* Notes (mainly kept for future debugging of deadlock issues):
         Unlocking syncmutex here might open up a can of worms. I have
         looked at the variables used in this thread, and it looks
         like it's ok to unlock at this point, but it's very easy to
         overlook something!

         The reason I had to look into doing more finegrained
         synchronization was because
         I stumbled across a deadlock. Description of the deadlock:
         - sound::fillbuffers locks sound::syncmutex and calls 
           clip::fillbuffer,
           which tries to lock clip::syncmutex
         - clip::timerCB locks clip::syncmutex and calls stopPlaying, which
           changes isActive, which triggers sound::sourceSensorCB, which tries
           to lock sound::syncmutex
         ==> deadlock
         A nice way to test for deadlocks is to do a lot of
         audioDevice::enable() and audioDevice::disable() calls fast, while
         a sound is / should be playing.
         First, I implemented buffer sync'ing and unlocked syncmutex here.

         After implementing buffer synchronization, I stumbled across another
         deadlock:
         - ac:timer lock ac:sync-> isActive -> sound:sourceCB
           -> sound:stopPlaying -> thread_join (lock-wait for thread to finish)
         - sound:fillbuffers -> ac:fillbuffer-> tries to lock ac:sync
         ==> deadlock
         This deadlock was removed by making sure ac:sync wasn't locked while
         changing ac:isActive.

         This also fixes the first deadlock described above, so I'll probably
         get away with not unlocking syncmutex here after all.

         2002-11-18 thammer */
      ret = this->currentAudioClip->read(this->cliphandle,
                                         this->audioBuffer,
                                         this->bufferLength, numchannels);

      if ( (numchannels==1) && (this->channels==2) )
        mono2stereo(this->audioBuffer, this->bufferLength);
      else if ( (numchannels==2) && (this->channels==1) )
        stereo2mono(this->audioBuffer, this->bufferLength);

      // copy buffer data to the newly generated or unqueued openal buffer

      int  alformat = 0;;
      alformat = getALSampleFormat(this->channels, 16);

      openal_wrapper()->alBufferData(bufferid, alformat, this->audioBuffer,
                   this->bufferLength * sizeof(int16_t) * this->channels,
                   this->currentAudioClip->getSampleRate());

      if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
        SoDebugError::post("SoVRMLSound::fillBuffers",
                           "alBufferData(buffer=%d, format=%d, data=%p, "
                           "size=%d, freq=%d) failed. "
                           "Queued: %d, Processed: %d. "
                           "OpenAL error: %s",
                           bufferid, alformat, this->audioBuffer,
                           this->bufferLength * sizeof(int16_t) *
                           this->channels,
                           this->currentAudioClip->getSampleRate(),
                           queued, processed,
                           coin_get_openal_error(error));
        this->errorInThread = TRUE;
        return;
      }

      // Queue buffer
      openal_wrapper()->alSourceQueueBuffers(this->sourceId, 1, &bufferid);
      if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
        SoDebugError::post("SoVRMLSound::fillBuffers",
                           "alSourceQueueBuffers failed. "
                           "Queued: %d, Processed: %d."
                           "OpenAL error: %s.",
                           queued, processed,
                           coin_get_openal_error(error));
        this->errorInThread = TRUE;
        return;
      }

      if (ret == 0) {
        this->endoffile = TRUE;
        // AudioClip has reached EOF (or an error), so we shouldn't
        // fill any more buffers
      }

      // Check to see if we're still playing if not, make sure to
      // start over again. If we're not playing, it's because the
      // buffers have not been filled up (unqueued, filled, queued)
      // fast enough, so the source has played the last buffer in the
      // queue and changed state from playing to stopped.

      int      state;
      openal_wrapper()->alGetSourcei(this->sourceId, AL_SOURCE_STATE, &state);
      if (state != AL_PLAYING) {
        if ( (state == AL_STOPPED) || (state == AL_INITIAL) ) {
          openal_wrapper()->alSourcePlay(this->sourceId);
          if ((error = openal_wrapper()->alGetError()) != AL_NO_ERROR) {
            SoDebugError::post("SoVRMLSoundP::fillBuffers",
                               "alSourcePlay(sid=%d) failed: %s",
                               this->sourceId,
                               coin_get_openal_error(error));
            this->errorInThread = TRUE;
            return;
          }
          if (state == AL_STOPPED)
            SoDebugError::postWarning("SoVRMLSoundP::fillBuffers",
              "Buffer underrun. The audio source had to be restarted. "
              "Queued: %d, Processed: %d. "
              "Try increasing buffer size (current: %d frames), "
              "and/or increasing number of buffers (current: %d buffers), "
              "and/or decreasing sleeptime (current: %0.3fs)",
              queued, processed,
              this->bufferLength, this->numBuffers,
              this->sleepTime.getValue());
          else {
#if COIN_DEBUG && DEBUG_AUDIO // debug
            SoDebugError::postInfo("SoVRMLSoundP::fillBuffers",
              "Source had not been started (state==AL_INITIAL). "
              "The audio source has been started. "
              "Queued: %d, Processed: %d. "
              "Current buffer size: %d frames, "
              "current number of buffers: %d buffers, "
              "current sleeptime: %0.3fs",
              queued, processed,
              this->bufferLength, this->numBuffers,
              this->sleepTime.getValue());
#endif // debug
          }
        }
        else {
#if COIN_DEBUG && DEBUG_AUDIO // debug
          char statestr[20];
          switch (state) {
          case AL_INITIAL : strcpy(statestr, "initial"); break;
          case AL_PLAYING : strcpy(statestr, "playing"); break;
          case AL_PAUSED : strcpy(statestr, "paused"); break;
          case AL_STOPPED : strcpy(statestr, "stopped"); break;
          default : strcpy(statestr, "unknown"); break;
          };
          // 20021007 thammer fixme: deal with this properly!
          SoDebugError::postWarning("SoVRMLSound::fillBuffers",
                                    "state == %s. Don't know what "
                                    "to do about it...", statestr);
#endif
        }
      }
      if (queued < this->numBuffers)
        queued++;
      else
        processed--;
    }
  }
#endif // HAVE_SOUND
}

//
// called when source changes
//
void
SoVRMLSoundP::sourceSensorCBWrapper(void * data, SoSensor *)
{
  SoVRMLSoundP * thisp = (SoVRMLSoundP*) data;
  thisp->sourceSensorCB(NULL);
}

//
// called when source changes
//
void
SoVRMLSoundP::sourceSensorCB(SoSensor *)
{
#ifdef HAVE_SOUND

#ifdef HAVE_THREADS
  SbThreadAutoLock autoLock(&this->syncmutex);
#endif
  if (!SoAudioDevice::instance()->haveSound())
    return;

  SoNode *node = (SoNode *)PUBLIC(this)->source.getValue();

  if (!node->isOfType(SoVRMLAudioClip::getClassTypeId())) {
    SoDebugError::postWarning("SoVRMLSoundP::sourceSensorCB",
                              "Unknown source node type");
    if (this->currentAudioClip != NULL) {
      this->currentAudioClip->unref();
#ifdef HAVE_THREADS
      this->syncmutex.unlock();
#endif
      this->stopPlaying();
#ifdef HAVE_THREADS
      this->syncmutex.lock();
#endif
    }
    this->currentAudioClip = NULL;
    return;
  }

  SoVRMLAudioClip *audioClip = (SoVRMLAudioClip *)node;
  if (audioClip != this->currentAudioClip) {
    if (this->currentAudioClip != NULL) {
      this->currentAudioClip->unref();
      this->currentAudioClip = NULL;
#ifdef HAVE_THREADS
      this->syncmutex.unlock();
#endif
      this->stopPlaying();
#ifdef HAVE_THREADS
      this->syncmutex.lock();
#endif
    }
    if (audioClip!=NULL)
      audioClip->ref();
    this->currentAudioClip = audioClip;
  }

  if (this->currentAudioClip == NULL)
    return;

  SoSFBool * isActiveField =
    (SoSFBool *)this->currentAudioClip->getField("isActive");
  SbBool isactive = isActiveField->getValue();

  if ( this->playing && (!isactive) ) {
#if COIN_DEBUG && DEBUG_AUDIO // debug
    SoDebugError::postInfo("SoVRMLSound::sourceSensorCB",
                           "this->playing && (!isactive). "
                           "Calling stopPlaying()");
#endif // debug
#ifdef HAVE_THREADS
      this->syncmutex.unlock();
#endif
      this->stopPlaying();
#ifdef HAVE_THREADS
      this->syncmutex.lock();
#endif
  }
#endif // HAVE_SOUND
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
