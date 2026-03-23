#ifndef COIN_SOSHAPE_SONURBSP_H
#define COIN_SOSHAPE_SONURBSP_H
#include "glue/glp.h"

#include <Inventor/SoPrimitiveVertex.h>

class SoAction;

template<class Master>
class SoNurbsP
{
 public:
  //
  // used only for GLU callbacks
  //
  class coin_nurbs_cbdata {
  public:
    coin_nurbs_cbdata(SoAction * action,
                      Master * master,
                      bool is4d) :
      action(action),
      thisp(master),
      is4D(is4d) {}
    SoAction * action;
    SoPrimitiveVertex vertex;
    Master * thisp;
    bool is4D;
  };

  static void APIENTRY tessBegin(int , void * data);
  static void APIENTRY tessTexCoord(float * texcoord, void * data);
  static void APIENTRY tessNormal(float * normal, void * data);
  static void APIENTRY tessVertex(float * vertex, void * data);
  static void APIENTRY tessEnd(void * data);

};

template<class Master>
void APIENTRY
SoNurbsP<Master>::tessTexCoord(float * texcoord, void * data)
{
  coin_nurbs_cbdata * cbdata = static_cast<coin_nurbs_cbdata *>(data);
  cbdata->vertex.setTextureCoords(SbVec4f(texcoord[0], texcoord[1], texcoord[2], texcoord[3]));
}

template<class Master>
void APIENTRY
SoNurbsP<Master>::tessNormal(float * normal, void * data)
{
  coin_nurbs_cbdata * cbdata = static_cast<coin_nurbs_cbdata *>(data);
  cbdata->vertex.setNormal(SbVec3f(normal[0], normal[1], normal[2]));
}

template<class Master>
void APIENTRY
SoNurbsP<Master>::tessVertex(float * vertex, void * data)
{
  coin_nurbs_cbdata * cbdata = static_cast<coin_nurbs_cbdata *>(data);
  // We've had a different version of this code, where we accounted for
  // 4D homogeneous coordinates. However, the GLU documentation states:
  //
  // "All the generated vertices have dimension 3, that is,
  // homogeneous coordinates have been transformed into affine
  // coordinates."
  
  // Any GLU implementation that sends 4D coordinates should be
  // considered buggy.
  
  cbdata->vertex.setPoint(SbVec3f(vertex[0], vertex[1], vertex[2]));
  cbdata->thisp->shapeVertex(&cbdata->vertex);
}

template<class Master>
void APIENTRY
SoNurbsP<Master>::tessEnd(void * data)
{
  coin_nurbs_cbdata * cbdata = static_cast<coin_nurbs_cbdata *>(data);
  cbdata->thisp->endShape();
}

//This function has specializations in implementation files, so be careful about reuse
template<class Master>
void APIENTRY
SoNurbsP<Master>::tessBegin(int type, void * data)
{
  coin_nurbs_cbdata * cbdata = static_cast<coin_nurbs_cbdata *>(data);
  typename Master::TriangleShape shapetype;
  switch (type) {
  case GL_LINES:
    shapetype = SoShape::LINES;
    break;
  case GL_LINE_STRIP:
    shapetype = SoShape::LINE_STRIP;
    break;
  case GL_LINE_LOOP:
    shapetype = SoShape::LINE_STRIP; // will not be closed...
    // FIXME: implement this functionality. 20010909 mortene.
    // Update 20060130 kyrah: According to gluNurbsCallback(3), the
    // only valid arguments we can expect here are GL_LINES or
    // GL_LINE_STRIP, so this should really be an assert. (See also
    // FIXME from 20010909 below - yes, it should be an assert.)

    // FIXME: un-commenting the following line exposes a _weird_ HP-UX
    // aCC bug -- should investigate closer.  (Detected with aCC
    // version B3910B A.03.25).  If possible, try to re-write the
    // COIN_STUB() function with a work-around for the aCC bug.
    // 20010917 mortene.

    //COIN_STUB();

#if COIN_DEBUG
    SoDebugError::postWarning("SoNurbsCurveP::tessBegin",
                              "LINE_LOOP is not supported yet");
#endif // COIN_DEBUG
    break;
  case GL_POINTS:
    shapetype = SoShape::POINTS;
    break;
  case GL_TRIANGLES:
    shapetype = SoShape::TRIANGLES;
    break;
  case GL_TRIANGLE_STRIP:
    shapetype = SoShape::TRIANGLE_STRIP;
    break;
  case GL_TRIANGLE_FAN:
    shapetype = SoShape::TRIANGLE_FAN;
    break;
  case GL_QUADS:
    shapetype = SoShape::QUADS;
    break;
  case GL_QUAD_STRIP:
    shapetype = SoShape::QUAD_STRIP;
    break;
  case GL_POLYGON:
    shapetype = SoShape::POLYGON;
    break;
  default:
    shapetype = SoShape::POINTS; // fall back to points
    // FIXME: should this be an assert, or does it represent
    // something which is out of our control, like a possible future
    // feature of the GLU tessellator?  20010909 mortene.
#if COIN_DEBUG
    SoDebugError::postWarning("SoNurbsCurveP::tessBegin",
                              "unsupported GL enum: 0x%x", type);
#endif // debug
    break;
  }
  cbdata->thisp->beginShape(cbdata->action, shapetype, NULL);
}


#endif //COIN_SOSHAPE_SONURBSP_H
