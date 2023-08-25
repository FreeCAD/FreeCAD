/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/elements/SoComplexityElement.h>
# include <Inventor/elements/SoComplexityTypeElement.h>
# include <Inventor/elements/SoCoordinateElement.h>
# include <Inventor/elements/SoFontNameElement.h>
# include <Inventor/elements/SoFontSizeElement.h>
# include <Inventor/elements/SoModelMatrixElement.h>
# include <Inventor/elements/SoProfileCoordinateElement.h>
# include <Inventor/elements/SoProfileElement.h>
# include <Inventor/elements/SoProjectionMatrixElement.h>
# include <Inventor/elements/SoShapeStyleElement.h>
# include <Inventor/elements/SoSwitchElement.h>
# include <Inventor/elements/SoUnitsElement.h>
# include <Inventor/elements/SoViewingMatrixElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/elements/SoViewVolumeElement.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCallback.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoComplexity.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCoordinate4.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoMatrixTransform.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoProfile.h>
# include <Inventor/nodes/SoProfileCoordinate2.h>
# include <Inventor/nodes/SoProfileCoordinate3.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransformation.h>
#endif

#include "SoFCSelectionAction.h"
#include "SoFCSelection.h"


using namespace Gui;


SO_ACTION_SOURCE(SoFCHighlightAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCHighlightAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCHighlightAction,SoAction);

  SO_ENABLE(SoFCHighlightAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCHighlightAction, SoModelMatrixElement);
  SO_ENABLE(SoFCHighlightAction, SoShapeStyleElement);
  SO_ENABLE(SoFCHighlightAction, SoComplexityElement);
  SO_ENABLE(SoFCHighlightAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCHighlightAction, SoCoordinateElement);
  SO_ENABLE(SoFCHighlightAction, SoFontNameElement);
  SO_ENABLE(SoFCHighlightAction, SoFontSizeElement);
  SO_ENABLE(SoFCHighlightAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCHighlightAction, SoProfileElement);
  SO_ENABLE(SoFCHighlightAction, SoSwitchElement);
  SO_ENABLE(SoFCHighlightAction, SoUnitsElement);
  SO_ENABLE(SoFCHighlightAction, SoViewVolumeElement);
  SO_ENABLE(SoFCHighlightAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCHighlightAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);

  SO_ACTION_ADD_METHOD(SoIndexedLineSet,callDoAction);
  SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);
  SO_ACTION_ADD_METHOD(SoPointSet,callDoAction);
}

void SoFCHighlightAction::finish()
{
  atexit_cleanup();
}


SoFCHighlightAction::SoFCHighlightAction (const SelectionChanges &SelCh)
:SelChange(SelCh)
{
  SO_ACTION_CONSTRUCTOR(SoFCHighlightAction);
}


SoFCHighlightAction::~SoFCHighlightAction() = default;


void SoFCHighlightAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCHighlightAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCSelectionAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCSelectionAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCSelectionAction,SoAction);

  SO_ENABLE(SoFCSelectionAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCSelectionAction, SoModelMatrixElement);
  SO_ENABLE(SoFCSelectionAction, SoShapeStyleElement);
  SO_ENABLE(SoFCSelectionAction, SoComplexityElement);
  SO_ENABLE(SoFCSelectionAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCSelectionAction, SoCoordinateElement);
  SO_ENABLE(SoFCSelectionAction, SoFontNameElement);
  SO_ENABLE(SoFCSelectionAction, SoFontSizeElement);
  SO_ENABLE(SoFCSelectionAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCSelectionAction, SoProfileElement);
  SO_ENABLE(SoFCSelectionAction, SoSwitchElement);
  SO_ENABLE(SoFCSelectionAction, SoUnitsElement);
  SO_ENABLE(SoFCSelectionAction, SoViewVolumeElement);
  SO_ENABLE(SoFCSelectionAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCSelectionAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);

  SO_ACTION_ADD_METHOD(SoIndexedLineSet,callDoAction);
  SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);
  SO_ACTION_ADD_METHOD(SoPointSet,callDoAction);
}

void SoFCSelectionAction::finish()
{
  atexit_cleanup();
}


SoFCSelectionAction::SoFCSelectionAction (const SelectionChanges &SelCh)
:SelChange(SelCh)
{
  SO_ACTION_CONSTRUCTOR(SoFCSelectionAction);
}


SoFCSelectionAction::~SoFCSelectionAction() = default;


void SoFCSelectionAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCSelectionAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCEnableSelectionAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCEnableSelectionAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCEnableSelectionAction,SoAction);

  SO_ENABLE(SoFCEnableSelectionAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCEnableSelectionAction, SoModelMatrixElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoShapeStyleElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoComplexityElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoCoordinateElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoFontNameElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoFontSizeElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoProfileElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoSwitchElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoUnitsElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoViewVolumeElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCEnableSelectionAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCEnableSelectionAction::finish()
{
  atexit_cleanup();
}


SoFCEnableSelectionAction::SoFCEnableSelectionAction (const SbBool& sel)
  : selection(sel)
{
  SO_ACTION_CONSTRUCTOR(SoFCEnableSelectionAction);
}


SoFCEnableSelectionAction::~SoFCEnableSelectionAction() = default;


void SoFCEnableSelectionAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCEnableSelectionAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCEnableHighlightAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCEnableHighlightAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCEnableHighlightAction,SoAction);

  SO_ENABLE(SoFCEnableHighlightAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCEnableHighlightAction, SoModelMatrixElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoShapeStyleElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoComplexityElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoCoordinateElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoFontNameElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoFontSizeElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoProfileElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoSwitchElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoUnitsElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoViewVolumeElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCEnableHighlightAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCEnableHighlightAction::finish()
{
  atexit_cleanup();
}


SoFCEnableHighlightAction::SoFCEnableHighlightAction (const SbBool& sel)
  : highlight(sel)
{
  SO_ACTION_CONSTRUCTOR(SoFCEnableHighlightAction);
}


SoFCEnableHighlightAction::~SoFCEnableHighlightAction() = default;


void SoFCEnableHighlightAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCEnableHighlightAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCSelectionColorAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCSelectionColorAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCSelectionColorAction,SoAction);

  SO_ENABLE(SoFCSelectionColorAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCSelectionColorAction, SoModelMatrixElement);
  SO_ENABLE(SoFCSelectionColorAction, SoShapeStyleElement);
  SO_ENABLE(SoFCSelectionColorAction, SoComplexityElement);
  SO_ENABLE(SoFCSelectionColorAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCSelectionColorAction, SoCoordinateElement);
  SO_ENABLE(SoFCSelectionColorAction, SoFontNameElement);
  SO_ENABLE(SoFCSelectionColorAction, SoFontSizeElement);
  SO_ENABLE(SoFCSelectionColorAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCSelectionColorAction, SoProfileElement);
  SO_ENABLE(SoFCSelectionColorAction, SoSwitchElement);
  SO_ENABLE(SoFCSelectionColorAction, SoUnitsElement);
  SO_ENABLE(SoFCSelectionColorAction, SoViewVolumeElement);
  SO_ENABLE(SoFCSelectionColorAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCSelectionColorAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCSelectionColorAction::finish()
{
  atexit_cleanup();
}


SoFCSelectionColorAction::SoFCSelectionColorAction (const SoSFColor& col)
  : selectionColor(col)
{
  SO_ACTION_CONSTRUCTOR(SoFCSelectionColorAction);
}


SoFCSelectionColorAction::~SoFCSelectionColorAction() = default;


void SoFCSelectionColorAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCSelectionColorAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCHighlightColorAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCHighlightColorAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCHighlightColorAction,SoAction);

  SO_ENABLE(SoFCHighlightColorAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCHighlightColorAction, SoModelMatrixElement);
  SO_ENABLE(SoFCHighlightColorAction, SoShapeStyleElement);
  SO_ENABLE(SoFCHighlightColorAction, SoComplexityElement);
  SO_ENABLE(SoFCHighlightColorAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCHighlightColorAction, SoCoordinateElement);
  SO_ENABLE(SoFCHighlightColorAction, SoFontNameElement);
  SO_ENABLE(SoFCHighlightColorAction, SoFontSizeElement);
  SO_ENABLE(SoFCHighlightColorAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCHighlightColorAction, SoProfileElement);
  SO_ENABLE(SoFCHighlightColorAction, SoSwitchElement);
  SO_ENABLE(SoFCHighlightColorAction, SoUnitsElement);
  SO_ENABLE(SoFCHighlightColorAction, SoViewVolumeElement);
  SO_ENABLE(SoFCHighlightColorAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCHighlightColorAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCHighlightColorAction::finish()
{
  atexit_cleanup();
}


SoFCHighlightColorAction::SoFCHighlightColorAction (const SoSFColor& col)
  : highlightColor(col)
{
  SO_ACTION_CONSTRUCTOR(SoFCHighlightColorAction);
}


SoFCHighlightColorAction::~SoFCHighlightColorAction() = default;


void SoFCHighlightColorAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCHighlightColorAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCDocumentAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCDocumentAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCDocumentAction,SoAction);

  SO_ENABLE(SoFCDocumentAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCDocumentAction, SoModelMatrixElement);
  SO_ENABLE(SoFCDocumentAction, SoShapeStyleElement);
  SO_ENABLE(SoFCDocumentAction, SoComplexityElement);
  SO_ENABLE(SoFCDocumentAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCDocumentAction, SoCoordinateElement);
  SO_ENABLE(SoFCDocumentAction, SoFontNameElement);
  SO_ENABLE(SoFCDocumentAction, SoFontSizeElement);
  SO_ENABLE(SoFCDocumentAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCDocumentAction, SoProfileElement);
  SO_ENABLE(SoFCDocumentAction, SoSwitchElement);
  SO_ENABLE(SoFCDocumentAction, SoUnitsElement);
  SO_ENABLE(SoFCDocumentAction, SoViewVolumeElement);
  SO_ENABLE(SoFCDocumentAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCDocumentAction, SoViewportRegionElement);




  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCDocumentAction::finish()
{
  atexit_cleanup();
}


SoFCDocumentAction::SoFCDocumentAction (const SoSFString& docName)
  : documentName(docName)
{
  SO_ACTION_CONSTRUCTOR(SoFCDocumentAction);
}


SoFCDocumentAction::~SoFCDocumentAction() = default;


void SoFCDocumentAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCDocumentAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}


// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoFCDocumentObjectAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoFCDocumentObjectAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoFCDocumentObjectAction,SoAction);

  SO_ENABLE(SoFCDocumentObjectAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoFCDocumentObjectAction, SoModelMatrixElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoShapeStyleElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoComplexityElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoComplexityTypeElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoCoordinateElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoFontNameElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoFontSizeElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoProfileCoordinateElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoProfileElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoSwitchElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoUnitsElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoViewVolumeElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoViewingMatrixElement);
  SO_ENABLE(SoFCDocumentObjectAction, SoViewportRegionElement);

  SO_ACTION_ADD_METHOD(SoCallback,callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2,callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

void SoFCDocumentObjectAction::finish()
{
  atexit_cleanup();
}

SoFCDocumentObjectAction::SoFCDocumentObjectAction()
{
  SO_ACTION_CONSTRUCTOR(SoFCDocumentObjectAction);
}

SoFCDocumentObjectAction::~SoFCDocumentObjectAction() = default;

void SoFCDocumentObjectAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoFCDocumentObjectAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

void SoFCDocumentObjectAction::setHandled()
{
  this->_handled = true;
}

SbBool SoFCDocumentObjectAction::isHandled() const
{
  return this->_handled;
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoGLSelectAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoGLSelectAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoGLSelectAction,SoAction);

  SO_ENABLE(SoGLSelectAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoGLSelectAction, SoModelMatrixElement);
  SO_ENABLE(SoGLSelectAction, SoProjectionMatrixElement);
  SO_ENABLE(SoGLSelectAction, SoCoordinateElement);
  SO_ENABLE(SoGLSelectAction, SoViewVolumeElement);
  SO_ENABLE(SoGLSelectAction, SoViewingMatrixElement);
  SO_ENABLE(SoGLSelectAction, SoViewportRegionElement);

  SO_ACTION_ADD_METHOD(SoCamera,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);
  SO_ACTION_ADD_METHOD(SoShape,callDoAction);
  SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

SoGLSelectAction::SoGLSelectAction (const SbViewportRegion& region,
                                    const SbViewportRegion& select)
  : vpregion(region), vpselect(select)
{
  SO_ACTION_CONSTRUCTOR(SoGLSelectAction);
}

SoGLSelectAction::~SoGLSelectAction() = default;

const SbViewportRegion& SoGLSelectAction::getViewportRegion () const
{
  return this->vpselect;
}

void SoGLSelectAction::beginTraversal(SoNode *node)
{
  SoViewportRegionElement::set(this->getState(), this->vpregion);
  traverse(node);
}

void SoGLSelectAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

void SoGLSelectAction::setHandled()
{
  this->_handled = true;
}

SbBool SoGLSelectAction::isHandled() const
{
  return this->_handled;
}

// ---------------------------------------------------------------

SO_ACTION_SOURCE(SoVisibleFaceAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoVisibleFaceAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoVisibleFaceAction,SoAction);

  SO_ENABLE(SoVisibleFaceAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoVisibleFaceAction, SoModelMatrixElement);
  SO_ENABLE(SoVisibleFaceAction, SoProjectionMatrixElement);
  SO_ENABLE(SoVisibleFaceAction, SoCoordinateElement);
  SO_ENABLE(SoVisibleFaceAction, SoViewVolumeElement);
  SO_ENABLE(SoVisibleFaceAction, SoViewingMatrixElement);
  SO_ENABLE(SoVisibleFaceAction, SoViewportRegionElement);


  SO_ACTION_ADD_METHOD(SoCamera,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);
  SO_ACTION_ADD_METHOD(SoShape,callDoAction);
  SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

SoVisibleFaceAction::SoVisibleFaceAction ()
{
  SO_ACTION_CONSTRUCTOR(SoVisibleFaceAction);
}

SoVisibleFaceAction::~SoVisibleFaceAction() = default;

void SoVisibleFaceAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoVisibleFaceAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

void SoVisibleFaceAction::setHandled()
{
  this->_handled = true;
}

SbBool SoVisibleFaceAction::isHandled() const
{
  return this->_handled;
}

// ---------------------------------------------------------------


SO_ACTION_SOURCE(SoUpdateVBOAction)

/**
 * The order of the defined SO_ACTION_ADD_METHOD statements is very important. First the base
 * classes and afterwards subclasses of them must be listed, otherwise the registered methods
 * of subclasses will be overridden. For more details see the thread in the Coin3d forum
 * https://www.coin3d.org/pipermail/coin-discuss/2004-May/004346.html.
 * This means that \c SoSwitch must be listed after \c SoGroup and \c SoFCSelection after
 * \c SoSeparator because both classes inherits the others.
 */
void SoUpdateVBOAction::initClass()
{
  SO_ACTION_INIT_CLASS(SoUpdateVBOAction,SoAction);

  SO_ENABLE(SoUpdateVBOAction, SoSwitchElement);

  SO_ACTION_ADD_METHOD(SoNode,nullAction);

  SO_ENABLE(SoUpdateVBOAction, SoModelMatrixElement);
  SO_ENABLE(SoUpdateVBOAction, SoProjectionMatrixElement);
  SO_ENABLE(SoUpdateVBOAction, SoCoordinateElement);
  SO_ENABLE(SoUpdateVBOAction, SoViewVolumeElement);
  SO_ENABLE(SoUpdateVBOAction, SoViewingMatrixElement);
  SO_ENABLE(SoUpdateVBOAction, SoViewportRegionElement);


  SO_ACTION_ADD_METHOD(SoCamera,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,callDoAction);
  SO_ACTION_ADD_METHOD(SoSwitch,callDoAction);
  SO_ACTION_ADD_METHOD(SoShape,callDoAction);
  SO_ACTION_ADD_METHOD(SoIndexedFaceSet,callDoAction);

  SO_ACTION_ADD_METHOD(SoSeparator,callDoAction);
  SO_ACTION_ADD_METHOD(SoFCSelection,callDoAction);
}

SoUpdateVBOAction::SoUpdateVBOAction ()
{
  SO_ACTION_CONSTRUCTOR(SoUpdateVBOAction);
}

SoUpdateVBOAction::~SoUpdateVBOAction() = default;

void SoUpdateVBOAction::finish()
{
  atexit_cleanup();
}

void SoUpdateVBOAction::beginTraversal(SoNode *node)
{
  traverse(node);
}

void SoUpdateVBOAction::callDoAction(SoAction *action,SoNode *node)
{
  node->doAction(action);
}

// ---------------------------------------------------------------

namespace Gui {
class SoBoxSelectionRenderActionP {
public:
    SoBoxSelectionRenderActionP(SoBoxSelectionRenderAction * master)
      : master(master)
    {
    }

    SoBoxSelectionRenderAction * master;
    SoSearchAction * searchaction{nullptr};
    SoSearchAction * selectsearch{nullptr};
    SoSearchAction * camerasearch{nullptr};
    SoGetBoundingBoxAction * bboxaction{nullptr};
    SoBaseColor * basecolor{nullptr};
    SoTempPath * postprocpath{nullptr};
    SoPath * highlightPath{nullptr};
    SoSeparator * localRoot{nullptr};
    SoMatrixTransform * xform{nullptr};
    SoCube * cube{nullptr};
    SoDrawStyle * drawstyle{nullptr};
    SoColorPacker colorpacker;

    void initBoxGraph();
    void updateBbox(const SoPath * path);
};

}

#undef PRIVATE
#define PRIVATE(p) ((p)->pimpl)
#undef PUBLIC
#define PUBLIC(p) ((p)->master)

// used to initialize the internal storage class with variables
void
SoBoxSelectionRenderActionP::initBoxGraph()
{
    this->localRoot = new SoSeparator;
    this->localRoot->ref();
    this->localRoot->renderCaching = SoSeparator::OFF;
    this->localRoot->boundingBoxCaching = SoSeparator::OFF;

    this->xform = new SoMatrixTransform;
    this->cube = new SoCube;

    this->drawstyle = new SoDrawStyle;
    this->drawstyle->style = SoDrawStyleElement::LINES;
    this->basecolor = new SoBaseColor;

    auto lightmodel = new SoLightModel;
    lightmodel->model = SoLightModel::BASE_COLOR;

    auto complexity = new SoComplexity;
    complexity->textureQuality = 0.0f;
    complexity->type = SoComplexityTypeElement::BOUNDING_BOX;

    this->localRoot->addChild(this->drawstyle);
    this->localRoot->addChild(this->basecolor);

    this->localRoot->addChild(lightmodel);
    this->localRoot->addChild(complexity);

    this->localRoot->addChild(this->xform);
    this->localRoot->addChild(this->cube);
}


// used to render shape and non-shape nodes (usually SoGroup or SoSeparator).
void
SoBoxSelectionRenderActionP::updateBbox(const SoPath * path)
{
    if (!this->camerasearch) {
        this->camerasearch = new SoSearchAction;
    }

    // find camera used to render node
    this->camerasearch->setFind(SoSearchAction::TYPE);
    this->camerasearch->setInterest(SoSearchAction::LAST);
    this->camerasearch->setType(SoCamera::getClassTypeId());
    this->camerasearch->apply(const_cast<SoPath*>(path));

    if (!this->camerasearch->getPath()) {
        // if there is no camera there is no point rendering the bbox
        return;
    }
    this->localRoot->insertChild(this->camerasearch->getPath()->getTail(), 0);
    this->camerasearch->reset();

    if (!this->bboxaction) {
        this->bboxaction = new SoGetBoundingBoxAction(SbViewportRegion(100, 100));
    }
    this->bboxaction->setViewportRegion(PUBLIC(this)->getViewportRegion());
    this->bboxaction->apply(const_cast<SoPath*>(path));

    SbXfBox3f & box = this->bboxaction->getXfBoundingBox();

    if (!box.isEmpty()) {
        // set cube size
        float x, y, z;
        box.getSize(x, y, z);
        this->cube->width  = x;
        this->cube->height  = y;
        this->cube->depth = z;

        SbMatrix transform = box.getTransform(); // clazy:exclude=rule-of-two-soft

        // get center (in the local bbox coordinate system)
        SbVec3f center = box.SbBox3f::getCenter();

        // if center != (0,0,0), move the cube
        if (center != SbVec3f(0.0f, 0.0f, 0.0f)) {
            SbMatrix t;
            t.setTranslate(center);
            transform.multLeft(t);
        }
        this->xform->matrix = transform;

        PUBLIC(this)->SoGLRenderAction::apply(this->localRoot);
    }
    // remove camera
    this->localRoot->removeChild(0);
}

SO_ACTION_SOURCE(SoBoxSelectionRenderAction)

// Overridden from parent class.
void
SoBoxSelectionRenderAction::initClass()
{
    SO_ACTION_INIT_CLASS(SoBoxSelectionRenderAction, SoGLRenderAction);
}

SoBoxSelectionRenderAction::SoBoxSelectionRenderAction()
  : inherited(SbViewportRegion())
{
    this->constructorCommon();
}

SoBoxSelectionRenderAction::SoBoxSelectionRenderAction(const SbViewportRegion & viewportregion)
  : inherited(viewportregion)
{
    this->constructorCommon();
}

//
// private. called by both constructors
//
void
SoBoxSelectionRenderAction::constructorCommon()
{
    SO_ACTION_CONSTRUCTOR(SoBoxSelectionRenderAction);

    PRIVATE(this) = new SoBoxSelectionRenderActionP(this);

    // Initialize local variables
    PRIVATE(this)->initBoxGraph();

    this->hlVisible = false;

    PRIVATE(this)->basecolor->rgb.setValue(1.0f, 0.0f, 0.0f);
    PRIVATE(this)->drawstyle->linePattern = 0xffff;
    PRIVATE(this)->drawstyle->lineWidth = 1.0f;
    PRIVATE(this)->searchaction = nullptr;
    PRIVATE(this)->selectsearch = nullptr;
    PRIVATE(this)->camerasearch = nullptr;
    PRIVATE(this)->bboxaction = nullptr;

    // SoBase-derived objects should be dynamically allocated.
    PRIVATE(this)->postprocpath = new SoTempPath(32);
    PRIVATE(this)->postprocpath->ref();
    PRIVATE(this)->highlightPath = nullptr;
}

SoBoxSelectionRenderAction::~SoBoxSelectionRenderAction()
{
    // clear highlighting node
    if (PRIVATE(this)->highlightPath) {
        PRIVATE(this)->highlightPath->unref();
    }
    PRIVATE(this)->postprocpath->unref();
    PRIVATE(this)->localRoot->unref();

    delete PRIVATE(this)->searchaction;
    delete PRIVATE(this)->selectsearch;
    delete PRIVATE(this)->camerasearch;
    delete PRIVATE(this)->bboxaction;
    delete PRIVATE(this);
}

void
SoBoxSelectionRenderAction::apply(SoNode * node)
{
    SoGLRenderAction::apply(node);
    if (this->hlVisible) {
        if (!PRIVATE(this)->searchaction) {
            PRIVATE(this)->searchaction = new SoSearchAction;
        }
        PRIVATE(this)->searchaction->setType(SoFCSelection::getClassTypeId());
        PRIVATE(this)->searchaction->setInterest(SoSearchAction::ALL);
        PRIVATE(this)->searchaction->apply(node);
        const SoPathList & pathlist = PRIVATE(this)->searchaction->getPaths();
        if (pathlist.getLength() > 0) {
            for (int i = 0; i < pathlist.getLength(); i++ ) {
                SoPath * path = pathlist[i];
                assert(path);
                auto selection = static_cast<SoFCSelection *>(path->getTail());
                assert(selection->getTypeId().isDerivedFrom(SoFCSelection::getClassTypeId()));
                if (selection->selected.getValue() && selection->style.getValue() == SoFCSelection::BOX) {
                    PRIVATE(this)->basecolor->rgb.setValue(selection->colorSelection.getValue());
                    if (!PRIVATE(this)->selectsearch) {
                        PRIVATE(this)->selectsearch = new SoSearchAction;
                    }
                    PRIVATE(this)->selectsearch->setType(SoShape::getClassTypeId());
                    PRIVATE(this)->selectsearch->setInterest(SoSearchAction::FIRST);
                    PRIVATE(this)->selectsearch->apply(selection);
                    SoPath* shapepath = PRIVATE(this)->selectsearch->getPath();
                    if (shapepath) {
                        SoPathList list;
                        list.append(shapepath);
                        this->drawBoxes(path, &list);
                    }
                    PRIVATE(this)->selectsearch->reset();
                }
                else if (selection->isHighlighted() &&
                         selection->selected.getValue() == SoFCSelection::NOTSELECTED &&
                         selection->style.getValue() == SoFCSelection::BOX) {
                    PRIVATE(this)->basecolor->rgb.setValue(selection->colorHighlight.getValue());

                    if (!PRIVATE(this)->selectsearch) {
                        PRIVATE(this)->selectsearch = new SoSearchAction;
                    }
                    PRIVATE(this)->selectsearch->setType(SoShape::getClassTypeId());
                    PRIVATE(this)->selectsearch->setInterest(SoSearchAction::FIRST);
                    PRIVATE(this)->selectsearch->apply(selection);
                    SoPath* shapepath = PRIVATE(this)->selectsearch->getPath();
                    if (shapepath) {
                        SoPathList list;
                        list.append(shapepath);
                        // clear old highlighting node if still active
                        if (PRIVATE(this)->highlightPath) {
                            PRIVATE(this)->highlightPath->unref();
                        }
                        PRIVATE(this)->highlightPath = path;
                        PRIVATE(this)->highlightPath->ref();
                        this->drawBoxes(path, &list);
                    }
                    PRIVATE(this)->selectsearch->reset();
                }
            }
        }
        PRIVATE(this)->searchaction->reset();
    }
}

void
SoBoxSelectionRenderAction::apply(SoPath * path)
{
    SoGLRenderAction::apply(path);
    SoNode* node = path->getTail();
    if (node && node->getTypeId() == SoFCSelection::getClassTypeId()) {
        auto selection = static_cast<SoFCSelection *>(node);

        // This happens when dehighlighting the current shape
        if (PRIVATE(this)->highlightPath == path) {
            PRIVATE(this)->highlightPath->unref();
            PRIVATE(this)->highlightPath = nullptr;
            // FIXME: Doing a redraw to remove the shown bounding box causes
            // some problems when moving the mouse from one shape to another
            // because this will destroy the box immediately
            selection->touch(); // force a redraw when dehighlighting
        }
        else if (selection->isHighlighted() &&
                 selection->selected.getValue() == SoFCSelection::NOTSELECTED &&
                 selection->style.getValue() == SoFCSelection::BOX) {
            PRIVATE(this)->basecolor->rgb.setValue(selection->colorHighlight.getValue());

            if (!PRIVATE(this)->selectsearch) {
                PRIVATE(this)->selectsearch = new SoSearchAction;
            }
            PRIVATE(this)->selectsearch->setType(SoShape::getClassTypeId());
            PRIVATE(this)->selectsearch->setInterest(SoSearchAction::FIRST);
            PRIVATE(this)->selectsearch->apply(selection);
            SoPath* shapepath = PRIVATE(this)->selectsearch->getPath();
            if (shapepath) {
                SoPathList list;
                list.append(shapepath);
                // clear old highlighting node if still active
                if (PRIVATE(this)->highlightPath) {
                    PRIVATE(this)->highlightPath->unref();
                }
                PRIVATE(this)->highlightPath = path;
                PRIVATE(this)->highlightPath->ref();
                this->drawBoxes(path, &list);
            }
            PRIVATE(this)->selectsearch->reset();
        }
    }
}

void
SoBoxSelectionRenderAction::apply(const SoPathList & pathlist,
                                  SbBool obeysrules)
{
    SoGLRenderAction::apply(pathlist, obeysrules);
}

void
SoBoxSelectionRenderAction::setColor(const SbColor & color)
{
    PRIVATE(this)->basecolor->rgb = color;
}

const SbColor &
SoBoxSelectionRenderAction::getColor()
{
    return PRIVATE(this)->basecolor->rgb[0];
}

void
SoBoxSelectionRenderAction::setLinePattern(unsigned short pattern)
{
    PRIVATE(this)->drawstyle->linePattern = pattern;
}

unsigned short
SoBoxSelectionRenderAction::getLinePattern() const
{
    return PRIVATE(this)->drawstyle->linePattern.getValue();
}

void
SoBoxSelectionRenderAction::setLineWidth(const float width)
{
    PRIVATE(this)->drawstyle->lineWidth = width;
}

float
SoBoxSelectionRenderAction::getLineWidth() const
{
    return PRIVATE(this)->drawstyle->lineWidth.getValue();
}

void
SoBoxSelectionRenderAction::drawBoxes(SoPath * pathtothis, const SoPathList * pathlist)
{
    int i;
    int thispos = static_cast<SoFullPath *>(pathtothis)->getLength()-1;
    assert(thispos >= 0);
    PRIVATE(this)->postprocpath->truncate(0); // reset

    for (i = 0; i < thispos; i++)
        PRIVATE(this)->postprocpath->append(pathtothis->getNode(i));

    // we need to disable accumulation buffer antialiasing while
    // rendering selected objects
    int oldnumpasses = this->getNumPasses();
    this->setNumPasses(1);

    SoState * thestate = this->getState();
    thestate->push();

    for (i = 0; i < pathlist->getLength(); i++) {
        auto path = static_cast<SoFullPath *>((*pathlist)[i]);

        for (int j = 0; j < path->getLength(); j++) {
            PRIVATE(this)->postprocpath->append(path->getNode(j));
        }

        // Previously SoGLRenderAction was used to draw the bounding boxes
        // of shapes in selection paths, by overriding renderstyle state
        // elements to lines drawstyle and simply doing:
        //
        //   SoGLRenderAction::apply(PRIVATE(this)->postprocpath); // Bug
        //
        // This could have the unwanted side effect of rendering
        // non-selected shapes, as they could be part of the path (due to
        // being placed below SoGroup nodes (instead of SoSeparator
        // nodes)) up to the selected shape.
        //
        //
        // A better approach turned out to be to soup up and draw only the
        // bounding boxes of the selected shapes:
        PRIVATE(this)->updateBbox(PRIVATE(this)->postprocpath);

        // Remove temporary path from path buffer
        PRIVATE(this)->postprocpath->truncate(thispos);
    }

    this->setNumPasses(oldnumpasses);
    thestate->pop();
}


#undef PRIVATE
#undef PUBLIC
