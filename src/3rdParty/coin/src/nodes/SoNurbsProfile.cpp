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
  \class SoNurbsProfile SoNurbsProfile.h Inventor/nodes/SoNurbsProfile.h
  \brief The SoNurbsProfile class is a node for specifying smooth profile curves.

  \ingroup coin_nodes

  Use nodes of this type if you want to set up profiles that are
  smooth curves.

  Use ProfileCoordinate2 for nonrational profile curves where weight is 1.0 (default),
  and ProfileCoordinate3 for rational profile curves to specify the weight.
  Weight is analogous to having magnets pulling on the curve.

  A typical usage case for SoNurbsProfile is to specify NURBS trimming
  curves. For example:

  \code
  #Inventor V2.1 ascii

  ShapeHints {
    vertexOrdering COUNTERCLOCKWISE
  }

  Coordinate3 {
    point [ 
      -3 -3 -3, -3 -1 -3, -3 1 -3, -3 3 -3,
      -1 -3 -3, -1 -1  3, -1 1  3, -1 3 -3,
       1 -3 -3,  1 -1  3,  1 1  3,  1 3 -3,
       3 -3 -3,  3 -1 -3,  3 1 -3,  3 3 -3
     ]
  }

  ProfileCoordinate2 {
    point [ 0.0 0.0 ,
            0.75 0.0,
            0.75 0.75 ,
            0.25 0.75 ,
            0.0 0.0  ]
  }

  NurbsProfile {
     index [ 0 , 1 , 2 , 3, 4 ]
     linkage START_NEW
     knotVector [ 0, 0, 0, 0, 0.5, 1, 1, 1, 1 ]
  }

  NurbsSurface {
    numUControlPoints 4
    numVControlPoints 4
    uKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]
    vKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]
  }
  \endcode

  <center>
  \image html nurbsprofile.png "Rendering of Example Scenegraph"
  </center>

  Note that the coordinates of the NurbsProfile live in the parametric
  space of the trimmed SoNurbsSurface, and that the same complexity
  setting (which is calculated based on the dimensions of the bounding
  box of the NurbsSurface) is used to determine the sampling
  tolerance both for the SoNurbsSurface and the SoNurbsProfile.

  This means that if you want to change the tessellation of the
  trimming curve <i>itself</i> (i.e. increase or decrease the
  resolution of the boundaries of the "cut-out"), you should not
  change the SoComplexity setting but rather adapt the parametric
  scale in relation to the trimmed surface.

  As an example, to increase the resolution of the curve in the above
  example, replace...

  \code
  ProfileCoordinate2 {
    point [ 0.0 0.0, 0.75 0.0, 0.75 0.75, 0.25 0.75, 0.0 0.0 ]
  }
  \endcode

  .. with... 

  \code
  ProfileCoordinate2 {
    point [ 0.0 0.0, 7.5 0.0, 7.5 7.5, 2.5 7.5, 0.0 0.0 ]
  }
  \endcode

  and change the uKnotVector and vKnotVector of the NurbsSurface to be

  \code
    uKnotVector [ 0.0, 0.0, 0.0, 0.0, 10, 10, 10, 10 ]
    vKnotVector [ 0.0, 0.0, 0.0, 0.0, 10, 10, 10, 10 ]
  \endcode

  However, keep in mind that increasing the accuracy of the trimming
  curve results in a much more complex tessellation of the trimmed
  surface. As a general rule of thumb, the extent of the trimming
  curve coordinates should never be greater than its "real" extents in
  relation to the trimmed surface, and often can be much lower.

  If you find the above confusing, you probably do not want to use
  NURBS without reading up on the general concepts first.  An
  explanation of NURBS is beyond the scope of the Coin documentation;
  for detailed information, refer to the specialized literature on the
  topic (for example "An Introduction to NURBS: With Historical
  Perspective" by David F. Rogers). A basic overview of curve and
  surface rendering using NURBS can also be found in chapter 8 of "The
  Inventor Mentor".

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    NurbsProfile {
        index 0
        linkage START_FIRST
        knotVector 0
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoNurbsProfile.h>

#include <cstdlib>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbStorage.h>

#include "nodes/SoSubNodeP.h"
#include "glue/GLUWrapper.h"
#include "tidbitsp.h"

// *************************************************************************

/*!
  \var SoMFFloat SoNurbsProfile::knotVector
  Knot values for the NURBS curve.
*/

// *************************************************************************

typedef struct {
  SbList <float> * coordlist;
  SbList <float> * tmplist;
} so_nurbsprofile_data;

static void
so_nurbsprofile_construct_data(void * closure)
{
  so_nurbsprofile_data * data = (so_nurbsprofile_data*) closure;
  data->coordlist = NULL;
  data->tmplist = NULL;
}

static void
so_nurbsprofile_destruct_data(void * closure)
{
  so_nurbsprofile_data * data = (so_nurbsprofile_data*) closure;
  delete data->coordlist;
  delete data->tmplist;
}

static SbStorage * so_nurbsprofile_storage;

static void
so_nurbsprofile_cleanup(void)
{
  delete so_nurbsprofile_storage;
}

static SbList <float> *
so_nurbsprofile_get_coordlist(const SbBool tmplist)
{
  so_nurbsprofile_data * data = NULL;
  data = (so_nurbsprofile_data*) so_nurbsprofile_storage->get();

  if (tmplist) {
    if (data->tmplist == NULL) {
      data->tmplist = new SbList<float>;
    }
    return data->tmplist;
  }
  else {
    if (data->coordlist == NULL) {
      data->coordlist = new SbList<float>;
    }
    return data->coordlist;
  }
}

// *************************************************************************

SO_NODE_SOURCE(SoNurbsProfile);

/*!
  Constructor.
*/
SoNurbsProfile::SoNurbsProfile(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNurbsProfile);

  SO_NODE_ADD_FIELD(knotVector, (0.0f));
  this->nurbsrenderer = NULL;
}

/*!
  Destructor.
*/
SoNurbsProfile::~SoNurbsProfile()
{
  if (this->nurbsrenderer) {
    GLUWrapper()->gluDeleteNurbsRenderer(this->nurbsrenderer);
  }
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoNurbsProfile::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoNurbsProfile, SO_FROM_INVENTOR_1);
  so_nurbsprofile_storage = new SbStorage(sizeof(so_nurbsprofile_data),
                                          so_nurbsprofile_construct_data,
                                          so_nurbsprofile_destruct_data);
  coin_atexit((coin_atexit_f*) so_nurbsprofile_cleanup, CC_ATEXIT_NORMAL);
}

// Doc from superclass.
void
SoNurbsProfile::getTrimCurve(SoState * state, int32_t & numpoints,
                             float *& points, int & floatspervec,
                             int32_t & numknots, float *& knotvector)
{
  SbList <float> * coordListNurbsProfile =
    so_nurbsprofile_get_coordlist(FALSE);

  numknots = this->knotVector.getNum();
  if (numknots) knotvector = (float *)(this->knotVector.getValues(0));

  const SoProfileCoordinateElement * elem = (const SoProfileCoordinateElement*)
    SoProfileCoordinateElement::getInstance(state);

  coordListNurbsProfile->truncate(0);

  // Get the number of SoProfileCoordinate2/3 points
  int32_t numcoords = elem->getNum();
  // Get the number of profile coordinate indices
  int n = this->index.getNum();

  if (numcoords) {
    // Both 2D or 3D profile coordinates might have been specified, so
    // get the appropriate coordinates and save the number of floats
    // per vector for later usage.
    if (elem->is2D()) {
      points = (float*) elem->getArrayPtr2();
      floatspervec = 2;
    }
    else {
      points = (float*) elem->getArrayPtr3();
      floatspervec = 3;
    }

    assert(points);
  }
  
  // Append the coordinates to a list over the profile coordinates.
  for (int i = 0; i < n; i++) {
    int idx = this->index[i];

    // If valid profile coordinates have been specified
    if (idx >= 0 && idx < numcoords) {
      for (int j = 0; j < floatspervec; j++) {
        coordListNurbsProfile->append(points[(idx * floatspervec) + j]);
      }
    }
    // If invalid profile coordinates have been specified
    else {
      // Add dummy coordinate for robustness
      for (int j = 0; j < floatspervec; j++) {
        coordListNurbsProfile->append(0.0f);
      }
      
      // Print errormessage
      static uint32_t current_errors = 0;
      if (current_errors < 1) {
        SoDebugError::postWarning("SoNurbsProfile::getTrimCurve", "Illegal profile "
                                  "coordinate index specified: %d. Should be within "
                                  "[0, %d]", idx, numcoords - 1);
      }
      current_errors++;
    }
  }

  points = (float*) coordListNurbsProfile->getArrayPtr();
  numpoints = n;
}


static void APIENTRY
nurbsprofile_tess_vertex(float * vertex)
{
  SbList <float> * coordListNurbsProfile =
    so_nurbsprofile_get_coordlist(FALSE);

  coordListNurbsProfile->append(vertex[0]);
  coordListNurbsProfile->append(vertex[1]);
}

// doc from superclass.
void
SoNurbsProfile::getVertices(SoState * state, int32_t & numvertices,
                            SbVec2f * & vertices)
{
  // FIXME: optimize by detecting when the previously calculated
  // vertices can be returned. pederb, 20000922
  int32_t numpoints;
  float * points;
  int floatspervec;
  int32_t numknots;
  float * knotvector;
  this->getTrimCurve(state, numpoints, points, floatspervec, numknots, knotvector);
  if (numpoints == 0 || numknots == 0) {
    numvertices = 0;
    vertices = NULL;
    return;
  }

  SbList <float> * coordListNurbsProfile =
    so_nurbsprofile_get_coordlist(FALSE);

  SbList <float> * nurbsProfileTempList =
    so_nurbsprofile_get_coordlist(TRUE);

  nurbsProfileTempList->truncate(0);
  for (int i = 0; i < numpoints; i++) {
    nurbsProfileTempList->append(points[i*floatspervec]);
    nurbsProfileTempList->append(points[i*floatspervec+1]);
    if (GLUWrapper()->available &&
        GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
      nurbsProfileTempList->append(0.0f); // gluNurbs needs 3D coordinates
    }
  }
  if (GLUWrapper()->available &&
      GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // we will write into this array in the GLU callback
    coordListNurbsProfile->truncate(0);

    if (this->nurbsrenderer == NULL) {
      this->nurbsrenderer = GLUWrapper()->gluNewNurbsRenderer();
      GLUWrapper()->gluNurbsCallback(this->nurbsrenderer, (GLenum) GLU_NURBS_VERTEX,
                                     (gluNurbsCallback_cb_t)nurbsprofile_tess_vertex);
      GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);
      GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_AUTO_LOAD_MATRIX, FALSE);
      GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_DISPLAY_MODE, GLU_POINT);
      GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
    }

    // this looks pretty good
    float cmplx = SoComplexityElement::get(state);
    cmplx += 1.0f;
    cmplx = cmplx * cmplx * cmplx;
    GLUWrapper()->gluNurbsProperty(this->nurbsrenderer, (GLenum) GLU_U_STEP, float(numpoints)*cmplx);

    // these values are not important as we're not using screen space
    // complexity (yet)
    SbMatrix modelmatrix = SbMatrix::identity();
    SbMatrix affine, proj;
    SbViewVolume vv;
    vv.ortho(0.0f, 1.0f,
             0.0f, 1.0f,
             -1.0f, 1.0f);
    vv.getMatrices(affine, proj);
    GLint viewport[4];
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = 256;
    viewport[3] = 256;
    GLUWrapper()->gluLoadSamplingMatrices(this->nurbsrenderer,
                                          modelmatrix[0],
                                          proj[0],
                                          viewport);

    // generate curve
    GLUWrapper()->gluBeginCurve(this->nurbsrenderer);
    GLUWrapper()->gluNurbsCurve(this->nurbsrenderer,
                                numknots,
                                (float*)knotvector,
                                3,
                                (float*)nurbsProfileTempList->getArrayPtr(),
                                numknots - numpoints,
                                GL_MAP1_VERTEX_3);
    GLUWrapper()->gluEndCurve(this->nurbsrenderer);

    // when we get here, the GLU callback should have added the
    // points to the list
    numvertices = coordListNurbsProfile->getLength() / 2;
    vertices = (SbVec2f*) coordListNurbsProfile->getArrayPtr();
  }
  else {
    // just send the control points when GLU v1.3 is not available
    numvertices = numpoints;
    vertices = (SbVec2f*) nurbsProfileTempList->getArrayPtr();
  }
}
