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

/*

  Quarter is a light-weight glue library that provides seamless
  integration between Systems in Motions's \COIN high-level 3D
  visualization library and Trolltech's \QT 2D user interface
  library.

  \QT and \COIN is a perfect match since they are both open source,
  widely portable and easy to use. Quarter has evolved from Systems in
  Motion's own experiences using \COIN and \QT together in our
  applications.

  The functionality in Quarter revolves around QuarterWidget, a
  subclass of QGLWidget. This widget provides functionality for
  rendering of Coin scenegraphs and translation of QEvents into
  SoEvents. Using this widget is as easy as using any other QWidget.

  \subpage QuarterWidgetPlugin

  Quarter also comes with a plugin for Qt Designer, Trolltech's tool
  for designing and building GUIs. Once you install Quarter, the
  QuarterWidget becomes accessible in Qt Designer, and you can include
  it in the user interfaces you create. The plugin facility also
  provides you with the capability of dynamically loading ui files
  containing a QuarterWidget in your application.

  By using \COIN, \QT and Quarter to build your 3D graphics
  applications, you have the power to write software that is portable
  across the whole range of UNIX, Linux, Microsoft Windows and Mac OS
  X operating systems, from a 100% common codebase.

  For a small, completely stand-alone usage example on how to
  initialize the library and set up a viewer instance window, see the
  following code:

  \code
  #include <QtGui/QApplication>

  #include <Inventor/nodes/SoBaseColor.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/nodes/SoSeparator.h>

  #include <Quarter/Quarter.h>
  #include <Quarter/QuarterWidget.h>

  using namespace SIM::Coin3D::Quarter;

  int
  main(int argc, char ** argv)
  {
    QApplication app(argc, argv);
    // Initializes Quarter library (and implicitly also the Coin and Qt
    // libraries).
    Quarter::init();

    // Make a dead simple scene graph by using the Coin library, only
    // containing a single yellow cone under the scenegraph root.
    SoSeparator * root = new SoSeparator;
    root->ref();

    SoBaseColor * col = new SoBaseColor;
    col->rgb = SbColor(1, 1, 0);
    root->addChild(col);

    root->addChild(new SoCone);

    // Create a QuarterWidget for displaying a Coin scene graph
    QuarterWidget * viewer = new QuarterWidget;
    viewer->setSceneGraph(root);

    // make the viewer react to input events similar to the good old
    // ExaminerViewer
    viewer->setNavigationModeFile(QUrl("coin:///scxml/navigation/examiner.xml"));

    // Pop up the QuarterWidget
    viewer->show();
    // Loop until exit.
    app.exec();
    // Clean up resources.
    root->unref();
    delete viewer;

    Quarter::clean();

    return 0;
  }
  \endcode

  \subpage examples
*/

// The subsequent doxygen referenced page/subpages do not exist in the copy of Quarter used within FreeCad.
// To preserve the history and their origin the doxygen commands have been disabled but left in the file.
// /*!
//  \page examples More Examples
//
//  The examples code is included in Quarter and can be found in the
//  src/examples subdirectory.
//
//  \subpage directui
//
//  \subpage dynamicui
//
//  \subpage inheritui
//
//  \subpage mdi
//
//  \subpage examiner
//*/

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/nodekits/SoNodeKit.h>

#include "SensorManager.h"

#include "Quarter.h"
#include "QuarterP.h"


using namespace SIM::Coin3D::Quarter;

static QuarterP * self = nullptr;

/*!
  initialize Quarter, and implicitly Coin
 */
void
Quarter::init(bool initCoin)
{
  COMPILE_ONLY_BEFORE(2,0,0,"Should not be encapsulated in double Quarter namespace");
  if (self) {
    // FIXME: Use SoDebugError
    fprintf(stderr, "Quarter is already initialized\n");
    return;
  }

  if (initCoin) {
    SoDB::init();
    SoNodeKit::init();
    SoInteraction::init();
  }

  self = new QuarterP;
  self->initCoin = initCoin;

}

/*!
  clean up resources
 */
void
Quarter::clean()
{
  COMPILE_ONLY_BEFORE(2,0,0,"Should not be encapsulated in double Quarter namespace");
  assert(self);
  bool initCoin = self->initCoin;

  delete self;
  self = nullptr;

  if (initCoin) {
    // SoDB::finish() will clean up everything that has been
    // initialized. There's no need to add SoNodeKit::finish() and
    // SoInteraction::finish() like in TGS Inventor
    SoDB::finish();
  }
}

/*!
  override lower refresh rate limit
 */
void
Quarter::setTimerEpsilon(double sec)
{
  COMPILE_ONLY_BEFORE(2,0,0,"Should not be encapsulated in double Quarter namespace");
  if (!self) {
    fprintf(stderr, "Quarter is not initialized!\n");
    return;
  }

  self->sensormanager->setTimerEpsilon(sec);
}
