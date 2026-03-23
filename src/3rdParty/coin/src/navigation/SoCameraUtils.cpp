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

#include "navigation/SoCameraUtils.h"
#include "coindefs.h"

#include <cassert>
#include <cmath>

#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoFrustumCamera.h>
#include <Inventor/errors/SoDebugError.h>

class SoOrthoPerspectiveCameraManager : public SoCameraManager {
public:
  SoOrthoPerspectiveCameraManager(SoCamera * camera);

  SoOrthographicCamera * getCastCamera(void) const;

  virtual void setZoomValue(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomValue(float diffvalue, SbBool limit = TRUE); // diffvalue=0.0 is identity
  virtual void adjustZoom(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomFactor(void) const;

  virtual void setZoomValueByDolly(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomByDollyDistance(float distance, SbBool limit = TRUE); // distance=0.0 is identity
  virtual void adjustZoomByDolly(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomByDollyFactor(void) const;

};

class SoPerspectiveCameraManager : public SoCameraManager {
public:
  SoPerspectiveCameraManager(SoCamera * camera);

  SoPerspectiveCamera * getCastCamera(void) const;

  virtual void setZoomValue(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomValue(float diffvalue, SbBool limit = TRUE); // diffvalue=0.0 is identity
  virtual void adjustZoom(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomFactor(void) const;

  virtual void setZoomValueByDolly(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomByDollyDistance(float distance, SbBool limit = TRUE); // distance=0.0 is identity
  virtual void adjustZoomByDolly(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomByDollyFactor(void) const;

};

class SoFrustumCameraManager : public SoCameraManager {
public:
  SoFrustumCameraManager(SoCamera * camera);

  SoFrustumCamera * getCastCamera(void) const;

  virtual void setZoomValue(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomValue(float diffvalue, SbBool limit = TRUE); // diffvalue=0.0 is identity
  virtual void adjustZoom(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomFactor(void) const;

  virtual void setZoomValueByDolly(float zoomvalue, SbBool limit = FALSE);
  virtual void adjustZoomByDollyDistance(float distance, SbBool limit = TRUE); // distance=0.0 is identity
  virtual void adjustZoomByDolly(float factor, SbBool limit = TRUE); // factor=1.0 is identity

  virtual float getZoomByDollyFactor(void) const;

};


// *************************************************************************

SoCameraManager *
SoCameraManager::createFor(SoCamera * camera)
{
  assert(camera);

  // FIXME: need to be able to instantiate an appdomain orthographic camera as well
  SoCameraManager * manager = NULL;
  if (camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    manager = new SoPerspectiveCameraManager(camera);
  }
  else if (camera->isOfType(SoFrustumCamera::getClassTypeId())) {
    manager = new SoFrustumCameraManager(camera);
  }
  else if (camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    manager = new SoOrthoPerspectiveCameraManager(camera);
  }
  else {
    SoDebugError::postInfo("SoCameraManager::createFor",
                           "Unsupported camera type ('%s').",
                           camera->getTypeId().getName().getString());
  }
  return manager;
}

SoCameraManager::SoCameraManager(SoCamera * thecamera)
: camera(thecamera)
{
  assert(camera);
  camera->ref();

  this->havezoomlimits = FALSE;
  this->minzoom = this->maxzoom = 0.0f;
  this->havezoombydollylimits = FALSE;
  this->mindollydistance = this->maxdollydistance = this->unitydistance = 0.0f;
}

SoCameraManager::~SoCameraManager(void)
{
  camera->unref();
  camera = NULL;
}

void
SoCameraManager::setZoomLimits(float minzoomvalue, float maxzoomvalue)
{
  if (minzoomvalue == 0.0f && maxzoomvalue == 0.0f) {
    this->minzoom = this->maxzoom = 0.0f;
    this->havezoomlimits = FALSE;
  }
  else {
    this->minzoom = minzoomvalue;
    this->maxzoom = maxzoomvalue;
    this->havezoomlimits = TRUE;
  }
}

SbBool
SoCameraManager::getZoomLimits(float & minzoomvalue, float & maxzoomvalue) const
{
  minzoomvalue = this->minzoom;
  maxzoomvalue = this->maxzoom;
  return this->havezoomlimits;
}

float
SoCameraManager::getZoomValue(void) const
{
  return 1.0f; // no implementation, zoom is 1.0
}

void
SoCameraManager::setZoomByDollyLimits(float mindollydist, float maxdollydist, float unitydist)
{
  if (mindollydist == 0.0f && maxdollydist == 0.0f && unitydist == 0.0f) {
    this->mindollydistance = this->maxdollydistance = this->unitydistance =  0.0f;
    this->havezoombydollylimits = FALSE;
  }
  else {
    this->mindollydistance = mindollydist;
    this->maxdollydistance = maxdollydist;
    this->unitydistance = unitydist;
    this->havezoombydollylimits = TRUE;
  }

}

SbBool
SoCameraManager::getZoomByDollyLimits(float & mindollydist, float & maxdollydist, float & unitydist) const
{
  mindollydist = this->mindollydistance;
  maxdollydist = this->maxdollydistance;
  unitydist = this->unitydistance;
  return this->havezoombydollylimits;
}

float
SoCameraManager::getZoomByDollyValue(void) const
{
  return 1.0f; // no implementation, zoom is 1.0
}

SbVec3f
SoCameraManager::getFocalPoint(void) const
{
  return SbVec3f(0.0f, 0.0f, 0.0f);
}

void
SoCameraManager::copyLimits(const SoCameraManager * other)
{
  this->minzoom = other->minzoom;
  this->maxzoom = other->maxzoom;
  this->havezoomlimits = other->havezoomlimits;
  this->mindollydistance = other->mindollydistance;
  this->maxdollydistance = other->maxdollydistance;
  this->unitydistance = other->unitydistance;
  this->havezoombydollylimits = other->havezoombydollylimits;
}

// *************************************************************************

/*
  \class SoOrthoPerspectiveCameraManager

  This class is about managing an SoOrthographicCamera as if it was an
  SoPerspectiveCamera.  The defined behaviour is to have the scale of the model
  identical in the plane in the focal point, based on the given zoom value.
*/

SoOrthoPerspectiveCameraManager::SoOrthoPerspectiveCameraManager(SoCamera * camera)
: SoCameraManager(camera)
{
}

SoOrthographicCamera *
SoOrthoPerspectiveCameraManager::getCastCamera(void) const
{
  assert(this->getCamera() && this->getCamera()->isOfType(SoOrthographicCamera::getClassTypeId()));
  return static_cast<SoOrthographicCamera *>(this->getCamera());
}

void
SoOrthoPerspectiveCameraManager::setZoomValue(float zoomvalue, SbBool limit)
{
  static const float defaultangle = float(M_PI_4);
  static const float defaultheight = sin(defaultangle) / cos(defaultangle);

  SoOrthographicCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();

  if (zoomvalue < 0.0f) {
    zoomvalue = 0.0f;
  }

  if (limit && this->havezoomlimits) {
    if (zoomvalue < this->minzoom) {
      zoomvalue = this->minzoom;
    } else if (zoomvalue > this->maxzoom) {
      zoomvalue = this->maxzoom;
    }
  }

  float unityheight = defaultheight * focaldistance;
  float zoomedheight = unityheight / zoomvalue;

  camera->height.setValue(zoomedheight);
}

void
SoOrthoPerspectiveCameraManager::adjustZoomValue(float diffvalue, SbBool limit)
{
  float currentfactor = this->getZoomFactor();
  float newfactor = currentfactor + diffvalue;
  this->setZoomValue(newfactor, limit);
}

void
SoOrthoPerspectiveCameraManager::adjustZoom(float factor, SbBool limit)
{
  float currentfactor = this->getZoomFactor();
  float newfactor = currentfactor * factor;
  this->setZoomValue(newfactor, limit);
}

float
SoOrthoPerspectiveCameraManager::getZoomFactor(void) const
{
  static const float defaultangle = float(M_PI_4);
  static const float defaultheight = sin(defaultangle) / cos(defaultangle);

  SoOrthographicCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();

  float unityheight = defaultheight * focaldistance;
  float height = camera->height.getValue();
  float zoomvalue = unityheight / height;

  return zoomvalue;
}

/*
  Performs a dolly and at the same time adjusts height to emulate how a perspective
  camera would zoom in
*/
void
SoOrthoPerspectiveCameraManager::setZoomValueByDolly(float zoomvalue, SbBool limit)
{
  if (!this->havezoombydollylimits) {
    // don't have the necessary information
    return;
  }

  static const float defaultangle = float(M_PI_4);
  static const float defaultheight = sin(defaultangle) / cos(defaultangle);

  // these are absolute, and wipes out existing non-dollied zoom. should fix
  float newdistance = 0.0f;
  if (zoomvalue < 0.0) {
    zoomvalue = 0.0;
    newdistance = 0.0;
  } else {
    newdistance = this->unitydistance / zoomvalue;
    if (limit) {
      if (newdistance < this->mindollydistance) {
        newdistance = this->mindollydistance;
      }
      else if (newdistance > this->maxdollydistance) {
        newdistance = this->maxdollydistance;
      }
    }
    if (newdistance < 0.0) { newdistance = 0.0; }
  }

  float newheight = newdistance * defaultheight;

  SoOrthographicCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();
  SbVec3f projdir(0.0f, 0.0f, 0.0f);
  camera->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), projdir);
  SbVec3f focalpoint = camera->position.getValue() + projdir * focaldistance;
  camera->position.setValue(focalpoint - projdir * newdistance);
  camera->focalDistance.setValue(newdistance);
  camera->height.setValue(newheight);

  //printf("setting zoom:%g, distance:%g height=%g\n", zoomvalue, newdistance, newheight);
}

void
SoOrthoPerspectiveCameraManager::adjustZoomByDollyDistance(float distance, SbBool limit)
{
  SoOrthographicCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();

  float newfocaldistance = focaldistance + distance;
  this->adjustZoomByDolly(newfocaldistance / focaldistance, limit);
}

void
SoOrthoPerspectiveCameraManager::adjustZoomByDolly(float factor, SbBool limit)
{
  float currentfactor = this->getZoomByDollyFactor();
  this->setZoomValueByDolly(currentfactor * factor, limit);
}

float
SoOrthoPerspectiveCameraManager::getZoomByDollyFactor(void) const
{
  if (!this->havezoombydollylimits) {
    // without knowing unitydistance, nothing can be said about this
    return 1.0f;
  }
  SoOrthographicCamera * camera = this->getCastCamera();
  return (this->unitydistance / camera->focalDistance.getValue());
}


// *************************************************************************

SoPerspectiveCameraManager::SoPerspectiveCameraManager(SoCamera * camera)
: SoCameraManager(camera)
{
}

SoPerspectiveCamera *
SoPerspectiveCameraManager::getCastCamera(void) const
{
  assert(this->getCamera() && this->getCamera()->isOfType(SoPerspectiveCamera::getClassTypeId()));
  return static_cast<SoPerspectiveCamera *>(this->getCamera());
}


void
SoPerspectiveCameraManager::setZoomValue(float inzoomvalue, SbBool limit)
{
  // default* defines what zoom=1.0 is
  static const float defaultangle = float(M_PI_4);
  static const float defaultheight = sin(defaultangle) / cos(defaultangle);

  float zoomvalue = inzoomvalue;
  if (limit) {
    if (zoomvalue < this->minzoom) { zoomvalue = this->minzoom; }
    else if (zoomvalue > this->maxzoom) { zoomvalue = this->maxzoom; }
  }
  float zoomedheight = defaultheight / zoomvalue;
  float zoomedangle = asin(zoomedheight / sqrt(1.0f + (zoomedheight * zoomedheight)));
  //float height = sin(zoomedangle) / cos(zoomedangle);

  // printf("angle is set to %g (default %g/%g), height = %g (default %g), zoom = %g\n",
  //        angle, defaultangle, mydefaultangle, height, defaultheight, inzoomvalue);

  SoPerspectiveCamera * camera = this->getCastCamera();
  camera->heightAngle.setValue(2.0f * zoomedangle);
  // we multiply by two to get full angle, not just with projection direction

  //const float calculated = this->getZoomFactor();
  //printf("clamped zoom %g, recomputed to %g\n", zoomvalue, calculated);

}

void
SoPerspectiveCameraManager::adjustZoomValue(float diffvalue, SbBool limit)
{
  float zoomfactor = this->getZoomFactor();
  float newzoomfactor = zoomfactor + diffvalue;
  this->setZoomValue(newzoomfactor, limit);
}

void
SoPerspectiveCameraManager::adjustZoom(float factor, SbBool limit)
{
  float zoomfactor = this->getZoomFactor();
  float newzoomfactor = zoomfactor * factor;
  this->setZoomValue(newzoomfactor, limit);
}

float
SoPerspectiveCameraManager::getZoomFactor(void) const
{
  // zoombydolly does not affect angle for SoPerspectiveCamera handling, so
  // there is no need to compensate for that here.
  static const float defaultangle = float(M_PI_4);
  static const float defaultheight = sin(defaultangle) / cos(defaultangle);

  SoPerspectiveCamera * camera = this->getCastCamera();
  const float angle = camera->heightAngle.getValue() * 0.5f;
  const float height = sin(angle) / cos(angle);

  const float zoomfactor = defaultheight / height;

  return zoomfactor;
}

void
SoPerspectiveCameraManager::setZoomValueByDolly(float zoomvalue, SbBool COIN_UNUSED_ARG(limit))
{
  if (!this->havezoombydollylimits) {
    // without unitydistance, we don't know anything
    return;
  }

  SoPerspectiveCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();
  SbVec3f projdir(0.0f, 0.0f, 0.0f);
  camera->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), projdir);
  SbVec3f focalpoint = camera->position.getValue() + projdir * focaldistance;

  float newdistance = this->unitydistance / zoomvalue;
  camera->position.setValue(focalpoint - projdir * newdistance);
  camera->focalDistance.setValue(newdistance);
}

void
SoPerspectiveCameraManager::adjustZoomByDollyDistance(float distance, SbBool limit)
{
  SoPerspectiveCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();
  float newdistance = focaldistance + distance;
  if (this->havezoombydollylimits && limit) {
    if (newdistance < this->mindollydistance) {
      newdistance = this->mindollydistance;
    }
    else if (newdistance > this->maxdollydistance) {
      newdistance = this->maxdollydistance;
    }
  }
  SbVec3f projdir(0.0f, 0.0f, 0.0f);
  camera->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), projdir);
  SbVec3f focalpoint = camera->position.getValue() + projdir * focaldistance;
  camera->position.setValue(focalpoint - projdir * newdistance);
  camera->focalDistance.setValue(newdistance);
}

void
SoPerspectiveCameraManager::adjustZoomByDolly(float factor, SbBool limit)
{
  SoPerspectiveCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();
  float newdistance = focaldistance * (1.0f / factor);
  if (this->havezoombydollylimits && limit) {
    if (newdistance < this->mindollydistance) {
      newdistance = this->mindollydistance;
    }
    else if (newdistance > this->maxdollydistance) {
      newdistance = this->maxdollydistance;
    }
  }
  SbVec3f projdir(0.0f, 0.0f, 0.0f);
  camera->orientation.getValue().multVec(SbVec3f(0.0f, 0.0f, -1.0f), projdir);
  SbVec3f focalpoint = camera->position.getValue() + projdir * focaldistance;
  camera->position.setValue(focalpoint - projdir * newdistance);
  camera->focalDistance.setValue(newdistance);
}

float
SoPerspectiveCameraManager::getZoomByDollyFactor(void) const
{
  if (!this->havezoombydollylimits) {
    // without unitydistance, we don't know anything
    return 1.0f;
  }

  SoPerspectiveCamera * camera = this->getCastCamera();
  float focaldistance = camera->focalDistance.getValue();
  float zoomfactor = this->unitydistance / focaldistance;

  return zoomfactor;
}

// *************************************************************************

// stub for now

SoFrustumCameraManager::SoFrustumCameraManager(SoCamera * camera)
: SoCameraManager(camera)
{
}

SoFrustumCamera *
SoFrustumCameraManager::getCastCamera(void) const
{
  assert(getCamera() && getCamera()->isOfType(SoFrustumCamera::getClassTypeId()));
  return static_cast<SoFrustumCamera *>(this->getCamera());
}


void
SoFrustumCameraManager::setZoomValue(float COIN_UNUSED_ARG(zoomvalue), SbBool COIN_UNUSED_ARG(limit))
{
}

void
SoFrustumCameraManager::adjustZoomValue(float COIN_UNUSED_ARG(diffvalue), SbBool COIN_UNUSED_ARG(limit))
{
}

void
SoFrustumCameraManager::adjustZoom(float COIN_UNUSED_ARG(factor), SbBool COIN_UNUSED_ARG(limit))
{
}

float
SoFrustumCameraManager::getZoomFactor(void) const
{
  return 1.0f;
}


void
SoFrustumCameraManager::setZoomValueByDolly(float COIN_UNUSED_ARG(zoomvalue), SbBool COIN_UNUSED_ARG(limit))
{
}

void
SoFrustumCameraManager::adjustZoomByDollyDistance(float COIN_UNUSED_ARG(distance), SbBool COIN_UNUSED_ARG(limit))
{
}

void
SoFrustumCameraManager::adjustZoomByDolly(float COIN_UNUSED_ARG(factor), SbBool COIN_UNUSED_ARG(limit))
{
}

float
SoFrustumCameraManager::getZoomByDollyFactor(void) const
{
  return 1.0f;
}

// *************************************************************************
