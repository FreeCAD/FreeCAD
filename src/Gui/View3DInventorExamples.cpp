/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/SbClip.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoComplexity.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoTexture3.h>
# include <Inventor/nodes/SoTextureCoordinate3.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/draggers/SoTransformerDragger.h>

# include <float.h>
# include <cstring>
#endif

#include "../Base/Console.h"


#include "View3DInventorExamples.h"

#include <Inventor/SbPlane.h>
#include <Inventor/SoDB.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/manips/SoPointLightManip.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/draggers/SoTransformerDragger.h>
#include <Inventor/SbBasic.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/sensors/SoTimerSensor.h>



unsigned char * generateTexture(int w, int h, int d)
{
  int val;
  float x,y,z;
//  unsigned char pixval;
  unsigned char * bitmap = new unsigned char[w*h*d];

  for (int k = 0;k<d;k++) {
    z = k*360/d;
    for (int j = 0;j<h;j++) {
      y = (j-h/2)/2;
      for (int i = 0;i<w;i++) {
        x = (i-w/2)/2;
        val = int(x*x + y*y*sin(2*x*M_PI/w + z*M_PI/180));
        val = abs(val%512);
        if (val >= 256) val = 511-val;
        bitmap[k*w*h + j*h + i] = val;
      }
    }
  }

  
  return bitmap;
}

void doClipping(SbVec3f trans, SbRotation rot)
{
  SbMatrix mat;
  SbVec3f normal;

  mat.setTransform(trans, rot, SbVec3f(1,1,1));
  mat.multDirMatrix(SbVec3f(0, -1, 0), normal);
  SbPlane plane(normal, trans);

  const float coords[][3] = {
    {-5,-5,-5},
    {5,-5,-5},
    {5,5,-5},
    {-5,5,-5},
    {-5,-5,5},
    {5,-5,5},
    {5,5,5},
    {-5,5,5}
  };
  const int indices[] = {
    0,3,2,1,-1,
    0,1,5,4,-1,
    2,6,5,1,-1,
    3,7,6,2,-1,
    3,0,4,7,-1,
    7,4,5,6,-1
  };

  // Clip box against plane

  SbClip clip;
  SoMFVec3f * globalVerts = 
    (SoMFVec3f *)SoDB::getGlobalField(SbName("globalVerts"));
  SoMFVec3f * globalTVerts = 
    (SoMFVec3f *)SoDB::getGlobalField(SbName("globalTVerts"));
  SoMFInt32 * globalnv = 
    (SoMFInt32 *)SoDB::getGlobalField(SbName("globalnv"));
  globalVerts->startEditing();
  globalVerts->setNum(0);
  globalTVerts->startEditing();
  globalTVerts->setNum(0);
  globalnv->startEditing();
  globalnv->setNum(0);
  int i;
  for (i = 0;i<6*5;i++) {
    if (indices[i] == -1) {
      clip.clip(plane);
      int numVerts = clip.getNumVertices();
      if (numVerts > 0) {
        for (int j = 0;j<numVerts;j++) {
          SbVec3f v;
          clip.getVertex(j, v);
          globalVerts->set1Value(globalVerts->getNum(), v);
          v += SbVec3f(5, 5, 5);
          v /= 10.0;
          globalTVerts->set1Value(globalTVerts->getNum(), v);
        }
        globalnv->set1Value(globalnv->getNum(), numVerts);
      }
      clip.reset();
    }
    else clip.addVertex(coords[indices[i]]);
  }
  globalVerts->finishEditing();
  globalTVerts->finishEditing();
  globalnv->finishEditing();

  // Close hole in clipped box by clipping against all 6 planes
  
  const SbVec3f planecoords[] = {
    SbVec3f(-10,0,-10),
    SbVec3f(10,0,-10),
    SbVec3f(10,0,10),
    SbVec3f(-10,0,10)
  };

  
  clip.reset();
  for (i = 0;i<4;i++) {
    SbVec3f v;
    mat.multVecMatrix(planecoords[i], v);
    clip.addVertex(v);
  }
  for (i = 0;i<6*5;i+=5) {
    SbPlane p(coords[indices[i+2]],
              coords[indices[i+1]],
              coords[indices[i]]);
    clip.clip(p);
  }
  int numVerts = clip.getNumVertices();
  SoMFVec3f * planeVerts = 
    (SoMFVec3f *)SoDB::getGlobalField(SbName("planeVerts"));
  SoMFVec3f * planeTVerts = 
    (SoMFVec3f *)SoDB::getGlobalField(SbName("planeTVerts"));
  planeVerts->startEditing();
  planeVerts->setNum(0);
  planeTVerts->startEditing();
  planeTVerts->setNum(0);
  for (i = 0;i<numVerts;i++) {
    SbVec3f v;
    clip.getVertex(i, v);
    planeVerts->set1Value(planeVerts->getNum(), v);
    v += SbVec3f(5, 5, 5);
    v /= 10.0;
    planeTVerts->set1Value(planeTVerts->getNum(), v);
  }
  planeVerts->finishEditing();
  planeTVerts->finishEditing();
}

void draggerCB(void * /*data*/, SoDragger * dragger)
{
  SoTransformerDragger * drag = (SoTransformerDragger *)dragger;
  doClipping(drag->translation.getValue(), drag->rotation.getValue());
}

void Texture3D(SoSeparator * root)
{

  SoDB::createGlobalField("globalVerts", SoMFVec3f::getClassTypeId());
  SoDB::createGlobalField("globalTVerts", SoMFVec3f::getClassTypeId());
  SoDB::createGlobalField("globalnv", SoMFInt32::getClassTypeId());
  SoDB::createGlobalField("planeVerts", SoMFVec3f::getClassTypeId());
  SoDB::createGlobalField("planeTVerts", SoMFVec3f::getClassTypeId());

  doClipping(SbVec3f(0,0,0), SbRotation());

//  SoSeparator * root = new SoSeparator;
//  root->ref();

  SoComplexity * comp = new SoComplexity;
  comp->textureQuality.setValue((float)0.9);
  root->addChild(comp);

  SoTexture3 * texture = new SoTexture3;
  texture->wrapR.setValue(SoTexture3::CLAMP);
  texture->wrapS.setValue(SoTexture3::CLAMP);
  texture->wrapT.setValue(SoTexture3::CLAMP);
//    SbString filenames[64];
//    for (int i=0;i<64;i++)
//      filenames[i].sprintf("../../../data/pgmvol/slice%02d.raw.pgm",i);
//    texture->filenames.setValues(0,64,filenames);
  unsigned char * img = generateTexture(256,256,256);
  texture->images.setValue(SbVec3s(256,256,256), 1, img);
  root->addChild(texture);

  SoMaterial * mat = new SoMaterial;
  mat->emissiveColor.setValue(1,1,1);
  root->addChild(mat);
    
  SoTransformerDragger * dragger = new SoTransformerDragger;
  dragger->scaleFactor.setValue(5,5,5);
  dragger->addValueChangedCallback(draggerCB, NULL);
  root->addChild(dragger);

  SoCoordinate3 * clippedCoords = new SoCoordinate3;
  clippedCoords->point.connectFrom((SoMFVec3f *)
                                   SoDB::getGlobalField("globalVerts"));
  root->addChild(clippedCoords);
  SoTextureCoordinate3 * clippedTCoords = new SoTextureCoordinate3;
  clippedTCoords->point.connectFrom((SoMFVec3f *)
                                    SoDB::getGlobalField("globalTVerts"));
  root->addChild(clippedTCoords);
  SoFaceSet * clippedFS = new SoFaceSet;
  clippedFS->numVertices.connectFrom((SoMFInt32 *)
                                     SoDB::getGlobalField("globalnv"));
  root->addChild(clippedFS);

  SoCoordinate3 * planeCoords = new SoCoordinate3;
  planeCoords->point.connectFrom((SoMFVec3f *)
                                 SoDB::getGlobalField("planeVerts"));
  root->addChild(planeCoords);
  SoTextureCoordinate3 * planeTCoords = new SoTextureCoordinate3;
  planeTCoords->point.connectFrom((SoMFVec3f *)
                                  SoDB::getGlobalField("planeTVerts"));
  root->addChild(planeTCoords);
  SoFaceSet * planeFS = new SoFaceSet;
  root->addChild(planeFS);
}

// *************************************************************************


static const char scenegraph[] = "#Inventor V2.1 ascii\n"
"Separator {\n"
"   DEF RedLight   PointLight { location -10 -10 10  color 1 0 0 }\n"
"   DEF GreenLight PointLight { location  -5 5 10  color 0 1 0 }\n"
"   DEF BlueLight  PointLight { location  10 10 10  color 0 0 1 }\n"
"   Material { diffuseColor 0.5 0.5 0.5  specularColor 1 1 1 }\n"
"   Array {\n"
"     origin CENTER\n"
"     numElements1 3  separation1 5.5 0 0\n"
"     numElements2 3  separation2 0 5.5 0\n"
"\n"
"     Sphere { radius 3 }\n"
"   }\n"
"}\n";

void LightManip(SoSeparator * root)
{

  SoInput in;
  in.setBuffer((void *)scenegraph, std::strlen(scenegraph));
  SoSeparator * _root = SoDB::readAll( &in );
  if ( _root == NULL ) return; // Shouldn't happen.
  root->addChild(_root);
  root->ref();

  const char * pointlightnames[3] = { "RedLight", "GreenLight", "BlueLight" };
  SoSearchAction sa;

  for (int i = 0; i < 3; i++) {
    sa.setName( pointlightnames[i] );
    sa.setInterest( SoSearchAction::FIRST );
    sa.setSearchingAll( false );
    sa.apply( root );
    SoPath * path = sa.getPath();
    if ( path == NULL) return; // Shouldn't happen.

    SoPointLightManip * manip = new SoPointLightManip;
    manip->replaceNode( path );
  }


} 




/***********************************************************************************************************/


// Global constants
const int texturewidth = 128;
const int textureheight = 128;

// Global variables
double global_cr = 0.33;
double global_ci = 0.43;

// Global pointer
//unsigned char * bitmap = new unsigned char[texturewidth*textureheight];
unsigned char bitmap[texturewidth*textureheight];

// Function to generate a julia set
// Parameters:
//  double cr   - real part of the julia set point
//  double ci   - imaginary part of the julia set point
//  float zoom  - length of the square to display (zoom*zoom), center (0,0)
//  int width   - width of the bitmap
//  int height  - height of the bitmap
//  int mult    - number to multiply each color by.
//  unsigned char * bmp - pointer to the bitmap
//  int n       - number of iterations 
void
julia(double crr, double cii, float zoom, int width, int height, int mult, 
      unsigned char * bmp, int n)
{
  double zr, zr_old, zi;
  int w;

  for (int y=0; y<height/2; y++)
    for (int x=0; x<width; x++) {
      zr = ((double)(x)/(double)width)*zoom-zoom/2;
      zi = ((double)(y)/(double)height)*zoom-zoom/2;
      for (w = 0; (w < n) && (zr*zr+zi*zi)<n; w++) {
        zr_old = zr;
        zr = zr*zr - zi*zi + crr;
        zi = 2*zr_old*zi + cii;
      }
      bmp[y*width+x] = 255-w*mult;
      bmp[((height-y)*width)-(x+1)] = 255-w*mult;
    }
}

// Function that loads a texture from memory, and return a pointer to a
// SoTexture2 node containing this texture..
// Return:
//  SoTexture2 *
SoTexture2 *
texture()
{
  SoTexture2 * texture = new SoTexture2;
  texture->image.setValue(SbVec2s(texturewidth, textureheight), 1, bitmap);
  texture->model = SoTexture2::MODULATE;
  texture->blendColor.setValue(1.0, 0.0, 0.0);
  return texture;
}

// This function is called 20 times each second. 
static void
timersensorcallback(void * data, SoSensor *)
{
  static SbBool direction = false;

  SoTexture2 * texnode = (SoTexture2*) data;

  if (!direction) {
    global_cr -= 0.0005;
    global_ci += 0.0005;
  }
  else {
    global_cr += 0.0005;
    global_ci -= 0.0005;
  }

  if (global_ci<0.30)
    direction = !direction;
  else if (global_ci>0.83)
    direction = !direction;

  SbVec2s size;
  int nc;
  unsigned char * image = texnode->image.startEditing(size, nc);
  // Generate a julia set to use as a texturemap
  julia(global_cr, global_ci, 2.5, size[0], size[1], 4, image, 64);
  texnode->image.finishEditing();
}

void AnimationTexture(SoSeparator * root)
{
  // Scene graph
  if ( root == NULL ) return; // Shouldn't happen.

  // Generate a julia set to use as a texturemap
  julia(global_cr, global_ci, 2.5, texturewidth, textureheight, 4, bitmap, 64);

 
  SoTexture2 * texnode = texture();

  // Enable backface culling
  SoShapeHints * hints = new SoShapeHints;

  hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  hints->shapeType = SoShapeHints::SOLID;

  // Timer sensor
  SoTimerSensor * texturetimer = new SoTimerSensor(timersensorcallback, texnode);
  texturetimer->setInterval(0.05);
  texturetimer->schedule();

  root->ref(); // prevent from being deleted because of the still running timer sensor
//  SoSeparator * root = new SoSeparator;
//  root->ref();

  root->addChild(hints);
  root->addChild(texnode);
  root->addChild(new SoCube);
}
