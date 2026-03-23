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

#include "rendering/SoGLNurbs.h"
#include <Inventor/threads/SbStorage.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>

#include <Inventor/actions/SoAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoProfile.h>
#include <Inventor/SbVec4f.h>
#include "glue/GLUWrapper.h"
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbStorage.h>
#include "tidbitsp.h"
#include "glue/glp.h"

#include <cstdlib>

SbBool
sogl_calculate_nurbs_normals()
{
  static int calculatenurbsnormals = -1;
  if (calculatenurbsnormals == -1) {
    const char * env = coin_getenv("COIN_CALCULATE_NURBS_NORMALS");
    calculatenurbsnormals = env ? atoi(env) : 1;
  }
  return calculatenurbsnormals ? TRUE : FALSE;
}

namespace {
  SbStorage * sogl_coordstorage = NULL;
  SbStorage * sogl_texcoordstorage = NULL;
  SbStorage * sogl_normalstorage = NULL;

  void nurbs_coord_cleanup(void)
  {
    delete sogl_coordstorage;
    sogl_coordstorage = NULL;
  }

  void nurbs_texcoord_cleanup(void)
  {
    delete sogl_texcoordstorage;
    sogl_texcoordstorage = NULL;
  }

  void nurbs_normal_cleanup(void)
  {
    delete sogl_normalstorage;
    sogl_normalstorage = NULL;
  }

  void sogl_alloc_coords(void * ptr)
  {
    SbList <float> ** cptr = (SbList <float> **) ptr;
    *cptr = new SbList <float>;
  }

  void sogl_dealloc_coords(void * ptr)
  {
    SbList <float> ** cptr = (SbList <float> **) ptr;
    delete *cptr;
  }

  SbList <float> *
  sogl_get_tmpcoordlist(void)
  {
    if (sogl_coordstorage == NULL) {
      sogl_coordstorage = new SbStorage(sizeof(void*), sogl_alloc_coords, sogl_dealloc_coords);
      coin_atexit((coin_atexit_f *)nurbs_coord_cleanup, CC_ATEXIT_NORMAL);
    }
    SbList <float> ** ptr = (SbList <float> **) sogl_coordstorage->get();
    return *ptr;
  }

  SbList <float> *
  sogl_get_tmptexcoordlist(void)
  {
    if (sogl_texcoordstorage == NULL) {
      sogl_texcoordstorage = new SbStorage(sizeof(void*), sogl_alloc_coords, sogl_dealloc_coords);
      coin_atexit((coin_atexit_f *)nurbs_texcoord_cleanup, CC_ATEXIT_NORMAL);
    }
    SbList <float> ** ptr = (SbList <float> **) sogl_texcoordstorage->get();
    return *ptr;
  }

  SbList <float> *
  sogl_get_tmpnormallist(void)
  {
    if (sogl_normalstorage == NULL) {
      sogl_normalstorage = new SbStorage(sizeof(void*), sogl_alloc_coords, sogl_dealloc_coords);
      coin_atexit((coin_atexit_f *)nurbs_normal_cleanup, CC_ATEXIT_NORMAL);
    }
    SbList <float> ** ptr = (SbList <float> **) sogl_normalstorage->get();
    return *ptr;
  }

  // Toggle extra debugging output for nurbs complexity settings code.
  SbBool
  sogl_nurbs_debugging(void)
  {
    static int COIN_DEBUG_NURBS_COMPLEXITY = -1;
    if (COIN_DEBUG_NURBS_COMPLEXITY == -1) {
      const char * str = coin_getenv("COIN_DEBUG_NURBS_COMPLEXITY");
      COIN_DEBUG_NURBS_COMPLEXITY = str ? atoi(str) : 0;
    }
    return (COIN_DEBUG_NURBS_COMPLEXITY == 0) ? FALSE : TRUE;
  }

  void
  sogl_set_nurbs_complexity(SoAction * action, SoShape * shape, void * nurbsrenderer,
                            int uIsLinear, int vIsLinear, int numuctrlpts, int numvctrlpts, int uIsClosed, int vIsClosed, float uSpan, float vSpan)
  {
    SoState * state = action->getState();

    float complexity = SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);

    if (!GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
      // GLU < 1.3 does not support view-independent error metrics
      // for tessellation accuracy. => Fall back to pixel-based metric.

      // Settings chosen by visual inspection of same sample curves and
      // surfaces, and comparison with the result of using the object-
      // space metric. The -0.5 is there because for an SoComplexity
      // value of 1, we should have an error of less than one pixel.
      complexity = float(1.0/(complexity*complexity) - 0.5);
      if (complexity < 0.5f) complexity = 0.5f;

      GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                     (GLenum) GLU_SAMPLING_METHOD,
                                     GLU_PARAMETRIC_ERROR);
      GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                     (GLenum) GLU_PARAMETRIC_TOLERANCE,
                                     complexity);

      static SbBool first = TRUE;
      if (sogl_nurbs_debugging() && first) {
        first = FALSE;
        SoDebugError::postInfo("sogl_set_nurbs_complexity",
                               "sampling method = GLU_PARAMETRIC_ERROR, "
                               "GLU_PARAMETRIC_TOLERANCE = %.4f",
                               complexity);
      }
      return;
    }

    static int oldnurbscomplexity = -1;
    
    // don't enable the new complexity algorithm for SCREEN_SPACE yet
    // (unless the user sets it to off) since it's basically the same as
    // OBJECT_SPACE. However, using SCREEN_SPACE complexity for an
    // object built up from multiple patches is not recommended, since
    // you'll get polygon cracks in the seams between the patches (the
    // bounding box for each patch is used for calculating the
    // complexity, they should really have been using the same complexity).
    if (oldnurbscomplexity == -1){
      const char * env = coin_getenv("COIN_OLD_NURBS_COMPLEXITY");
      oldnurbscomplexity = env ? atoi(env) : -2;
    }
    if ((oldnurbscomplexity > 0) || 
        ((oldnurbscomplexity == -2) && 
         (SoComplexityTypeElement::get(state) == SoComplexityTypeElement::SCREEN_SPACE))) {
      switch (SoComplexityTypeElement::get(state)) {
      case SoComplexityTypeElement::SCREEN_SPACE:
        {
          SbBox3f box;
          SbVec3f center;
          shape->computeBBox(action, box, center);
          float diag;
          {
            float dx, dy, dz;
            box.getSize(dx, dy, dz);
            diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
            if (diag == 0.0f) diag = 1.0f;
          }
          SbVec2s size;
          SoShape::getScreenSize(state, box, size);
          float maxpix = (float) SbMax(size[0], size[1]);
          if (maxpix < 1.0f) maxpix = 1.0f;
          float complexity = SoComplexityElement::get(state);
          if (complexity < 0.0001f) complexity = 0.0001f;
          complexity *= maxpix;
          complexity = diag * 0.5f / complexity;

          static SbBool first = TRUE;
          if (sogl_nurbs_debugging() && first) {
            first = FALSE;
            SoDebugError::postInfo("sogl_set_nurbs_complexity",
                                   "sampling method = GLU_OBJECT_PARAMETRIC_ERROR,"
                                   " GLU_PARAMETRIC_TOLERANCE = %.4f",
                                   complexity);
          }
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_SAMPLING_METHOD,
                                         GLU_OBJECT_PARAMETRIC_ERROR);
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_PARAMETRIC_TOLERANCE,
                                         complexity);
          break;
        }
      case SoComplexityTypeElement::OBJECT_SPACE:
        {
          float diag;
          {
            SbBox3f box;
            SbVec3f center;
            shape->computeBBox(action, box, center);
            float dx, dy, dz;
            box.getSize(dx, dy, dz);
            diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
            if (diag == 0.0f) diag = 1.0f;
          }
          float complexity = SoComplexityElement::get(state);
          complexity *= complexity;
          if (complexity < 0.0001f) complexity = 0.0001f;
          complexity = diag * 0.01f / complexity;

          static SbBool first = TRUE;
          if (sogl_nurbs_debugging() && first) {
            first = FALSE;
            SoDebugError::postInfo("sogl_set_nurbs_complexity",
                                   "sampling method = GLU_OBJECT PARAMETRIC_ERROR,"
                                   " GLU_PARAMETRIC_TOLERANCE = %.4f",
                                   complexity);
          }

          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_SAMPLING_METHOD,
                                         GLU_OBJECT_PARAMETRIC_ERROR);
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_PARAMETRIC_TOLERANCE,
                                         complexity);
          break;
        }
      case SoComplexityTypeElement::BOUNDING_BOX:
        assert(0 && "should never get here");
        break;

      default:
        assert(0 && "unknown complexity type");
        break;

      }
    }
    else { // new nurbs complexity
      switch (SoComplexityTypeElement::get(state)) {
      case SoComplexityTypeElement::SCREEN_SPACE:
        {
          float tolerance;
          if      ( complexity < 0.10 ) tolerance = 10;
          else if ( complexity < 0.20 ) tolerance = 8;
          else if ( complexity < 0.30 ) tolerance = 6;
          else if ( complexity < 0.40 ) tolerance = 4;
          else if ( complexity < 0.50 ) tolerance = 2;
          else if ( complexity < 0.70 ) tolerance = 1;
          else if ( complexity < 0.80 ) tolerance = .5;
          else if ( complexity < 0.90 ) tolerance = .25;
          else                          tolerance = .125;

          static SbBool first = TRUE;
          if (sogl_nurbs_debugging() && first) {
            first = FALSE;
            SoDebugError::postInfo("sogl_set_nurbs_complexity",
                                   "sampling method = GLU_PARAMETRIC_ERROR,"
                                   " GLU_PARAMETRIC_TOLERANCE = %.4f",
                                   tolerance);
          }
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_SAMPLING_METHOD,
                                         GLU_PARAMETRIC_ERROR);
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_PARAMETRIC_TOLERANCE,
                                         tolerance);
          break;
        }
      case SoComplexityTypeElement::OBJECT_SPACE:
        {
          // Find the number of steps required for object space tessellation.
          //
          int srfSteps;
          if      ( complexity < 0.10 ) srfSteps = 2;
          else if ( complexity < 0.25 ) srfSteps = 3;
          else if ( complexity < 0.40 ) srfSteps = 4;
          else if ( complexity < 0.55 ) srfSteps = 5;
          else                          srfSteps = (int)(pow(complexity, 3.32f)*28) + 2;

          int crvSteps;
          if ( complexity < 0.5 ) crvSteps = (int)(18*complexity) + 1;
          else                    crvSteps = (int)(380*complexity) - 180;

          static int reducelinearnurbssteps = -1;
          if (reducelinearnurbssteps == -1) {
            const char * env = coin_getenv("COIN_REDUCE_LINEAR_NURBS_STEPS");
            reducelinearnurbssteps = env ? atoi(env) : 1;
          }
          //
          // Set the sampling to be constant across the surface with the
          // tessellation to be 'steps' across the U and V parameters
          //
          int nusteps;
          int nvsteps;
          if (reducelinearnurbssteps && uIsLinear) nusteps = 1;
          else if (uIsClosed) nusteps = srfSteps*4;
          else nusteps = (numuctrlpts < 4 ? 1 : numuctrlpts - 3)*srfSteps+1;

          if (reducelinearnurbssteps && vIsLinear) nvsteps = 1;
          else if (vIsClosed) nvsteps = srfSteps*4;
          else nvsteps = (numvctrlpts < 4 ? 1 : numvctrlpts - 3)*srfSteps+1;

          // it's a curve, not a surface
          if (!numvctrlpts)
            nvsteps = nusteps = uIsClosed ? srfSteps*4-1 : (numuctrlpts < 4 ? 1 : numuctrlpts - 3)*crvSteps;

          nusteps = int(nusteps/uSpan);
          if ( numvctrlpts )
            nvsteps = int(nvsteps/vSpan);
          else
            nvsteps = nusteps;

          static SbBool first = TRUE;
          if (sogl_nurbs_debugging() && first) {
            first = FALSE;
            SoDebugError::postInfo("sogl_set_nurbs_complexity",
                                   "sampling method = GLU_DOMAIN_DISTANCE,"
                                   " GLU_U_STEP = %d",
                                   " GLU_V_STEP = %d",
                                   nusteps, nvsteps);
          }
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_SAMPLING_METHOD,
                                         GLU_DOMAIN_DISTANCE);
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_U_STEP,
                                         (GLfloat) nusteps);
          GLUWrapper()->gluNurbsProperty(nurbsrenderer,
                                         (GLenum) GLU_V_STEP,
                                         (GLfloat) nvsteps);
          break;
        }
      case SoComplexityTypeElement::BOUNDING_BOX:
        assert(0 && "should never get here");
        break;
      default:
        assert(0 && "unknown complexity type");
        break;
      }
    }
  }
  class nurbs {
  public:
    nurbs(const int uorder_, const int numuknots_, const float* uknotvec_,
          const int vorder_, const int numvknots_, const float* vknotvec_,
          const int ustride_, const int vstride_, const float* ctlPoints_, const int dim_)
      : uorder(uorder_),
        numuknots(numuknots_),
        uknotvec(uknotvec_),
        vorder(vorder_),
        numvknots(numvknots_),
        vknotvec(vknotvec_),
        ustride(ustride_),
        vstride(vstride_),
        ctlPoints(ctlPoints_),
        dim(dim_)
    {
      udegree = uorder-1;
      vdegree = vorder-1;
      numuctrlpts = numuknots-uorder;
      numvctrlpts = numvknots-vorder;

      int maxorder = SbMax(uorder, vorder);

      left = new float[maxorder];
      right = new float[maxorder];

      ndu = new float*[maxorder];
      ndu[0] = new float[maxorder*maxorder];
      for ( int i = 1; i < maxorder; i++ ) ndu[i] = ndu[0]+i*maxorder;

      a = new float*[2];
      a[0] = new float[2*maxorder];
      a[1] = a[0]+maxorder;

      temp = new SbVec4f[maxorder];

      Nu = new float*[uorder];
      Nu[0] = new float[uorder*uorder];
      for ( int i = 1; i < uorder; i++ ) Nu[i] = Nu[0]+i*uorder;
      Nv = new float*[vorder];
      Nv[0] = new float[vorder*vorder];
      for ( int i = 1; i < vorder; i++ ) Nv[i] = Nv[0]+i*vorder;

      Bin = new float*[maxorder];
      Bin[0] = new float[maxorder*maxorder];
      for ( int i = 1; i < maxorder; i++ ) Bin[i] = Bin[0]+i*maxorder;
    }
    ~nurbs()
    {
      delete [] left;
      delete [] right;
      delete [] ndu[0];
      delete [] ndu;
      delete [] a[0];
      delete [] a;
      delete [] temp;
      delete [] Nu[0];
      delete [] Nu;
      delete [] Nv[0];
      delete [] Nv;
      delete [] Bin[0];
      delete [] Bin;
    }
    void Test();

    void BinomialCoefficients(const int rows, const int cols);
    int FindSpan(const float u, const int degree, const int numctrlpts, const float* knotvec) const;
    void BasisFunctions(const float u, const int span, const float* knotvec, const int order, float* N);
    void DersBasisFunctions(const float u, const int span, const float* knotvec, const int order, const int n, float** ders );
    void RationalSurfaceDerivsH(const float u, const float v, const int d, SbVec4f** skl);
    void RationalSurfaceDerivs(const float u, const float v, const int d, SbVec3f** skl);
    SbVec3f normal(const float u, const float v);
    SbVec3f normal(const int i, const int j);
    SbVec4f SurfacePoint(const float u, const float v);
    SbVec4f SurfacePoint(const int i, const int j);
    void mapijtouv(const int i, const int j, float& u, float &v);

  private:
    const int uorder;
    const int numuknots;
    const float* uknotvec;
    const int vorder;
    const int numvknots;
    const float* vknotvec;
    const int ustride;
    const int vstride;
    const float* ctlPoints;
    const int dim;

    int udegree;
    int numuctrlpts;
    int vdegree;
    int numvctrlpts;

    /// temporary fields
    float* left;
    float* right;
    float** ndu;
    float** a;
    SbVec4f* temp;
    float** Nu;
    float** Nv;
    float** Bin;
  };

  // Setup the binomial coefficients into the matrix Bin
  // Bin(i,j) = (i  j)
  // The binomical coefficients are defined as follow
  //   (n)         n!
  //   (k)  =    k!(n-k)!       0<=k<=n
  // and the following relationship applies
  // (n+1)     (n)   ( n )
  // ( k ) =   (k) + (k-1)
  void nurbs::BinomialCoefficients(const int rows, const int cols) {
    Bin[0][0] = 1.0f;
    for (int k = 1; k < cols; k++)
      Bin[0][k] = 0.0f;

    for ( int n = 0; n < rows-1; n++ ) {
      Bin[n+1][0] = 1.0f;
      for ( int k = 1; k < cols; k++ )
        if ( n+1 < k )
          Bin[n][k] = 0.0f;
        else
          Bin[n+1][k] = Bin[n][k] + Bin[n][k-1];
    }
  }

  /// Find the span in the knot vector containing the specified parameter value using binary search.
  int nurbs::FindSpan(const float u, const int degree, const int numctrlpts, const float* knotvec) const {
    if ( u >= knotvec[numctrlpts] )
      return numctrlpts-1;
    if ( u <= knotvec[degree] )
      return degree;

    int low = 0;
    int high = numctrlpts + 1;
    int mid = (low + high)/2;

    while ( u < knotvec[mid] || u >= knotvec[mid + 1] ) {
      if ( u < knotvec[mid] )
        high = mid;
      else
        low = mid;
      mid = (low + high)/2;
    }
    return mid;
  }

/// Return the basis functions for the specified parameter value.
  void nurbs::BasisFunctions(const float u, const int span, const float* knotvec, const int order, float* N) {
    N[0] = 1.0f;
    for ( int j = 1; j < order; j++ ) {
      left[j] = u - knotvec[span + 1 - j];
      right[j] = knotvec[span + j] - u;
      float saved = 0.0f;
      for ( int r = 0; r < j; r++ )
        {
          float temp = N[r] / (right[r + 1] + left[j - r]);
          N[r] = saved + right[r + 1] * temp;
          saved = left[j - r] * temp;
        }
      N[j] = saved;
    }
  }

  /// Return the basis functions for the specified parameter value.
  void nurbs::DersBasisFunctions(const float u, const int span, const float* knotvec, const int order, const int n, float** ders) {
    int p = order - 1;

    ndu[0][0] = 1.0f;
    for (int j = 1; j < order; j++) {
      left[j] = u - knotvec[span + 1 - j];
      right[j] = knotvec[span + j] - u;
      float saved = 0.0f;
      for ( int r = 0; r < j; r++ ) {
        ndu[j][r] = right[r + 1] + left[j - r];
        float temp = ndu[r][j - 1] / ndu[j][r];

        ndu[r][j] = saved + right[r + 1] * temp;
        saved = left[j - r] * temp;
      }
      ndu[j][j] = saved;
    }

    // Load the basis functions.
    for ( int j = 0; j < order; j++ )
      ders[0][j] = ndu[j][p];
    // Compute the derivatives.
    for ( int r = 0; r < order; r++ ) {
      // Alternate rows in array a.
      int s1 = 0;
      int s2 = 1;
      a[0][0] = 1.0f;
      int j1, j2;

      // Loop to compute the kth derivative.
      for ( int k = 1; k <= n; k++ ) {
        float d = 0.0f;
        int rk = r - k;
        int pk = p - k;
        if ( r >= k ) {
          a[s2][0] = a[s1][0] / ndu[pk + 1][rk];
          d = a[s2][0] * ndu[rk][pk];
        }
        if ( rk >= -1 )
          j1 = 1;
        else
          j1 = -rk;

        if ( r - 1 <= pk )
          j2 = k - 1;
        else
          j2 = p - r;

        for ( int j = j1; j <= j2; j++ ) {
          a[s2][j] = (a[s1][j] - a[s1][j - 1]) / ndu[pk + 1][rk + j];
          d += a[s2][j] * ndu[rk + j][pk];
        }
        if ( r <= pk ) {
          a[s2][k] = -a[s1][k - 1] / ndu[pk + 1][r];
          d += a[s2][k] * ndu[r][pk];
        }
        ders[k][r] = d;

        // Switch rows.
        int t = s1;
        s1 = s2;
        s2 = t;
      }
    }
    // Multiply through the correct factors.
    int l = p;
    for ( int k = 1; k <= n; k++ ) {
      for ( int j = 0; j < order; j++ )
        ders[k][j] *= l;
      l *= p - k;
    }
  }

  /// Evaluate the derivatives of degree d and below at (u,v).
  void nurbs::RationalSurfaceDerivsH(const float u, const float v, const int d, SbVec4f** skl) {
    int du = SbMin( d, udegree );
    // Initialize the requested derivatives greater than the degree in u.
    for ( int k = uorder; k <= d; k++ )
      for ( int l = 0; l <= d - k; l++ )
        skl[k][l] = SbVec4f(0.0f, 0.0f, 0.0f, 1.0f);

    int dv = SbMin( d, vdegree );
    // Initialize the requested derivatives greater than the degree in v.
    for ( int l = vorder; l <= d; l++ )
      for ( int k = 0; k <= d - l; k++ )
        skl[k][l] = SbVec4f(0.0f, 0.0f, 0.0f, 1.0f);

    int uspan = FindSpan( u, udegree, numuctrlpts, uknotvec );
    DersBasisFunctions( u, uspan, uknotvec, uorder, du, Nu );
    int vspan = FindSpan( v, vdegree, numvctrlpts, vknotvec );
    DersBasisFunctions( v, vspan, vknotvec, vorder, dv, Nv );

    for ( int k = 0; k <= du; k++ ) {
      for ( int s = 0; s < vorder; s++ ) {
        temp[s] = SbVec4f(0.0f, 0.0f, 0.0f, 0.0f);
        for ( int r = 0; r < uorder; r++ ) {
          int offs = (uspan - udegree + r)*ustride + (vspan - vdegree + s)*vstride;
          SbVec4f ctlpt4(0.0f, 0.0f, 0.0f, 1.0f);
          for ( int i = 0; i < dim; i++ )
            ctlpt4[i] = ctlPoints[offs+i];
          //char buffer[1024];
          //sprintf(buffer, "point[%d][%d]: %g %g %g %g\n", uspan - udegree + r, vspan - vdegree + s, ctlPoints[offs+0], ctlPoints[offs+1], ctlPoints[offs+2], ctlPoints[offs+3]);
          //OutputDebugString(buffer);
          //SbVec3f ctlpt3;
          //ctlpt4.getReal(ctlpt3);
          //ctlpt4 = SbVec4f(ctlpt3[0], ctlpt3[1], ctlpt3[2], 1.0f);
          temp[s] = temp[s] + Nu[k][r] * ctlpt4;
        }
      }
      int dd = SbMin( d - k, dv );
      for ( int l = 0; l <= dd; l++ ) {
        skl[k][l] = SbVec4f(0.0f, 0.0f, 0.0f, 0.0f);
        for ( int s = 0; s < vorder; s++ ) {
          skl[k][l] = skl[k][l] + Nv[l][s] * temp[s];
          //char buffer[1024];
          //sprintf(buffer, "skl[%d][%d]: %g %g %g %g\ttemp[%d]: %g %g %g %g\tNv[%d][%d]: %g\n", k, l, skl[k][l][0], skl[k][l][1], skl[k][l][2], skl[k][l][3], s, temp[s][0], temp[s][1], temp[s][2], temp[s][3], l, s, Nv[l][s]);
          //OutputDebugString(buffer);
        }
      }
    }
  }

  /// Computes the point and the derivatives of degree d and below at (u,v).
  void nurbs::RationalSurfaceDerivs(float u, float v, const int d, SbVec3f** skl) {
    SbVec4f** ders = new SbVec4f*[d+1];
    ders[0] = new SbVec4f[(d + 1)*(d + 1)];
    for ( int i = 1; i < (d + 1); i++ ) ders[i] = ders[0]+i*(d + 1);

    RationalSurfaceDerivsH( u, v, d, ders );
    BinomialCoefficients( d+1, d+1 );

    for ( int k = 0; k <= d; k++ ) {
      for ( int l = 0; l <= d-k; l++ ) {
        SbVec3f v1(ders[k][l][0], ders[k][l][1], ders[k][l][2]);
        for ( int j = 1; j <= l; j++ )
          v1 = v1 - Bin[l][j]*ders[0][j][3]*skl[k][l-j];
        for ( int i = 1; i <= k; i++ ) {
          SbVec3f v2(0.0f, 0.0f, 0.0f);
          v1 = v1 - Bin[k][i]*ders[i][0][3]*skl[k-i][l];
          for ( int j = 1; j <= l; j++ )
            v2 = v2 + Bin[l][j]*ders[i][j][3]*skl[k-i][l-j];
          v1 = v1 - Bin[k][i]*v2;
        }
        skl[k][l] = v1/ders[0][0][3];
        //char buffer[1024];
        //sprintf(buffer, "skl[%d][%d]: %g %g %g\tpv: %g %g %g\n", k, l, skl[k][l][0], skl[k][l][1], skl[k][l][2], pv[0], pv[1], pv[2]);
        //OutputDebugString(buffer);
      }
    }
    delete [] ders[0];
    delete [] ders;
  }

  /// Computes the normal of the surface at parameters (u,v).
  SbVec3f nurbs::normal(const float u, const float v) {
    int d = 1;

    SbVec3f** ders = new SbVec3f*[d+1];
    ders[0] = new SbVec3f[(d + 1)*(d + 1)];
    for ( int i = 1; i < (d + 1); i++ ) ders[i] = ders[0]+i*(d + 1);
    //SbVec4f** ders = new SbVec4f*[d+1];
    //ders[0] = new SbVec4f[(d + 1)*(d + 1)];
    //for ( int i = 1; i < (d + 1); i++ ) ders[i] = ders[0]+i*(d + 1);

    RationalSurfaceDerivs( u, v, d, ders );
    //RationalSurfaceDerivsH( u, v, d, ders );

    SbVec3f N;

    //SbVec3f d10(ders[1][0][0], ders[1][0][1], ders[1][0][2]), d01(ders[0][1][0], ders[0][1][1], ders[0][1][2]);
    //N = d10.cross( d01 );
    N = ders[1][0].cross( ders[0][1] );

    float mag = N.length();

    //char buffer[1024];
    //sprintf(buffer, "Normal: %g %g %g, magnitude: %g\n", N[0], N[1], N[2], mag);
    //OutputDebugString(buffer);

    if ( mag > 1.e-5f )
      N /= mag;
    else {
      //char buffer[1024];
      //sprintf(buffer, "Set normal to zero (u=%g, v=%g), magnitude: %g\n", u, v, mag);
      //OutputDebugString(buffer);
      N = SbVec3f(0.0f, 0.0f, 0.0f);
    }

    delete [] ders[0];
    delete [] ders;

    return N;
  }

  /// Computes the normal of the surface at indices (i,j).
  SbVec3f nurbs::normal(const int i, const int j) {
    float u, v;
    mapijtouv( i, j, u, v );

    return normal( u, v );
  }

  /// Maps the point indices (i,j) to parameters (u,v).
  void nurbs::mapijtouv(const int i, const int j, float& u, float& v) {
    float deltau = (uknotvec[numuknots-1] - uknotvec[0]) / (numuctrlpts - 1);
    float deltav = (vknotvec[numvknots-1] - vknotvec[0]) / (numvctrlpts - 1);
    u = (i == numuctrlpts-1) ? uknotvec[numuknots-1] : (uknotvec[0] + i*deltau);
    v = (j == numvctrlpts-1) ? vknotvec[numvknots-1] : (vknotvec[0] + j*deltav);
  }

  /// Returns the point on the surface at parameters (u,v).
  SbVec4f nurbs::SurfacePoint(const float u, const float v) {
    int uspan = FindSpan( u, udegree, numuctrlpts, uknotvec );
    BasisFunctions( u, uspan, uknotvec, uorder, Nu[0] );
    int vspan = FindSpan( v, vdegree, numvctrlpts, vknotvec );
    BasisFunctions( v, vspan, vknotvec, vorder, Nv[0] );

    for ( int l = 0; l < vorder; l++ ) {
      temp[l] = SbVec4f(0.0f, 0.0f, 0.0f, 0.0f);
      for ( int k = 0; k < uorder; k++ ) {
        int offs = (uspan - udegree + k)*ustride + (vspan - vdegree + l)*vstride;
        SbVec4f ctlpt4(0.0f, 0.0f, 0.0f, 1.0f);
        for ( int i = 0; i < dim; i++ )
          ctlpt4[i] = ctlPoints[offs+i];
        temp[l] = temp[l] + Nu[0][k]*ctlpt4;
      }
    }
    SbVec4f sp(0.0f, 0.0f, 0.0f, 0.0f);
    for ( int l = 0; l < vorder; l++ )
      sp = sp + Nv[0][l]*temp[l];

    return sp;
  }

  /// Returns the point on the surface at indices (i,j).
  SbVec4f nurbs::SurfacePoint(const int i, const int j) {
    float u, v;
    mapijtouv( i, j, u, v );

    return SurfacePoint( u, v );
  }

  void nurbs::Test() {
    // testing the constructor
    assert(numuknots == 10);
    assert(numvknots == 12);
    assert(numuctrlpts == 6);
    assert(numvctrlpts == 9);
    assert(uorder == 4);
    assert(vorder == 3);
    assert(udegree == 3);
    assert(vdegree == 2);
    assert(ustride == 4);
    assert(vstride == 4*6);
    assert(dim == 4);

    // testing the routine mapijtouv
    float u, v;
    mapijtouv(0, 0, u, v);
    assert(fabs(u - uknotvec[0]) < 1.e-12);
    assert(fabs(v - vknotvec[0]) < 1.e-12);
    mapijtouv(numuctrlpts-1, numvctrlpts-1, u, v);
    assert(fabs(u - uknotvec[numuknots-1]) < 1.e-12);
    assert(fabs(v - vknotvec[numvknots-1]) < 1.e-12);

    // testing the findspan routine
    int i = 0;
    // rechter rand
    i = FindSpan(uknotvec[0], udegree, numuctrlpts, uknotvec);
    assert( i == udegree );
    // rechts ausserhalb
    i = FindSpan(-1.0f, udegree, numuctrlpts, uknotvec);
    assert( i == udegree );
    // linker rand
    i = FindSpan(uknotvec[numuknots-1], udegree, numuctrlpts, uknotvec);
    assert( i == numuctrlpts-1 );
    // links ausserhalb
    i = FindSpan(uknotvec[numuknots-1]+1.0f, udegree, numuctrlpts, uknotvec);
    assert( i == numuctrlpts-1 );
    // erster Abschnitt
    i = FindSpan((uknotvec[udegree+1]+uknotvec[udegree])/2.0f, udegree, numuctrlpts, uknotvec);
    assert( i == udegree );
    //// zweiter Abschnitt
    //i = FindSpan((uknotvec[udegree+2]+uknotvec[udegree+1])/2.0f, udegree, numuctrlpts, uknotvec);
    //assert( i == udegree+2 );

    float* ptrnormals = new float[numvctrlpts*numuctrlpts*3];
    float* ptr = new float[numvctrlpts*numuctrlpts*dim];
    char buffer[1024];
    static int fno = 0;
    sprintf(buffer, "normaltest%d.iv", fno++);
    FILE* fp = NULL;
    fp = fopen(buffer, "w");
    if ( fp ) fprintf(fp,"#Inventor V2.1 ascii\n"
                      "Separator {\n"
                      "Coordinate3 {\n"
                      "point [\n");
    for ( int j = 0; j < numvctrlpts; j++) {
      for ( int i = 0; i < numuctrlpts; i++) {
        SbVec3f normal = this->normal(i, j);
        int idx2 = j*3*vstride/dim + i*3*ustride/dim;
        ptrnormals[idx2] = normal[0];
        ptrnormals[idx2+1] = normal[1];
        ptrnormals[idx2+2] = normal[2];

        SbVec4f srfpt = this->SurfacePoint(i, j);
        //srfpt.Homogenize();

        int idx = j*vstride + i*ustride;
        ptr[idx] = srfpt[0]/srfpt[3];
        ptr[idx+1] = srfpt[1]/srfpt[3];
        ptr[idx+2] = srfpt[2]/srfpt[3];

        sprintf(buffer, "%g %g %g,", srfpt[0]/srfpt[3], srfpt[1]/srfpt[3], srfpt[2]/srfpt[3]);
        if ( fp ) fputs(buffer,fp);
      }
      if ( fp ) fprintf(fp, "\n");
    }
    if ( fp ) fprintf(fp, "\n");
    for (int j = 0; j < numvctrlpts; j++) {
      for (int i = 0; i < numuctrlpts; i++) {
        int idx = j*3*vstride/dim + i*3*ustride/dim;
        int idxpt = j*vstride + i*ustride;
        // correct the normals if zero normals are present!
        sprintf(buffer, "%g %g %g, ", ptr[idxpt+0]+ptrnormals[idx+0], ptr[idxpt+1]+ptrnormals[idx+1], ptr[idxpt+2]+ptrnormals[idx+2]);
        if ( fp ) fputs(buffer, fp);
      }
      if ( fp ) fprintf(fp, "\n");
    }
    if ( fp ) {
      fprintf(fp,	"]\n"
              "}\n"
              "MarkerSet { numPoints %d markerIndex 21 }\n"
              "IndexedLineSet { coordIndex [\n", numuctrlpts*numvctrlpts);
      for (int j = 0; j < numvctrlpts; j++) {
        for (int i = 0; i < numuctrlpts; i++) {
			fprintf(fp, "%d, %d, -1, ", j*numuctrlpts+i, j*numuctrlpts+i + numuctrlpts*numvctrlpts);
        }
        fprintf(fp, "\n");
      }
      fprintf(fp,	"]\n"
              "}\n"
              "}\n"
              "Coordinate4 {\n"
              "point [\n");
      for ( int j = 0; j < numvctrlpts; j++ ) {
        for ( int i = 0; i < numuctrlpts; i++ ) {
          int idx = j*vstride + i*ustride;
          fprintf(fp, "%g %g %g %g,\n", ctlPoints[idx+0], ctlPoints[idx+1], ctlPoints[idx+2], ctlPoints[idx+3]);
        }
        fprintf(fp, "\n");
      }
      fprintf(fp,	"]\n"
              "}\n"
              "NurbsSurface {\n"
              "numUControlPoints %d\n"
              "numVControlPoints %d\n"
              "uKnotVector [", numuctrlpts, numvctrlpts);
      for ( int i = 0; i < numuknots; i++ )
        fprintf(fp, "%g, ", uknotvec[i]);
      fprintf(fp,	"]\n"
              "vKnotVector [");
      for ( int i = 0; i < numvknots; i++ )
        fprintf(fp, "%g, ", vknotvec[i]);
      fprintf(fp,	"]\n"
              "}\n");
      fclose(fp);
      fp = NULL;
    }
  }

}

void
sogl_render_nurbs_surface(SoAction * action, SoShape * shape,
                          void * nurbsrenderer,
                          const int numuctrlpts, const int numvctrlpts,
                          const float * uknotvec, const float * vknotvec,
                          const int numuknot, const int numvknot,
                          const int numsctrlpts, const int numtctrlpts,
                          const float * sknotvec, const float * tknotvec,
                          const int numsknot, const int numtknot,
                          const SbBool glrender,
                          const int numcoordindex, const int32_t * coordindex,
                          const int numtexcoordindex, const int32_t * texcoordindex)
{
  // Should never get this far if the NURBS functionality is missing.
  assert(GLUWrapper()->available && "NURBS functionality is missing");

  // We use GLU_NURBS_TESSELLATOR further down in the function if
  // glrender==FALSE (i.e. on callback actions were we want to get the
  // polygons), and this is not supported before GLU v1.3.
  assert((glrender ||
          (!glrender && GLUWrapper()->versionMatchesAtLeast(1, 3, 0))) &&
         "NURBS tessellator requires GLU 1.3.");

  // We check for glGetError() at the end of this function, so we
  // should "clean out" at the start.
  if (glrender) {
    cc_string str;
    cc_string_construct(&str);
    const unsigned int errs = coin_catch_gl_errors(&str);
    if (errs > 0) {
      SoDebugError::post("sogl_render_nurbs_surface",
                         "pre GLU-calls, glGetError()s => '%s'",
                         cc_string_get_text(&str));
    }
    cc_string_clean(&str);
  }


  SoState * state = action->getState();

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // Should not set mode if GLU version is < 1.3, as NURBS_RENDERER
    // was the only game in town back then in the old days.
    GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_NURBS_MODE,
                                   (GLfloat) (glrender ? GLU_NURBS_RENDERER : GLU_NURBS_TESSELLATOR));
  }
  // Need to load sampling matrices if glrender==FALSE.
  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_AUTO_LOAD_MATRIX, (GLfloat) glrender);

  if (!glrender) { // supply the sampling matrices
    SbMatrix glmodelmatrix = SoViewingMatrixElement::get(state);
    glmodelmatrix.multLeft(SoModelMatrixElement::get(state));
    SbVec2s size, origin;
    // not all actions enables SoViewportRegion
    // (e.g. SoGetPrimitiveCount).  Just set viewport to a default
    // viewport if the element is not enabled.
    if (state->isElementEnabled(SoViewportRegionElement::getClassStackIndex())) {
      origin = SoViewportRegionElement::get(state).getViewportOriginPixels();
      size = SoViewportRegionElement::get(state).getViewportSizePixels();
    }
    else {
      origin.setValue(0, 0);
      size.setValue(640, 480);
    }
    GLint viewport[4];
    viewport[0] = origin[0];
    viewport[1] = origin[1];
    viewport[2] = size[0];
    viewport[3] = size[1];
    GLUWrapper()->gluLoadSamplingMatrices(nurbsrenderer,
                                          (float*)glmodelmatrix,
                                          SoProjectionMatrixElement::get(state)[0],
                                          viewport);
  }

  int dim = coords->is3D() ? 3 : 4;

  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  if (!coords->getNum()) return;

  GLfloat * ptr = coords->is3D() ?
    (GLfloat *)coordelem->getArrayPtr3() :
    (GLfloat *)coordelem->getArrayPtr4();

  // just copy indexed control points into a linear array
  if (numcoordindex && coordindex) {
    SbList <float> * tmpcoordlist = sogl_get_tmpcoordlist();
    tmpcoordlist->truncate(0);
    for (int i = 0; i < numcoordindex; i++) {
      for (int j = 0; j < dim; j++) {
        tmpcoordlist->append(ptr[coordindex[i]*dim+j]);
      }
    }
    ptr = (float*) tmpcoordlist->getArrayPtr();
  }


  int uIsLinear = numuctrlpts == 2 && numuknot == 2*(numuknot - numuctrlpts);
  int vIsLinear = numvctrlpts == 2 && numvknot == 2*(numvknot - numvctrlpts);
  int uIsClosed = 0;
  int vIsClosed = 0;

  int ustride = dim;
  int vstride = dim * numuctrlpts;

  for (int k = 0; k < dim; k++) {
    // compare first and last values in u direction
    uIsClosed += ptr[k] == ptr[(numuctrlpts-1)*dim+k];
    // compare first and last values in v direction
    for (int j = 0; j < numuctrlpts; j++) {
      vIsClosed += ptr[j*dim + k] == ptr[((numvctrlpts-1)*numuctrlpts + j)*dim + k];
    }
  }
  uIsClosed = uIsClosed == ustride;
  vIsClosed = vIsClosed == vstride;

  float uSpan = uknotvec[numuknot-1] - uknotvec[0];
  float vSpan = vknotvec[numvknot-1] - vknotvec[0];

  sogl_set_nurbs_complexity(action, shape, nurbsrenderer, uIsLinear, vIsLinear, numuctrlpts, numvctrlpts, uIsClosed, vIsClosed, uSpan, vSpan);

  GLUWrapper()->gluBeginSurface(nurbsrenderer);
  GLUWrapper()->gluNurbsSurface(nurbsrenderer,
                                numuknot, (GLfloat*) uknotvec,
                                numvknot, (GLfloat*) vknotvec,
                                ustride, vstride, ptr,
                                numuknot - numuctrlpts, numvknot - numvctrlpts,
                                (dim == 3) ? GL_MAP2_VERTEX_3 : GL_MAP2_VERTEX_4);



  static int calculatenurbsnormals = -1;
  if (calculatenurbsnormals == -1) {
    const char * env = coin_getenv("COIN_CALCULATE_NURBS_NORMALS");
    	calculatenurbsnormals = env ? atoi(env) : 1;
  }
  // generate normal map
  if (calculatenurbsnormals) {
    GLfloat * ptrnormals = NULL;

    SbList <float> * tmpnormallist = sogl_get_tmpnormallist();
    tmpnormallist->truncate(0);

    nurbs nrb(numuknot-numuctrlpts, numuknot, uknotvec,
              numvknot-numvctrlpts, numvknot, vknotvec,
              ustride, vstride, ptr, dim);

    for (int j = 0; j < numvctrlpts; j++) {
      for (int i = 0; i < numuctrlpts; i++) {
        SbVec3f normal = nrb.normal(i, j);
        tmpnormallist->append(normal[0]);
        tmpnormallist->append(normal[1]);
        tmpnormallist->append(normal[2]);
      }
    }
    ptrnormals = (float*) tmpnormallist->getArrayPtr();

    // correct the normals if zero normals are present!
    for (int j = 0; j < numvctrlpts; j++) {
      for (int i = 0; i < numuctrlpts; i++) {
        int idx = j*3*vstride/dim + i*3*ustride/dim;
        // because we explicitly set the normal to 0.0f we can compare against 0.0f!
        if ( ptrnormals[idx] == 0.0f && ptrnormals[idx+1] == 0.0f && ptrnormals[idx+2] == 0.0f ) {
          int idx2 = 0;
          if ( i+1 < numuctrlpts )
            idx2 = idx + 3*ustride/dim;
          else if ( i-1 >= 0)
            idx2 = idx - 3*ustride/dim;
          // use the neighbor v, if the neighbor u also has no normal
          if ( ptrnormals[idx2] == 0.0f && ptrnormals[idx2+1] == 0.0f && ptrnormals[idx2+2] == 0.0f ) {
            if ( j+1 < numvctrlpts )
              idx2 = idx + 3*vstride/dim;
            else if ( j-1 >= 0)
              idx2 = idx - 3*vstride/dim;
          }
          ptrnormals[idx] = ptrnormals[idx2];
          ptrnormals[idx+1] = ptrnormals[idx2+1];
          ptrnormals[idx+2] = ptrnormals[idx2+2];
        }
      }
      //if ( numuctrlpts == 6 && numvctrlpts == 9 )
      //  nrb.Test();
    }

    GLUWrapper()->gluNurbsSurface(nurbsrenderer,
                                  numuknot, (GLfloat*) uknotvec,
                                  numvknot, (GLfloat*) vknotvec,
                                  3*ustride/dim, 3 * vstride/dim, ptrnormals,
                                  numuknot - numuctrlpts, numvknot - numvctrlpts,
                                  GL_MAP2_NORMAL);
  }

  SbBool okcheckelem =
    state->isElementEnabled(SoMultiTextureEnabledElement::getClassStackIndex());

  if (!okcheckelem || (SoMultiTextureEnabledElement::get(state, 0))) {
    const SoMultiTextureCoordinateElement * tc =
      SoMultiTextureCoordinateElement::getInstance(state);
    if (numsctrlpts && numtctrlpts && numsknot && numtknot &&
        (tc->getType() == SoMultiTextureCoordinateElement::EXPLICIT) &&
        tc->getNum()) {
      int texdim = tc->is2D() ? 2 : 4;
      GLfloat * texptr = tc->is2D() ?
        (GLfloat*) tc->getArrayPtr2() :
        (GLfloat*) tc->getArrayPtr4();

      // copy indexed texcoords into temporary array
      if (numtexcoordindex && texcoordindex) {
        SbList <float> * tmptexcoordlist = sogl_get_tmptexcoordlist();
        tmptexcoordlist->truncate(0);
        for (int i = 0; i < numtexcoordindex; i++) {
          for (int j = 0; j < texdim; j++) {
            tmptexcoordlist->append(texptr[texcoordindex[i]*texdim+j]);
          }
        }
        texptr = (float*) tmptexcoordlist->getArrayPtr();
      }

      GLUWrapper()->gluNurbsSurface(nurbsrenderer,
                                    numsknot, (GLfloat*) sknotvec,
                                    numtknot, (GLfloat*) tknotvec,
                                    texdim, texdim * numsctrlpts,
                                    texptr, numsknot - numsctrlpts, numtknot - numtctrlpts,
                                    (texdim == 2) ? GL_MAP2_TEXTURE_COORD_2 : GL_MAP2_TEXTURE_COORD_4);

    }
    else if ((tc->getType() == SoMultiTextureCoordinateElement::DEFAULT) ||
             (tc->getType() == SoMultiTextureCoordinateElement::EXPLICIT)) {
      //FIXME: 3D texture coordinate generation (kintel 20020202)
      static float defaulttex[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
      };
      int i;
      GLfloat defaultknots[4] = {uknotvec[0], uknotvec[0], uknotvec[0], uknotvec[0]};
      for (i = 1; i < numuknot; i++) {
        float val = uknotvec[i];
        if (val < defaultknots[0]) {
          defaultknots[0] = val;
          defaultknots[1] = val;
        }
        if (val > defaultknots[2]) {
          defaultknots[2] = val;
          defaultknots[3] = val;
        }
      }
      GLfloat defaultknott[4] = {vknotvec[0], vknotvec[0], vknotvec[0], vknotvec[0]};
      for (i = 1; i < numvknot; i++) {
        float val = vknotvec[i];
        if (val < defaultknott[0]) {
          defaultknott[0] = val;
          defaultknott[1] = val;
        }
        if (val > defaultknott[2]) {
          defaultknott[2] = val;
          defaultknott[3] = val;
        }
      }
      GLUWrapper()->gluNurbsSurface(nurbsrenderer, 4, defaultknots, 4, defaultknott,
                                    2, 2*2, defaulttex, 4-2, 4-2,
                                    GL_MAP2_TEXTURE_COORD_2);
    }
  }
  const SoNodeList & profilelist = SoProfileElement::get(state);
  int i, n = profilelist.getLength();
  SbBool istrimming = FALSE;

  if (n) {
    for (i = 0; i < n; i++) {
      float * points;
      int32_t numpoints;
      int floatspervec;
      int32_t numknots;
      float * knotvector;

      SoProfile * profile = (SoProfile*) profilelist[i];

      if (istrimming && (profile->linkage.getValue() != SoProfileElement::ADD_TO_CURRENT)) {
        istrimming = FALSE;
        GLUWrapper()->gluEndTrim(nurbsrenderer);
      }
      if (!istrimming) {
        GLUWrapper()->gluBeginTrim(nurbsrenderer);
        istrimming = TRUE;
      }
      profile->getTrimCurve(state, numpoints,
                            points, floatspervec,
                            numknots, knotvector);

      if (numknots) {
        GLUWrapper()->gluNurbsCurve(nurbsrenderer, numknots, knotvector, floatspervec,
                                    points, numknots-numpoints, floatspervec == 2 ?
                                    (GLenum) GLU_MAP1_TRIM_2 : (GLenum) GLU_MAP1_TRIM_3);

      }

      else {
        GLUWrapper()->gluPwlCurve(nurbsrenderer, numpoints, points, floatspervec,
                                  floatspervec == 2 ?
                                  (GLenum) GLU_MAP1_TRIM_2 : (GLenum) GLU_MAP1_TRIM_3 );
      }
    }
    if (istrimming) GLUWrapper()->gluEndTrim(nurbsrenderer);
  }
  GLUWrapper()->gluEndSurface(nurbsrenderer);

  // clear GL error(s) if parametric error value is out of range.
  // FIXME: man, this is ugly! 20020530 mortene.
  if (glrender) {
    cc_string str;
    cc_string_construct(&str);
    const unsigned int errs = coin_catch_gl_errors(&str);
    if (errs > 0) {
      SoDebugError::post("sogl_render_nurbs_surface",
                         "post GLU-calls, glGetError()s => '%s'",
                         cc_string_get_text(&str));

      // this is even uglier. Don't cache if there's an error. I
      // haven't got time to fix this properly right now.
      // pederb, 2003-07-10
      SoCacheElement::invalidate(state);
    }
    cc_string_clean(&str);
  }
}

void
sogl_render_nurbs_curve(SoAction * action, SoShape * shape,
                        void * nurbsrenderer,
                        const int numctrlpts,
                        const float * knotvec,
                        const int numknots,
                        const SbBool glrender,
                        const SbBool drawaspoints,
                        const int numcoordindex, const int32_t * coordindex)
{
  // Should never get this far if the NURBS functionality is missing.
  assert(GLUWrapper()->available && "NURBS functionality is missing");

  // We use GLU_NURBS_TESSELLATOR further down in the function if
  // glrender==FALSE (i.e. on callback actions were we want to get the
  // polygons), and this is not supported before GLU v1.3.
  assert((glrender ||
          (!glrender && GLUWrapper()->versionMatchesAtLeast(1, 3, 0))) &&
         "NURBS tessellator requires GLU 1.3.");

  // We check for glGetError() at the end of this function, so we
  // should "clean out" at the start.
  if (glrender) {
    cc_string str;
    cc_string_construct(&str);
    const unsigned int errs = coin_catch_gl_errors(&str);
    if (errs > 0) {
      SoDebugError::post("sogl_render_nurbs_curve",
                         "pre GLU-calls, glGetError()s => '%s'",
                         cc_string_get_text(&str));
    }
    cc_string_clean(&str);
  }


  SoState * state = action->getState();

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);

  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_DISPLAY_MODE, (GLfloat) (drawaspoints ? GLU_POINT : GLU_LINE));
  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // Should not set mode if GLU version is < 1.3, as NURBS_RENDERER
    // was the only game in town back then in the old days.
    GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_NURBS_MODE,
                                   (GLfloat) (glrender ? GLU_NURBS_RENDERER : GLU_NURBS_TESSELLATOR));
  }
  // Need to load sampling matrices if glrender==FALSE.
  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_AUTO_LOAD_MATRIX, (GLfloat) glrender);

  if (!glrender) { // supply the sampling matrices
    SbMatrix glmodelmatrix = SoViewingMatrixElement::get(state);
    glmodelmatrix.multLeft(SoModelMatrixElement::get(state));
    GLint viewport[4];
    // this element is not enabled for SoGetPrimitiveCountAction
    if (state->isElementEnabled(SoViewportRegionElement::getClassStackIndex())) {
      SbVec2s origin = SoViewportRegionElement::get(state).getViewportOriginPixels();
      SbVec2s size = SoViewportRegionElement::get(state).getViewportSizePixels();

      viewport[0] = origin[0];
      viewport[1] = origin[1];
      viewport[2] = size[0];
      viewport[3] = size[1];
    }
    else {
      viewport[0] = 0;
      viewport[1] = 0;
      viewport[2] = 640;
      viewport[3] = 480;
    }
    GLUWrapper()->gluLoadSamplingMatrices(nurbsrenderer,
                                          (float*)glmodelmatrix,
                                          SoProjectionMatrixElement::get(state)[0],
                                          viewport);
  }

  int dim = coords->is3D() ? 3 : 4;

  GLfloat * ptr = coords->is3D() ?
    (GLfloat *)coords->getArrayPtr3() :
    (GLfloat *)coords->getArrayPtr4();

  // just copy indexed control points into a linear array
  if (numcoordindex && coordindex) {
    SbList <float> * tmpcoordlist = sogl_get_tmpcoordlist();
    tmpcoordlist->truncate(0);
    for (int i = 0; i < numcoordindex; i++) {
      for (int j = 0; j < dim; j++) {
        tmpcoordlist->append(ptr[coordindex[i]*dim+j]);
      }
    }
    ptr = (float*) tmpcoordlist->getArrayPtr();
  }

  int uIsLinear = numctrlpts == 2 && numknots == 2*(numknots - numctrlpts);
  int uIsClosed = 0;

  // compare first and last values in u direction
  for (int i = 0; i < dim; i++)
    uIsClosed += ptr[i] == ptr[(numctrlpts-1)*dim+i];
  uIsClosed = uIsClosed == dim;

  float uSpan = knotvec[numknots-1] - knotvec[0];

  sogl_set_nurbs_complexity(action, shape, nurbsrenderer, uIsLinear, 0, numctrlpts, 0, uIsClosed, 0, uSpan, 0);

  GLUWrapper()->gluBeginCurve(nurbsrenderer);
  GLUWrapper()->gluNurbsCurve(nurbsrenderer,
                              numknots,
                              (float*)knotvec,
                              dim,
                              ptr,
                              numknots - numctrlpts,
                              (GLenum)(dim == 3 ? GL_MAP1_VERTEX_3 : GL_MAP1_VERTEX_4));

  GLUWrapper()->gluEndCurve(nurbsrenderer);

  // clear GL error(s) if parametric error value is out of range.
  // FIXME: man, this is ugly! 20020530 mortene.
  if (glrender) {
    cc_string str;
    cc_string_construct(&str);
    const unsigned int errs = coin_catch_gl_errors(&str);
    if (errs > 0) {
      SoDebugError::post("sogl_render_nurbs_curve",
                         "post GLU-calls, glGetError()s => '%s'",
                         cc_string_get_text(&str));

      // this is even uglier. Don't cache if there's an error. I
      // haven't got time to fix this properly right now.
      // pederb, 2003-07-10
      SoCacheElement::invalidate(state);
    }
    cc_string_clean(&str);
  }
}
