/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESGUI_VIEWPROVIDERMESHTRANSFORMDEMOLDING_H
#define MESGUI_VIEWPROVIDERMESHTRANSFORMDEMOLDING_H

#include <Inventor/nodes/SoRotation.h>

class SoSeparator;
class SbVec3f;
class SoSwitch;
class SoCoordinate3;
class SoDragger;
class SoNormal;
class SoIndexedFaceSet;
class SoFaceSet;
class SoPath;
class SoLocateHighlight;
class SoTrackballDragger;
class SoTransformerManip;

namespace Gui {
  class View3DInventorViewer;
}


#include "ViewProvider.h"
#include <Base/Vector3D.h>

namespace MeshGui {

/** Like Mesh viewprovider but with manipulator
 */
class ViewProviderMeshTransformDemolding : public ViewProviderMesh
{
  PROPERTY_HEADER(MeshGui::ViewProviderMeshTransformDemolding);

public:
  ViewProviderMeshTransformDemolding();
  virtual ~ViewProviderMeshTransformDemolding();


  /** 
   * Extracts the mesh data from the feature \a pcFeature and creates
   * an Inventor node \a SoNode with these data. 
   */
  virtual void attach(App::DocumentObject *);

  /// set the viewing mode
  virtual void setDisplayMode(const char* ModeName);
  /// get the default display mode
  virtual const char* getDefaultDisplayMode() const;
  /// returns a list of all possible modes
  virtual std::vector<std::string> getDisplayModes(void) const;

protected:
  void calcMaterialIndex(const SbRotation &rot);
  void calcNormalVector(void);

  static void sValueChangedCallback(void *, SoDragger *);
  void valueChangedCallback(void);

  static void sDragEndCallback(void *, SoDragger *);
  void DragEndCallback(void);

  SoTrackballDragger *pcTrackballDragger;
  SoTransform        *pcTransformDrag;
  SoMaterial         *pcColorMat;
  std::vector<SbVec3f> normalVector;
  Base::Vector3f            center;

};

} // namespace MeshGui


#endif // MESGUI_VIEWPROVIDERMESHTRANSFORMDEMOLDING_H

