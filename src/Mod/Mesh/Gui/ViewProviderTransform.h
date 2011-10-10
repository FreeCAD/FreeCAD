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

#ifndef MESHGUI_VIEWPROVIDERMESHTRANSFORM_H
#define MESHGUI_VIEWPROVIDERMESHTRANSFORM_H

class SoSeparator;
class SbVec3f;
class SoSwitch;
class SoCoordinate3;
class SoNormal;
class SoIndexedFaceSet;
class SoFaceSet;
class SoPath;
class SoLocateHighlight;
class SoTransformerManip;

namespace Gui {
  class View3DInventorViewer;
}


#include "ViewProvider.h"

namespace MeshGui {

/** Like Mesh viewprovider but with manipulator
 */
class ViewProviderMeshTransform : public ViewProviderMesh
{
  PROPERTY_HEADER(MeshGui::ViewProviderMeshTransform);

public:
  ViewProviderMeshTransform();
  virtual ~ViewProviderMeshTransform();


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
  /// Update the Mesh representation
  virtual void updateData(const App::Property*);

protected:

  SoTransformerManip *pcTransformerDragger;
};

} // namespace MeshGui


#endif // MESHGUI_VIEWPROVIDERMESHTRANSFORM_H

